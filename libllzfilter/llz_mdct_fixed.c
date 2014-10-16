/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_mdct_fixed.c 
  time    : 2012/07/19 - 2012/07/20  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "llz_mdct_fixed.h"
#include "llz_fft_fixed.h"

#ifndef		M_PI
#define		M_PI							3.14159265358979323846
#endif

#ifndef LLZ_CMUL
/*#define LLZ_CMUL(dre, dim, are, aim, bre, bim) do { \*/
        /*(dre) = (are) * (bre) - (aim) * (bim);  \*/
        /*(dim) = (are) * (bim) + (aim) * (bre);  \*/
    /*} while (0)*/
#define LLZ_CMUL(dre, dim, are, aim, bre, bim)  { \
        (dre) = (are) * (bre) - (aim) * (bim);  \
        (dim) = (are) * (bim) + (aim) * (bre);  \
    } 
#endif

typedef struct _llz_mdct_fixed_ctx_t {
    int     type;
    int     length;

    /*fft used by mdct1 and mdct2, shared by pos and inv*/
    unsigned long h_fft;
    int     *fft_buf;

    /*mdct0*/
    int     *mdct_work;
    short   **cos_ang_pos;
    short   **cos_ang_inv;

    /*mdct1*/
    short   *pre_c_pos, *pre_s_pos;
    short   *c_pos    , *s_pos;
    short   *pre_c_inv, *pre_s_inv;
    short   *c_inv    , *s_inv;

    /*mdct2*/
    short   *tw_c, *tw_s;
    int     *rot; 
    short   sqrt_cof;

    void    (*mdct_func)(struct _llz_mdct_fixed_ctx_t *f, const int *x, int *X, int N);
    void    (*imdct_func)(struct _llz_mdct_fixed_ctx_t *f, const int *X, int *x, int N2);

}llz_mdct_fixed_ctx_t;


static short ** matrix_short_init(int Nrow, int Ncol)
{
    short **A;
    int i;

    /* Allocate the row pointers */
    A = (short **) malloc (Nrow * sizeof (short *));

    /* Allocate the matrix of short values */
    A[0] = (short *) malloc (Nrow * Ncol * sizeof (short));
    memset(A[0], 0, Nrow*Ncol*sizeof(short));

    /* Set up the pointers to the rows */
    for (i = 1; i < Nrow; ++i)
        A[i] = A[i-1] + Ncol;

    return A;
}

static void matrix_short_uninit(short **A)
{
    /*free the Nrow*Ncol data space*/
    if(A[0])
        free(A[0]);

    /*free the row pointers*/
    if(A)
        free(A);

}
/*
int llz_sine_win(double *w, int N)
{
    double tmp;
    int   n;

    for(n = 0; n < N; n++) {
        tmp = (M_PI/N) * (n + 0.5);
        w[n] = sin(tmp); 
    }

    return N;
}
*/

/*
    after I test, I found that mdct0_fixed is the largest err when use fixed point, because 
    the MAC always use FIX32X15, and the accumlate err is increasing when N goes up, so
    we can not use mdct0 fixed in the applicaion, the mdct2_fixed is the smallest
    err in my test experiment
*/
static void mdct0_fixed(llz_mdct_fixed_ctx_t *f, const int *x, int *X, int N)
{
    int k, n;
    int N2;

    int Xk;

    N2 = N >> 1;

    for(k = 0; k < N2; k++) {
        Xk = 0;
        for(n = 0; n < N; n++) {
            Xk += LLZ_FIXMUL_32X15(x[n], f->cos_ang_pos[k][n]);
        }
        X[k] = Xk;
    }

}

static void imdct0_fixed(llz_mdct_fixed_ctx_t *f, const int *X, int *x, int N2)
{
    int n, k;
    int N;

    int xn;

    N = N2 << 1;

    for(n = 0; n < N; n++) {
        xn = 0;
        for(k = 0; k < N2; k++) {
            xn += LLZ_FIXMUL_32X15(X[k], f->cos_ang_inv[n][k]);
        }
        x[n] = (xn * 4)/N;
    }

}


static void mdct1_fixed(llz_mdct_fixed_ctx_t *f, const int *x, int *X, int N)
{
    int k;
    int N2;

    N2 = N >> 1;

    for(k = 0; k < N; k++) {
        f->fft_buf[k+k]   = LLZ_FIXMUL_32X15(x[k], f->pre_c_pos[k]);
        f->fft_buf[k+k+1] = LLZ_FIXMUL_32X15(x[k], f->pre_s_pos[k]);
    }

    llz_fft_fixed(f->h_fft, f->fft_buf);

    for(k = 0; k < N2; k++) 
        X[k] = LLZ_FIXMUL_32X15(f->fft_buf[k+k], f->c_pos[k]) - LLZ_FIXMUL_32X15(f->fft_buf[k+k+1], f->s_pos[k]);

}

static void imdct1_fixed(llz_mdct_fixed_ctx_t *f, const int *X, int *x, int N2)
{
    int k, i;
    int N;

    N = N2 << 1;

    for(k = 0; k < N2; k++) {
        f->fft_buf[k+k]   = LLZ_FIXMUL_32X15(X[k], f->pre_c_inv[k]);
        f->fft_buf[k+k+1] = LLZ_FIXMUL_32X15(X[k], f->pre_s_inv[k]);
    }
    for(k = N2, i = N2 -1; k < N; k++, i--) {
        f->fft_buf[k+k]   = LLZ_FIXMUL_32X15(-X[i], f->pre_c_inv[k]);
        f->fft_buf[k+k+1] = LLZ_FIXMUL_32X15(-X[i], f->pre_s_inv[k]);
    }

    llz_ifft_fixed(f->h_fft, f->fft_buf);

    for(k = 0; k < N; k++) 
        x[k] = (LLZ_FIXMUL_32X15(f->fft_buf[k+k], f->c_inv[k]) - LLZ_FIXMUL_32X15(f->fft_buf[k+k+1], f->s_inv[k])) << 1;

}

static void mdct2_fixed(llz_mdct_fixed_ctx_t *f, const int *x, int *X, int N)
{
    int k;
    int N2;
    int N4;
    int re, im;
    int *rot = f->rot;

    N2 = N >> 1;
    N4 = N >> 2;

    memset(rot, 0, sizeof(int)*f->length);
    /*shift x*/
    for(k = 0; k < N4; k++) 
        rot[k] = -x[k+3*N4];
    for(k = N4; k < N; k++)
        rot[k] = x[k-N4];

    /*pre twiddle*/
    for(k = 0; k < N4; k++) {
        re = rot[2*k]      - rot[N-1-2*k];
        im = rot[N2-1-2*k] - rot[N2+2*k] ;
        f->fft_buf[k+k]   = (LLZ_FIXMUL_32X15(re, f->tw_c[k]) - LLZ_FIXMUL_32X15(im, f->tw_s[k])) >> 1;
        f->fft_buf[k+k+1] = (LLZ_FIXMUL_32X15(re, f->tw_s[k]) + LLZ_FIXMUL_32X15(im, f->tw_c[k])) >> 1;
    }

    llz_fft_fixed(f->h_fft, f->fft_buf);

    /*post twiddle*/
    for(k = 0; k < N4; k++) {
        re = f->fft_buf[k+k];
        im = f->fft_buf[k+k+1];
        X[2*k]      =  2 * (LLZ_FIXMUL_32X15(re, f->tw_c[k]) - LLZ_FIXMUL_32X15(im, f->tw_s[k]));
        X[N2-1-2*k] = -2 * (LLZ_FIXMUL_32X15(re, f->tw_s[k]) + LLZ_FIXMUL_32X15(im, f->tw_c[k]));
    }

}

static void imdct2_fixed(llz_mdct_fixed_ctx_t *f, const int *X, int *x, int N2)
{
    int k;
    int N;
    int N4;
    int re, im;
    int *rot = f->rot;
    short cof = f->sqrt_cof;
    int tmp;

    N  = N2 << 1;
    N4 = N2 >> 1;

    memset(rot, 0, sizeof(int)*f->length);

    /*pre twiddle*/
    for(k = 0; k < N4; k++) {
        re = X[2*k];
        im = X[N2-1-2*k];
        f->fft_buf[k+k]   = (LLZ_FIXMUL_32X15(re, f->tw_c[k]) - LLZ_FIXMUL_32X15(im, f->tw_s[k])) >> 1;
        f->fft_buf[k+k+1] = (LLZ_FIXMUL_32X15(re, f->tw_s[k]) + LLZ_FIXMUL_32X15(im, f->tw_c[k])) >> 1;
    }

    llz_fft_fixed(f->h_fft, f->fft_buf);

    /*post twiddle*/
    for(k = 0; k < N4; k++) {
        re = f->fft_buf[k+k];
        im = f->fft_buf[k+k+1];
        tmp = LLZ_FIXMUL_32X15(re, f->tw_c[k]) - LLZ_FIXMUL_32X15(im, f->tw_s[k]);
        f->fft_buf[k+k]   = 8 * LLZ_FIXMUL_32X15(tmp, cof);
        tmp = LLZ_FIXMUL_32X15(re, f->tw_s[k]) + LLZ_FIXMUL_32X15(im, f->tw_c[k]);
        f->fft_buf[k+k+1] = 8 * LLZ_FIXMUL_32X15(tmp, cof);
    }

    /*shift*/
    for(k = 0; k < N4; k++) {
        rot[2*k]    = f->fft_buf[k+k];
        rot[N2+2*k] = f->fft_buf[k+k+1];
    }
    for(k = 1; k < N; k+=2)
        rot[k] = -rot[N-1-k];

    for(k = 0; k < 3*N4; k++)
        x[k] = LLZ_FIXMUL_32X15(rot[N4+k], cof);
    for(k = 3*N4; k < N; k++)
        x[k] = LLZ_FIXMUL_32X15(-rot[k-3*N4], cof);

}

unsigned long llz_mdct_fixed_init(int type, int size)
{
    int   k, n;
    double tmp;
    int   base;
    int   length;
    llz_mdct_fixed_ctx_t *f = NULL;

    f = (llz_mdct_fixed_ctx_t *)malloc(sizeof(llz_mdct_fixed_ctx_t));
    memset(f, 0, sizeof(llz_mdct_fixed_ctx_t));

    base = (int)(log(size)/log(2));
    if((1<<base) < size)
        base += 1;

    length    = (1 << base);
    f->length = length;

    f->type = type;

    switch(type) {
        case MDCT_FIXED_ORIGIN:{
                                 /*mdct0 init*/
                                 f->mdct_work   = (int *)malloc(sizeof(int)*(length>>1));
                                 memset(f->mdct_work, 0, sizeof(int)*(length>>1));
                                 f->cos_ang_pos = (short **)matrix_short_init(length>>1, length);
                                 f->cos_ang_inv = (short **)matrix_short_init(length   , length>>1);

                                 for(k = 0; k < (length>>1); k++) {
                                     for(n = 0; n < length; n++) {
                                         /* formula: cos( (pi/(2*N)) * (2*n + 1 + N/2) * (2*k + 1) ) */
                                         tmp = (M_PI/(2*length)) * (2*n + 1 + (length>>1)) * (2*k + 1);
                                         f->cos_ang_pos[k][n] = f->cos_ang_inv[n][k] = LLZ_FIX15(cos(tmp));
                                     }
                                 }

                                 f->mdct_func  = mdct0_fixed;
                                 f->imdct_func = imdct0_fixed;
                                 break;
                            }
        case MDCT_FIXED_FFT:{
                                 double n0;

                                 n0 = ((double)length/2 + 1) / 2;

                                 f->h_fft   = llz_fft_fixed_init(length);
                                 f->fft_buf = (int *)malloc(sizeof(int)*length*2);

                                 /*positive transform --> mdct*/
                                 f->pre_c_pos = (short *)malloc(sizeof(short)*length);
                                 f->pre_s_pos = (short *)malloc(sizeof(short)*length);
                                 f->c_pos     = (short *)malloc(sizeof(short)*(length>>1));
                                 f->s_pos     = (short *)malloc(sizeof(short)*(length>>1));

                                 for(k = 0; k < length; k++) {
                                     f->pre_c_pos[k] = LLZ_FIX15(cos(-(M_PI*k)/length));
                                     f->pre_s_pos[k] = LLZ_FIX15(sin(-(M_PI*k)/length));
                                 }
                                 for(k = 0; k < (length>>1); k++) {
                                     f->c_pos[k] = LLZ_FIX15(cos(-2*M_PI*n0*(k+0.5)/length)); 
                                     f->s_pos[k] = LLZ_FIX15(sin(-2*M_PI*n0*(k+0.5)/length)); 
                                 }


                                 /*inverse transform -->imdct*/
                                 f->pre_c_inv = (short*)malloc(sizeof(short)*length);
                                 f->pre_s_inv = (short*)malloc(sizeof(short)*length);
                                 f->c_inv     = (short*)malloc(sizeof(short)*length);
                                 f->s_inv     = (short*)malloc(sizeof(short)*length);

                                 for(k = 0; k < length; k++) {
                                     f->pre_c_inv[k] = LLZ_FIX15(cos((2*M_PI*k*n0)/length));
                                     f->pre_s_inv[k] = LLZ_FIX15(sin((2*M_PI*k*n0)/length));
                                 }
                                 for(k = 0; k < length; k++) {
                                     f->c_inv[k] = LLZ_FIX15(cos(M_PI*(k+n0)/length)); 
                                     f->s_inv[k] = LLZ_FIX15(sin(M_PI*(k+n0)/length)); 
                                 }

                                 f->mdct_func  = mdct1_fixed;
                                 f->imdct_func = imdct1_fixed;
                                 break;
                             }
        case MDCT_FIXED_FFT4:{
                                 f->h_fft   = llz_fft_fixed_init(length>>2);
                                 f->fft_buf = (int *)malloc(sizeof(int)*(length>>1));
                                 f->sqrt_cof = LLZ_FIX15(1./sqrt(length));

                                 f->rot = (int *)malloc(sizeof(int)*length);
                                 f->tw_c = (short *)malloc(sizeof(short)*(length>>2));
                                 f->tw_s = (short *)malloc(sizeof(short)*(length>>2));

                                 memset(f->rot, 0, sizeof(int)*length);
                                 for(k = 0; k < (length>>2); k++) {
                                     f->tw_c[k] = LLZ_FIX15(cos(-2*M_PI*(k+0.125)/length));
                                     f->tw_s[k] = LLZ_FIX15(sin(-2*M_PI*(k+0.125)/length));
                                 }

                                 f->mdct_func  = mdct2_fixed;
                                 f->imdct_func = imdct2_fixed;
                                 break;
                             }

    }

    return (unsigned long)f;
}


static void free_mdct_fixed_origin(llz_mdct_fixed_ctx_t *f)
{
    if(f->cos_ang_pos) {
        matrix_short_uninit(f->cos_ang_pos);
        f->cos_ang_pos = NULL;
    }

    if(f->cos_ang_inv) {
        matrix_short_uninit(f->cos_ang_inv);
        f->cos_ang_inv = NULL;
    }

    if(f->mdct_work) {
        free(f->mdct_work);
        f->mdct_work = NULL;
    }
}

static void free_mdct_fixed_fft(llz_mdct_fixed_ctx_t *f)
{
    if(f->pre_c_pos) {
        free(f->pre_c_pos);
        f->pre_c_pos = NULL;
    }

    if(f->pre_s_pos) {
        free(f->pre_s_pos);
        f->pre_s_pos = NULL;
    }

    if(f->pre_c_inv) {
        free(f->pre_c_inv);
        f->pre_c_inv = NULL;
    }

    if(f->pre_s_inv) {
        free(f->pre_s_inv);
        f->pre_s_inv = NULL;
    }

    if(f->c_pos) {
        free(f->c_pos);
        f->c_pos = NULL;
    }
 
    if(f->s_pos) {
        free(f->s_pos);
        f->s_pos = NULL;
    }

    if(f->c_inv) {
        free(f->c_inv);
        f->c_inv = NULL;
    }

    if(f->s_inv) {
        free(f->s_inv);
        f->s_inv = NULL;
    }

    if(f->fft_buf) {
        free(f->fft_buf);
        f->fft_buf = NULL;
    }

    llz_fft_fixed_uninit(f->h_fft);

}

static void free_mdct_fixed_fft4(llz_mdct_fixed_ctx_t *f)
{
    if (f->fft_buf) {
        free(f->fft_buf);
        f->fft_buf = NULL;
    }

    if(f->tw_c) {
        free(f->tw_c);
        f->tw_c = NULL;
    }

    if(f->tw_s) {
        free(f->tw_s);
        f->tw_s = NULL;
    }

    if(f->rot) {
        free(f->rot);
        f->rot = NULL;
    }

    llz_fft_fixed_uninit(f->h_fft);
}


void llz_mdct_fixed_uninit(unsigned long handle)
{
    int type;

    llz_mdct_fixed_ctx_t *f = (llz_mdct_fixed_ctx_t *)handle;
    type = f->type;

    if(f) {
        switch(type) {
            case MDCT_FIXED_ORIGIN:
                free_mdct_fixed_origin(f);
                break;
            case MDCT_FIXED_FFT:
                free_mdct_fixed_fft(f);
                break;
            case MDCT_FIXED_FFT4:
                free_mdct_fixed_fft4(f);
                break;
        }
        free(f);
        f = NULL;
    }

}

void llz_mdct_fixed(unsigned long handle, int *x, int *X)
{
    llz_mdct_fixed_ctx_t *f = (llz_mdct_fixed_ctx_t *)handle;

    f->mdct_func(f, x, X, f->length); 
}


void llz_imdct_fixed(unsigned long handle, int *X, int *x)
{
    llz_mdct_fixed_ctx_t *f = (llz_mdct_fixed_ctx_t *)handle;

    f->imdct_func(f, X, x, (f->length)>>1); 

}
