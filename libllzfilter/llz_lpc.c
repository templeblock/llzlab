/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_lpc.c
  time    : 2012/11/17 23:52
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "llz_lpc.h"
#include "llz_levinson.h"
#include "llz_corr.h"

typedef struct _llz_lpc_t {
    int p;      //order

    double *r;   //relation matrix will be used

    double *acof;
    double *kcof;
    double err;
} llz_lpc_t;


unsigned long llz_lpc_init(int p)
{
    llz_lpc_t *f = NULL;

    f = (llz_lpc_t *)malloc(sizeof(llz_lpc_t));
    memset(f, 0, sizeof(llz_lpc_t));

    f->p = p;

    f->r = (double *)malloc(sizeof(double)*(p+1));
    memset(f->r, 0, sizeof(double)*(p+1));
    f->acof = (double *)malloc(sizeof(double)*(p+1));
    memset(f->acof, 0, sizeof(double)*(p+1));
    f->kcof = (double *)malloc(sizeof(double)*(p+1));
    memset(f->kcof, 0, sizeof(double)*(p+1));
    f->err = 0.0;
 
    return (unsigned long)f;
}

void      llz_lpc_uninit(unsigned long handle)
{
    llz_lpc_t *f = (llz_lpc_t *)handle;

    if (f) {
        free(f->r);
        f->r = NULL;

        free(f->acof);
        f->acof = NULL;
        free(f->kcof);
        f->kcof = NULL;

        free(f);
        f = NULL;
    }
}


double llz_lpc(unsigned long handle, double *x, int x_len, double *lpc_cof, double *kcof, double *err)
{
    llz_lpc_t *f = (llz_lpc_t *)handle;
    int k;
    double gain;

    /*caculate autorelation matrix*/
    llz_autocorr(x, x_len, f->p, f->r);

    /*use levinson-durbin algorithm to calculate solution coffients*/
    llz_levinson(f->r, f->p, f->acof, f->kcof, &(f->err));

    *err = f->err / x_len;
    for (k = 0; k <= f->p; k++) {
        lpc_cof[k] = f->acof[k];
        kcof[k]    = f->kcof[k];
    }

    /*gain = f->err / f->r[0];*/
    if (f->err > 0)
        gain = f->r[0] / f->err;
    else 
        gain = 0.0;
    /*printf("\nerr=%f, r[0]=%f, gain=%f\n", f->err, f->r[0], gain);*/

    return gain;
}


