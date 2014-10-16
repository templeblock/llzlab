/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_corr.c 
  time    : 2012/11/13 22:52
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "llz_corr.h"
#include "llz_fft.h" 

/*
 *   p is the order
 *   WARN: p order , the r is the (p+1) demension
*/

/*
 * high presion 
 * WARN: use levinson method to resolve the relationship matrix will lead to different 
 *       result according to the input vector R, because the recursion of the levinson,
 *       so it is sensitive to the input vector, I simulate in matlab, the double *x and 
 *       doulbe *x will lead to little error R matrix, but totoally different visual 
 *       results resolved by levinson , so I add this function to vertify the results is 
 *       correct using this method
 */
void  llz_autocorr(double *x, int n, int p, double *r)
{
    int i, j, k;

    for (k = 0; k <= p; k++) {
        r[k] = 0.0;
        for (i = 0,j = k; j < n; i++,j++)  
            r[k] += x[i] * x[j];
    }
}

void  llz_crosscorr(double *x, double *y, int n, int p, double *r)
{
    int i, j, k;

    for (k = 0; k <= p; k++) {
        r[k] = 0.0;
        for (i = 0,j = k; j < n; i++,j++)  
            r[k] += x[i] * y[j];
    }
}

/*a and b are the frame which be caculate the correlation coffients*/
double llz_corr_cof(double *a, double *b, int len)
{
	int k;
	double ta,tb,tc;
	double rab;

	ta = tb = tc = 0;

	for (k = 0 ; k < len ; k++) {
		ta += a[k] * b[k];
		tb += a[k] * a[k];
		tc += b[k] * b[k];
	}

	rab = ta/sqrt(tb*tc);

	return rab;
}


static int nextpow2(int n)
{
    int level;
    int n1;

    level = log2(n);
    n1 = (1<<level);

    if (n1 < n)
        level += 1;

    return level;
}


typedef struct _llz_autocorr_fast_t {
    unsigned long h_fft;

    int   fft_len;
	double *fft_buf1;
    double *fft_buf2;
} llz_autocorr_fast_t;

unsigned long llz_autocorr_fast_init(int n)
{
    int level;

    llz_autocorr_fast_t *f = (llz_autocorr_fast_t *)malloc(sizeof(llz_autocorr_fast_t));
    memset(f, 0, sizeof(llz_autocorr_fast_t));

    /*level = nextpow2(2*n-1);*/
    level = nextpow2(2*n);
    /*printf("level = %d\n", level);*/

    f->fft_len  = (1<<level);
	f->h_fft    = llz_fft_init(f->fft_len);
    f->fft_buf1 = (double *)malloc(sizeof(double)*f->fft_len*2);
    f->fft_buf2 = (double *)malloc(sizeof(double)*f->fft_len*2);
    memset(f->fft_buf1, 0, sizeof(double)*f->fft_len*2);
    memset(f->fft_buf2, 0, sizeof(double)*f->fft_len*2);
 
    return (unsigned long)f;
}

void      llz_autocorr_fast_uninit(unsigned long handle)
{
    llz_autocorr_fast_t *f = (llz_autocorr_fast_t *)handle;

    if (f) {
        if (f->h_fft) 
            llz_fft_uninit(f->h_fft);

        if (f->fft_buf1) {
            free(f->fft_buf1);
            f->fft_buf1 = NULL;
        }

        if (f->fft_buf2) {
            free(f->fft_buf2);
            f->fft_buf2 = NULL;
        }

        free(f);
        f = NULL;
    }
}



/*
 *   ---------------------WARN!! VERY IMPORTANT-----------------------------------------
    this fast autocorr algorithm is faster than the ordinary correlation function 
    WHEN the n is large(maybe > 128) and the order p is close to the length n!! 
    the speed is obviously raise up when this conditions is satisfied.
    THUS, when you use small length correlation and low order p , I strongly suggested
    that you use the ordinary method llz_autocorr function to calculate the relation 
    matrix R
*/
void  llz_autocorr_fast(unsigned long handle, double *x, int n, int p, double *r)
{
    llz_autocorr_fast_t *f = (llz_autocorr_fast_t *)handle;
    int i;

    memset(f->fft_buf1, 0, sizeof(double)*f->fft_len*2);
    for (i = 0; i < n; i++) {
        f->fft_buf1[2*i]   = x[i];
        f->fft_buf1[2*i+1] = 0.0;
    }
    llz_fft(f->h_fft, f->fft_buf1);

    memset(f->fft_buf2, 0, sizeof(double)*f->fft_len*2);
    for (i = 0; i < n; i++) {
        f->fft_buf2[2*i]   = f->fft_buf1[2*i]   * f->fft_buf1[2*i]  + 
                             f->fft_buf1[2*i+1] * f->fft_buf1[2*i+1];
        f->fft_buf2[2*i+1] = 0.0;
    }
    llz_ifft(f->h_fft, f->fft_buf2);
    for (i = 0; i <= p; i++) {
        r[i] = f->fft_buf2[2*i] * 2;
        /*printf("r_fast[%d]=%f\n", i, r[i]);*/
    }
}

