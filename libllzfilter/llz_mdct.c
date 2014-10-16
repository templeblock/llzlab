/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_mdct.h 
  time    : 2012/07/16 - 2012/07/18  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "llz_mdct.h"
#include "llz_fft.h"

#ifndef		M_PI
#define		M_PI							3.14159265358979323846
#endif

#undef  EPS
#define EPS		1E-16

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

typedef struct _llz_mdct_ctx_t {
    int     type;
    int     length;

    /*fft used by mdct1 and mdct2, shared by pos and inv*/
    unsigned long h_fft;
    double   *fft_buf;

    /*mdct0*/
    double   *mdct_work;
    double   **cos_ang_pos;
    double   **cos_ang_inv;

    /*mdct1*/
    double   *pre_c_pos, *pre_s_pos;
    double   *c_pos    , *s_pos;
    double   *pre_c_inv, *pre_s_inv;
    double   *c_inv    , *s_inv;

    /*mdct2*/
    double   *tw_c, *tw_s;
    double   *rot; 
    double   sqrt_cof;

    void    (*mdct_func)(struct _llz_mdct_ctx_t *f, const double *x, double *X, int N);
    void    (*imdct_func)(struct _llz_mdct_ctx_t *f, const double *X, double *x, int N2);

}llz_mdct_ctx_t;


static double ** matrix_init(int Nrow, int Ncol)
{
    double **A;
    int i;

    /* Allocate the row pointers */
    A = (double **) malloc (Nrow * sizeof (double *));

    /* Allocate the matrix of double values */
    A[0] = (double *) malloc (Nrow * Ncol * sizeof (double));
    memset(A[0], 0, Nrow*Ncol*sizeof(double));

    /* Set up the pointers to the rows */
    for (i = 1; i < Nrow; ++i)
        A[i] = A[i-1] + Ncol;

    return A;
}

static void matrix_uninit(double **A)
{
    /*free the Nrow*Ncol data space*/
    if (A[0])
        free(A[0]);

    /*free the row pointers*/
    if (A)
        free(A);

}

int llz_mdct_sine(double *w, int N)
{
    double tmp;
    int   n;

    for (n = 0; n < N; n++) {
        tmp = (M_PI/N) * (n + 0.5);
        w[n] = sin(tmp); 
    }

    return N;
}

static double bessel(double x)
{
  	double  xh, sum, pow, ds;
 	int k;

  	xh  = (double)0.5 * x;
  	sum = 1.0;
  	pow = 1.0;
  	k   = 0;
  	ds  = 1.0;
 	while (ds > sum * EPS) {
        ++k;
        pow = pow * (xh / k);
        ds = pow * pow;
        sum = sum + ds;
  	}

 	return sum;	
}

static int kaiser_beta(double *w, const int N, const double beta)
{
	int i;
	double Ia,Ib;

	for (i = 0 ; i < N ; i++) {
		double x;
		
		Ib = bessel(beta);
		
		x = (double)((2. *i/(N-1))-1);
		Ia = bessel(beta*(double)sqrt(1. - x*x));
		
		w[i] = (double)(Ia/Ib);
	}

	return N;
}

/*you can read details of the KBD formula from wiki, google "mdct Kaiser wiki"*/
int llz_mdct_kbd(double *w, int N, double alpha)
{
    int i,j;
    int N2;
    double *w1;
    double sum = 0.0;
    double tmp = 0.0;

    N2 = N >> 1;
    w1 = (double *)malloc(sizeof(double)*(N2+1));

    /*generate kaiser-basel window*/
    kaiser_beta(w1, N2+1, alpha*M_PI);

    /*calculate the denuminotor sum*/
    for (i = 0; i < N2+1; i++) 
        sum += w1[i];
    sum = 1.0/sum;

    /*calculate the KBD, window symmetric*/
    tmp = 0.0;
    for (i = 0, j = N-1; i < N2; i++, j--) {
        tmp += w1[i];
        w[i] = w[j] = sqrt(tmp*sum);
    }

    free(w1);

    return N;
}


static void mdct0(llz_mdct_ctx_t *f, const double *x, double *X, int N)
{
    int k, n;
    int N2;

    double Xk;

    N2 = N >> 1;

    for (k = 0; k < N2; k++) {
        Xk = 0;
        for (n = 0; n < N; n++) {
            Xk += x[n] * f->cos_ang_pos[k][n];
        }
        X[k] = Xk;
    }

}

static void imdct0(llz_mdct_ctx_t *f, const double *X, double *x, int N2)
{
    int n, k;
    int N;

    double xn;

    N = N2 << 1;

    for (n = 0; n < N; n++) {
        xn = 0;
        for (k = 0; k < N2; k++) {
            xn += X[k] * f->cos_ang_inv[n][k];
        }
        x[n] = (xn * 4)/N;
    }

}


static void mdct1(llz_mdct_ctx_t *f, const double *x, double *X, int N)
{
    int k;
    int N2;

    N2 = N >> 1;

    for (k = 0; k < N; k++) {
        f->fft_buf[k+k]   = x[k] * f->pre_c_pos[k];
        f->fft_buf[k+k+1] = x[k] * f->pre_s_pos[k];
    }

    llz_fft(f->h_fft, f->fft_buf);

    for (k = 0; k < N2; k++) 
        X[k] = f->fft_buf[k+k] * f->c_pos[k] - f->fft_buf[k+k+1] * f->s_pos[k];

}

static void imdct1(llz_mdct_ctx_t *f, const double *X, double *x, int N2)
{
    int k, i;
    int N;

    N = N2 << 1;

    for (k = 0; k < N2; k++) {
        f->fft_buf[k+k]   = X[k] * f->pre_c_inv[k];
        f->fft_buf[k+k+1] = X[k] * f->pre_s_inv[k];
    }
    for (k = N2, i = N2 -1; k < N; k++, i--) {
        f->fft_buf[k+k]   = -X[i] * f->pre_c_inv[k];
        f->fft_buf[k+k+1] = -X[i] * f->pre_s_inv[k];
    }

    llz_ifft(f->h_fft, f->fft_buf);

    for (k = 0; k < N; k++) 
        x[k] = 2 * (f->fft_buf[k+k] * f->c_inv[k] - f->fft_buf[k+k+1] * f->s_inv[k]);

}

static void mdct2(llz_mdct_ctx_t *f, const double *x, double *X, int N)
{
    int k;
    int N2;
    int N4;
    double re, im;
    double *rot = f->rot;

    N2 = N >> 1;
    N4 = N >> 2;

    memset(rot, 0, sizeof(double)*f->length);
    /*shift x*/
    for (k = 0; k < N4; k++) 
        rot[k] = -x[k+3*N4];
    for (k = N4; k < N; k++)
        rot[k] = x[k-N4];

    /*pre twiddle*/
    for (k = 0; k < N4; k++) {
        re = rot[2*k]      - rot[N-1-2*k];
        im = rot[N2-1-2*k] - rot[N2+2*k] ;
        f->fft_buf[k+k]   = 0.5 * (re * f->tw_c[k] - im * f->tw_s[k]);
        f->fft_buf[k+k+1] = 0.5 * (re * f->tw_s[k] + im * f->tw_c[k]);
    }

    llz_fft(f->h_fft, f->fft_buf);

    /*post twiddle*/
    for (k = 0; k < N4; k++) {
        re = f->fft_buf[k+k];
        im = f->fft_buf[k+k+1];
        X[2*k]      =  2 * (re * f->tw_c[k] - im * f->tw_s[k]);
        X[N2-1-2*k] = -2 * (re * f->tw_s[k] + im * f->tw_c[k]);
    }

}

static void imdct2(llz_mdct_ctx_t *f, const double *X, double *x, int N2)
{
    int k;
    int N;
    int N4;
    double re, im;
    double *rot = f->rot;
    double cof = f->sqrt_cof;

    N  = N2 << 1;
    N4 = N2 >> 1;

    memset(rot, 0, sizeof(double)*f->length);

    /*pre twiddle*/
    for (k = 0; k < N4; k++) {
        re = X[2*k];
        im = X[N2-1-2*k];
        f->fft_buf[k+k]   = 0.5 * (re * f->tw_c[k] - im * f->tw_s[k]);
        f->fft_buf[k+k+1] = 0.5 * (re * f->tw_s[k] + im * f->tw_c[k]);
    }

    llz_fft(f->h_fft, f->fft_buf);

    /*post twiddle*/
    for (k = 0; k < N4; k++) {
        re = f->fft_buf[k+k];
        im = f->fft_buf[k+k+1];
        f->fft_buf[k+k]   = 8 * cof * (re * f->tw_c[k] - im * f->tw_s[k]);
        f->fft_buf[k+k+1] = 8 * cof * (re * f->tw_s[k] + im * f->tw_c[k]);
    }

    /*shift*/
    for (k = 0; k < N4; k++) {
        rot[2*k]    = f->fft_buf[k+k];
        rot[N2+2*k] = f->fft_buf[k+k+1];
    }
    for (k = 1; k < N; k+=2)
        rot[k] = -rot[N-1-k];

    for (k = 0; k < 3*N4; k++)
        x[k] = rot[N4+k] * cof;
    for (k = 3*N4; k < N; k++)
        x[k] = -rot[k-3*N4] * cof;

}

unsigned long llz_mdct_init(int type, int size)
{
    int   k, n;
    double tmp;
    int   base;
    int   length;
    llz_mdct_ctx_t *f = NULL;

    f = (llz_mdct_ctx_t *)malloc(sizeof(llz_mdct_ctx_t));
    memset(f, 0, sizeof(llz_mdct_ctx_t));

    base = (int)(log(size)/log(2));
    if ((1<<base) < size)
        base += 1;

    length    = (1 << base);
    f->length = length;

    f->type = type;

    switch (type) {
        case MDCT_ORIGIN:{
                             /*mdct0 init*/
                             f->mdct_work   = (double *)malloc(sizeof(double)*(length>>1));
                             memset(f->mdct_work, 0, sizeof(double)*(length>>1));
                             f->cos_ang_pos = (double **)matrix_init(length>>1, length);
                             f->cos_ang_inv = (double **)matrix_init(length   , length>>1);

                             for (k = 0; k < (length>>1); k++) {
                                 for (n = 0; n < length; n++) {
                                     /* formula: cos( (pi/(2*N)) * (2*n + 1 + N/2) * (2*k + 1) ) */
                                     tmp = (M_PI/(2*length)) * (2*n + 1 + (length>>1)) * (2*k + 1);
                                     f->cos_ang_pos[k][n] = f->cos_ang_inv[n][k] = cos(tmp);
                                 }
                             }

                             f->mdct_func  = mdct0;
                             f->imdct_func = imdct0;
                             break;
                         }
        case MDCT_FFT:   {
                             double n0;

                             n0 = ((double)length/2 + 1) / 2;

                             f->h_fft   = llz_fft_init(length);
                             f->fft_buf = (double *)malloc(sizeof(double)*length*2);

                             /*positive transform --> mdct*/
                             f->pre_c_pos = (double *)malloc(sizeof(double)*length);
                             f->pre_s_pos = (double *)malloc(sizeof(double)*length);
                             f->c_pos     = (double *)malloc(sizeof(double)*(length>>1));
                             f->s_pos     = (double *)malloc(sizeof(double)*(length>>1));

                             for (k = 0; k < length; k++) {
                                 f->pre_c_pos[k] = cos(-(M_PI*k)/length);
                                 f->pre_s_pos[k] = sin(-(M_PI*k)/length);
                             }
                             for (k = 0; k < (length>>1); k++) {
                                 f->c_pos[k] = cos(-2*M_PI*n0*(k+0.5)/length); 
                                 f->s_pos[k] = sin(-2*M_PI*n0*(k+0.5)/length); 
                             }


                             /*inverse transform -->imdct*/
                             f->pre_c_inv = (double *)malloc(sizeof(double)*length);
                             f->pre_s_inv = (double *)malloc(sizeof(double)*length);
                             f->c_inv     = (double *)malloc(sizeof(double)*length);
                             f->s_inv     = (double *)malloc(sizeof(double)*length);

                             for (k = 0; k < length; k++) {
                                 f->pre_c_inv[k] = cos((2*M_PI*k*n0)/length);
                                 f->pre_s_inv[k] = sin((2*M_PI*k*n0)/length);
                             }
                             for (k = 0; k < length; k++) {
                                 f->c_inv[k] = cos(M_PI*(k+n0)/length); 
                                 f->s_inv[k] = sin(M_PI*(k+n0)/length); 
                             }

                             f->mdct_func  = mdct1;
                             f->imdct_func = imdct1;
                             break;
                         }
        case MDCT_FFT4:  {
                             f->h_fft   = llz_fft_init(length>>2);
                             f->fft_buf = (double *)malloc(sizeof(double)*(length>>1));
                             f->sqrt_cof = 1./sqrt(length);

                             f->rot = (double *)malloc(sizeof(double)*length);
                             f->tw_c = (double *)malloc(sizeof(double)*(length>>2));
                             f->tw_s = (double *)malloc(sizeof(double)*(length>>2));

                             memset(f->rot, 0, sizeof(double)*length);
                             for (k = 0; k < (length>>2); k++) {
                                 f->tw_c[k] = cos(-2*M_PI*(k+0.125)/length);
                                 f->tw_s[k] = sin(-2*M_PI*(k+0.125)/length);
                             }

                             f->mdct_func  = mdct2;
                             f->imdct_func = imdct2;
                             break;
                         }

    }

    return (unsigned long)f;
}


static void free_mdct_origin(llz_mdct_ctx_t *f)
{
    if (f->cos_ang_pos) {
        matrix_uninit(f->cos_ang_pos);
        f->cos_ang_pos = NULL;
    }

    if (f->cos_ang_inv) {
        matrix_uninit(f->cos_ang_inv);
        f->cos_ang_inv = NULL;
    }

    if (f->mdct_work) {
        free(f->mdct_work);
        f->mdct_work = NULL;
    }
}

static void free_mdct_fft(llz_mdct_ctx_t *f)
{
    if (f->pre_c_pos) {
        free(f->pre_c_pos);
        f->pre_c_pos = NULL;
    }

    if (f->pre_s_pos) {
        free(f->pre_s_pos);
        f->pre_s_pos = NULL;
    }

    if (f->pre_c_inv) {
        free(f->pre_c_inv);
        f->pre_c_inv = NULL;
    }

    if (f->pre_s_inv) {
        free(f->pre_s_inv);
        f->pre_s_inv = NULL;
    }

    if (f->c_pos) {
        free(f->c_pos);
        f->c_pos = NULL;
    }
 
    if (f->s_pos) {
        free(f->s_pos);
        f->s_pos = NULL;
    }

    if (f->c_inv) {
        free(f->c_inv);
        f->c_inv = NULL;
    }

    if (f->s_inv) {
        free(f->s_inv);
        f->s_inv = NULL;
    }

    if (f->fft_buf) {
        free(f->fft_buf);
        f->fft_buf = NULL;
    }

    llz_fft_uninit(f->h_fft);

}

static void free_mdct_fft4(llz_mdct_ctx_t *f)
{
    if (f->fft_buf) {
        free(f->fft_buf);
        f->fft_buf = NULL;
    }

    if (f->tw_c) {
        free(f->tw_c);
        f->tw_c = NULL;
    }

    if (f->tw_s) {
        free(f->tw_s);
        f->tw_s = NULL;
    }

    if (f->rot) {
        free(f->rot);
        f->rot = NULL;
    }

    llz_fft_uninit(f->h_fft);
}


void llz_mdct_uninit(unsigned long handle)
{
    int type;

    llz_mdct_ctx_t *f = (llz_mdct_ctx_t *)handle;
    type = f->type;

    if (f) {
        switch (type) {
            case MDCT_ORIGIN:
                free_mdct_origin(f);
                break;
            case MDCT_FFT:
                free_mdct_fft(f);
                break;
            case MDCT_FFT4:
                free_mdct_fft4(f);
                break;
        }
        free(f);
        f = NULL;
    }

}

void llz_mdct(unsigned long handle, double *x, double *X)
{
    llz_mdct_ctx_t *f = (llz_mdct_ctx_t *)handle;

    f->mdct_func(f, x, X, f->length); 
}


void llz_imdct(unsigned long handle, double *X, double *x)
{
    llz_mdct_ctx_t *f = (llz_mdct_ctx_t *)handle;

    f->imdct_func(f, X, x, (f->length)>>1); 

}
