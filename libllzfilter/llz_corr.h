/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_corr.h 
  time    : 2012/11/13 22:52
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#ifndef _LLZ_CORR_H
#define _LLZ_CORR_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  


void  llz_autocorr(double *x, int n, int p, double *r);

void  llz_crosscorr(double *x, double *y, int n, int p, double *r);

double llz_corr_cof(double *a, double *b, int len);

unsigned long llz_autocorr_fast_init(int n);
void  llz_autocorr_fast_uninit(unsigned long handle);
void  llz_autocorr_fast(unsigned long handle, double *x, int n, int p, double *r);


#ifdef __cplusplus 
}
#endif  



#endif
