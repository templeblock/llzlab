/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_asmodel.c 
  time    : 2012/07/21 - 2012/07/23  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/



#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "llz_asmodel.h"

/*
    Peference:
 
    The purpose of this source file is only to teach you how to use OLA and mdct TDAC feature to 
    analysis and synthesis signal, this 2 methods is the basis to study and analysis signal in 
    time-frequence domain, the filterbank or subbands thoery will based on this simple trick.
*/
typedef struct _llz_asmodel_fft_t {
    int         frame_len;
    int         fft_len;

    double *     x_buf;
    unsigned long   h_fft;
    double *     fft_buf;
    double *     window;
/*
    NOTE:   this magic num is no theory support, it is tested by me(luolongzhi) when I
            compare the input buf_in[i] and output buf_out[i], and use this magic number to make
            them almost same.  I guess, becuase the overlap, the overlap add parts maybe increase
            according to the increasing overlap parts .
            And I just test 50% overlap and 75% overlap , in my experiment, using 50%,
            the magic_num=1, and using 75% overlap, the magic num=0.812.  In addition, using 
            blackman and kaier window is better than the hamming window

            if you want to modify this magic num and want compare the input and output, be careful,
            the buf_out is the delayed signal for I use zero pad before the buf_in, the length of
            the zero pad is frame_len(half of the fft_len)

    WARN:   I found that it is better to use 75% overlap, because the reconstruct signal of 50% 
            is not good, if you want to use 50% overlap, do not use fft transform ,please use
            mdct with TDAC feature, also use the power complement window(sine window and kaiser
            window) to reconstruct the signal
                                                           
                                                                      2012-07-22 luolongzhi

*/
    double       magic_num;
}llz_asmodel_fft_t;


typedef struct _llz_asmodel_mdct_t {
    int        frame_len;
    int        mdct_len;

    double *    x_buf;
    unsigned long  h_mdct;
    double *    mdct_buf;
    double *    window;

}llz_asmodel_mdct_t;

/*
    the digram of the algorithm block 

    take 50% overlap for example
    |---first one---|
    |----zero pads--|
    |---frame_len---|---frame_len---|
    |-----------win_len-------------| ==> window direction
    |-----------fft_len-------------|
    |-------re-------|              |
    |-------im-------|              |
             ||                             
             ||                            
             \/                           
                    |-----------win_len-------------| ==> window direction
                    |-----------fft_len-------------|
                    |-------re-------|              |
                    |-------im-------|              |
                            ||
                            ||
                            \/
             
    |-----------fft_len-------------|
    |-----------win_len-------------|
    |----------overlap buf----------| ==> move direction
    <*********** OLA ***************>
    |---frame_len---|
                    |-----------fft_len-------------|
                    |-----------win_len-------------|
                    |----------overlap buf----------|  ==> move direction
                    <*********** OLA ***************>
    |....delay..... |---frame_len---|

    NOTE: 50% overlap, delay is 1 frame_len, for the fft_len is 2*frame_len, the 2nd frame will out 
          75% overlap, delay is 3 frame_len, for the fft_len is 4*frame_len, the 4th frame will out

                                                                2012-07-22 luolongzhi

*/
unsigned long llz_analysis_fft_init(int overlap_hint, int frame_len, win_t win_type)
{
    llz_asmodel_fft_t * f = (llz_asmodel_fft_t *)malloc(sizeof(llz_asmodel_fft_t));

    f->frame_len = frame_len;
    switch (overlap_hint) {
        case LLZ_OVERLAP_HIGH:
            f->fft_len = frame_len << 2;
            f->magic_num = 0.812;
            break;
        case LLZ_OVERLAP_LOW:
            f->fft_len = frame_len << 1;
            f->magic_num = 1;
            break;
    }

    f->x_buf = (double *)malloc(sizeof(double)*f->fft_len);
    memset(f->x_buf, 0, sizeof(double)*f->fft_len);

    f->fft_buf = (double *)malloc(sizeof(double)*(f->fft_len<<1));
    memset(f->fft_buf, 0, sizeof(double)*(f->fft_len<<1));
    f->h_fft = llz_fft_init(f->fft_len);
    f->window = (double *)malloc(sizeof(double)*f->fft_len);

    switch (win_type) {
        case HAMMING:
            llz_hamming(f->window, f->fft_len);
            break;
        case BLACKMAN:
            llz_blackman(f->window, f->fft_len);
            break;
        case KAISER:
            llz_kaiser(f->window, f->fft_len);
            break;
    }

    return (unsigned long)f;

}

void      llz_analysis_fft_uninit(unsigned long handle)
{
    llz_asmodel_fft_t *f = (llz_asmodel_fft_t *)handle;

    if (f) {
        if (f->x_buf) {
            free(f->x_buf);
            f->x_buf = NULL;
        }

        if (f->fft_buf) {
            free(f->fft_buf);
            f->fft_buf = NULL;
        }

        if (f->window) {
            free(f->window);
            f->window = NULL;
        }

        if (f->h_fft) {
            llz_fft_uninit(f->h_fft);
        }

    }

}

void      llz_analysis_fft(unsigned long handle, double *x, double *re, double *im)
{
    llz_asmodel_fft_t *f = (llz_asmodel_fft_t *)handle;
    int   frame_len = f->frame_len;
    int   fft_len   = f->fft_len;
    double *w        = f->window;
    int i;

    /*update x_buf, copy the remain data to the beginning position*/
    for (i = 0; i < (fft_len-frame_len); i++) 
        f->x_buf[i] = f->x_buf[i+frame_len];
    for (i = 0; i < frame_len; i++)
        f->x_buf[i+fft_len-frame_len] = x[i];

    for (i = 0; i < fft_len; i++) {
        f->fft_buf[i+i]   = f->x_buf[i] * w[i];
        f->fft_buf[i+i+1] = 0;
    }

    llz_fft(f->h_fft, f->fft_buf);

    for (i = 0; i < (fft_len>>1)+1; i++) {
        re[i] = f->fft_buf[i+i];
        im[i] = f->fft_buf[i+i+1];
    }

}

unsigned long llz_synthesis_fft_init(int overlap_hint, int frame_len, win_t win_type)
{
    llz_asmodel_fft_t * f = (llz_asmodel_fft_t *)malloc(sizeof(llz_asmodel_fft_t));

    f->frame_len = frame_len;
    switch (overlap_hint) {
        case LLZ_OVERLAP_HIGH:
            f->fft_len = frame_len << 2;
            f->magic_num = 0.812;
            break;
        case LLZ_OVERLAP_LOW:
            f->fft_len = frame_len << 1;
            f->magic_num = 1;
            break;
    }

    f->x_buf = (double *)malloc(sizeof(double)*f->fft_len);
    memset(f->x_buf, 0, sizeof(double)*f->fft_len);

    f->fft_buf = (double *)malloc(sizeof(double)*(f->fft_len<<1));
    memset(f->fft_buf, 0, sizeof(double)*(f->fft_len<<1));
    f->h_fft = llz_fft_init(f->fft_len);
    f->window = (double *)malloc(sizeof(double)*f->fft_len);

    switch (win_type) {
        case HAMMING:
            llz_hamming(f->window, f->fft_len);
            break;
        case BLACKMAN:
            llz_blackman(f->window, f->fft_len);
            break;
        case KAISER:
            llz_kaiser(f->window, f->fft_len);
            break;
    }

    return (unsigned long)f;

}


void      llz_synthesis_fft_uninit(unsigned long handle)
{
    llz_asmodel_fft_t *f = (llz_asmodel_fft_t *)handle;

    if (f) {
        if (f->x_buf) {
            free(f->x_buf);
            f->x_buf = NULL;
        }

        if (f->fft_buf) {
            free(f->fft_buf);
            f->fft_buf = NULL;
        }

        if (f->window) {
            free(f->window);
            f->window = NULL;
        }

        if (f->h_fft) {
            llz_fft_uninit(f->h_fft);
        }

    }
}


void      llz_synthesis_fft(unsigned long handle, double *re, double *im, double *x)
{
    llz_asmodel_fft_t *f = (llz_asmodel_fft_t *)handle;
    int   frame_len = f->frame_len;
    int   fft_len   = f->fft_len;
    double *w        = f->window;
    double magic_num = f->magic_num;
    int i, j;

    for (i = 0; i < (fft_len>>1)+1; i++) {
        f->fft_buf[i+i]   = re[i];
        f->fft_buf[i+i+1] = im[i];
    }
    /*real parts is symmetric, the image parts is asymmetric*/
    for (i = 0, j = (fft_len>>1)-1; i < (fft_len>>1)-1; i++,j--) {
        f->fft_buf[fft_len+2+2*i]   = re[j];
        f->fft_buf[fft_len+2+2*i+1] = -im[j];
    }

    llz_ifft(f->h_fft, f->fft_buf);

    /*OLA*/
    for (i = 0; i < fft_len; i++) 
        f->x_buf[i] += f->fft_buf[i+i] * w[i] ;

    /*magic_num to scale the output signal*/
    for (i = 0; i < frame_len; i++)
        x[i] = magic_num * f->x_buf[i];

    /*update x_buf*/
    for (i = 0; i < (fft_len-frame_len); i++) 
        f->x_buf[i] = f->x_buf[i+frame_len];
    for (i = 0; i < frame_len; i++)
        f->x_buf[i+fft_len-frame_len] = 0;

}


unsigned long llz_analysis_mdct_init(int frame_len, mdct_win_t win_type)
{
    llz_asmodel_mdct_t * f = (llz_asmodel_mdct_t *)malloc(sizeof(llz_asmodel_mdct_t));

    f->frame_len = frame_len;
    f->mdct_len  = frame_len << 1;
    f->x_buf     = (double *)malloc(sizeof(double)*f->mdct_len);
    memset(f->x_buf, 0, sizeof(double)*f->mdct_len);

    f->mdct_buf = (double *)malloc(sizeof(double)*f->mdct_len);
    memset(f->mdct_buf, 0, sizeof(double)*f->mdct_len);
    f->h_mdct = llz_mdct_init(MDCT_FFT4, f->mdct_len);
    f->window = (double *)malloc(sizeof(double)*f->mdct_len);

    switch (win_type) {
        case MDCT_SINE:
            llz_mdct_sine(f->window, f->mdct_len);
            break;
        case MDCT_KBD:
            llz_mdct_kbd(f->window, f->mdct_len, 6);
            break;
    }

    return (unsigned long)f;

}

void      llz_analysis_mdct_uninit(unsigned long handle)
{
    llz_asmodel_mdct_t *f = (llz_asmodel_mdct_t *)handle;

    if (f) {
        if (f->x_buf) {
            free(f->x_buf);
            f->x_buf = NULL;
        }

        if (f->mdct_buf) {
            free(f->mdct_buf);
            f->mdct_buf = NULL;
        }

        if (f->window) {
            free(f->window);
            f->window = NULL;
        }

        llz_mdct_uninit(f->h_mdct);
    }

}


void      llz_analysis_mdct(unsigned long handle, double *x, double *X)
{
    llz_asmodel_mdct_t *f = (llz_asmodel_mdct_t *)handle;
    int   frame_len      = f->frame_len;
    double *w             = f->window;
    int i;

    /*update x_buf, 50% overlap, copy the remain half data to the beginning position*/
    for (i = 0; i < frame_len; i++) 
        f->x_buf[i] = f->x_buf[i+frame_len];
    for (i = 0; i < frame_len; i++)
        f->x_buf[i+frame_len] = x[i];

    for (i = 0; i < f->mdct_len; i++) 
        f->mdct_buf[i] = f->x_buf[i] * w[i];

    llz_mdct(f->h_mdct, f->mdct_buf, X);

}



unsigned long llz_synthesis_mdct_init(int frame_len, mdct_win_t win_type)
{
    llz_asmodel_mdct_t * f = (llz_asmodel_mdct_t *)malloc(sizeof(llz_asmodel_mdct_t));

    f->frame_len = frame_len;
    f->mdct_len  = frame_len << 1;
    f->x_buf     = (double *)malloc(sizeof(double)*f->mdct_len);
    memset(f->x_buf, 0, sizeof(double)*f->mdct_len);

    f->mdct_buf = (double *)malloc(sizeof(double)*f->mdct_len);
    memset(f->mdct_buf, 0, sizeof(double)*f->mdct_len);
    f->h_mdct = llz_mdct_init(MDCT_FFT4, f->mdct_len);
    f->window = (double *)malloc(sizeof(double)*f->mdct_len);

    switch (win_type) {
        case MDCT_SINE:
            llz_mdct_sine(f->window, f->mdct_len);
            break;
        case MDCT_KBD:
            llz_mdct_kbd(f->window, f->mdct_len, 6);
            break;
    }


    return (unsigned long)f;

}

void      llz_synthesis_mdct_uninit(unsigned long handle)
{
    llz_asmodel_mdct_t *f = (llz_asmodel_mdct_t *)handle;

    if (f) {
        if (f->x_buf) {
            free(f->x_buf);
            f->x_buf = NULL;
        }

        if (f->mdct_buf) {
            free(f->mdct_buf);
            f->mdct_buf = NULL;
        }

        if (f->window) {
            free(f->window);
            f->window = NULL;
        }

        llz_mdct_uninit(f->h_mdct);
    }

}

void      llz_synthesis_mdct(unsigned long handle, double *X, double *x)
{
    llz_asmodel_mdct_t *f = (llz_asmodel_mdct_t *)handle;
    int   frame_len      = f->frame_len;
    double *w             = f->window;
    int i;

    llz_imdct(f->h_mdct, X, f->mdct_buf);

    /*OLA and TDAC*/
    for (i = 0; i < f->mdct_len; i++) 
        f->x_buf[i] += f->mdct_buf[i] * w[i];

    for (i = 0; i < frame_len; i++)
        x[i] = f->x_buf[i];

    /*update x_buf, copy the remain data to the beginning position*/
    for (i = 0; i < frame_len; i++) 
        f->x_buf[i] = f->x_buf[i+frame_len];
    for (i = 0; i < frame_len; i++)
        f->x_buf[i+frame_len] = 0;

}




