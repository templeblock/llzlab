/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_mdct.h 
  time    : 2012/07/16 - 2012/07/18  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_MDCT_H
#define _LLZ_MDCT_H

#ifdef __cplusplus 
extern "C"
{ 
#endif  


typedef int mdct_win_t;

/*
    origin: the naive mdct using the original formular, this mdct is leading to you learning mdct
    fft   : normally called fast mdct, use fft transform to compute mdct
    fft4  : the wildly used, N/4 point FFT to compute mdct
*/
enum{
    MDCT_ORIGIN = 0,
    MDCT_FFT,
    MDCT_FFT4,
};

enum{
    MDCT_SINE = 0,
    MDCT_KBD,
};

unsigned long llz_mdct_init(int type, int len);
void      llz_mdct_uninit(unsigned long handle);

void llz_mdct(unsigned long handle, double *x, double *X);
void llz_imdct(unsigned long handle, double *X, double *x);

int llz_mdct_sine(double *w, int N);
int llz_mdct_kbd(double *w, int N, double alpha);

#ifdef __cplusplus 
}
#endif  


#endif
