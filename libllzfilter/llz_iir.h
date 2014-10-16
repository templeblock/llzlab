/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_iir.h 
  time    : 2013/03/18 18:38 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#ifndef _LLZ_IIR_H
#define _LLZ_IIR_H 

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif


unsigned long llz_iir_filter_init(int M, double *a, int N, double *b);
void      llz_iir_filter_uninit(unsigned long handle);
int       llz_iir_filter(unsigned long handle, double *x, double *y, int frame_len);
int       llz_iir_filter_flush(unsigned long handle, double *y);



#ifdef __cplusplus
}
#endif

#endif
