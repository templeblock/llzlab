/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_asmodel.h 
  time    : 2012/07/21 - 2012/07/23  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_ASMODEL_H
#define _LLZ_ASMODEL_H 

#include "llz_fir.h"
#include "llz_fft.h"
#include "llz_mdct.h"

#ifdef __cplusplus 
extern "C"
{ 
#endif  


enum {
    LLZ_OVERLAP_HIGH = 0,       //3/4 overlap
    LLZ_OVERLAP_LOW,            //1/2 overlap
};


/*
    in fact, the llz_analysis_fft_init is same as llz_synthesis_fft_init 
    I use 2 functions in consider that maybe someone want use one only
*/
unsigned long llz_analysis_fft_init(int overlap_hint, int frame_len, win_t win_type);
void      llz_analysis_fft_uninit(unsigned long handle);
void      llz_analysis_fft(unsigned long handle, double *x, double *re, double *im);

unsigned long llz_synthesis_fft_init(int overlap_hint, int frame_len, win_t win_type);
void      llz_synthesis_fft_uninit(unsigned long handle);
void      llz_synthesis_fft(unsigned long handle, double *re, double *im, double *x);


unsigned long llz_analysis_mdct_init(int frame_len, mdct_win_t win_type);
void      llz_analysis_mdct_uninit(unsigned long handle);
void      llz_analysis_mdct(unsigned long handle, double *x, double *X);

unsigned long llz_synthesis_mdct_init(int frame_len, mdct_win_t win_type);
void      llz_synthesis_mdct_uninit(unsigned long handle);
void      llz_synthesis_mdct(unsigned long handle, double *X, double *x);

#ifdef __cplusplus 
}
#endif  


#endif
