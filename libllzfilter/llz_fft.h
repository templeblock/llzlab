/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_fft.h 
  time    : 2012/07/15 14:14 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#ifndef _LLZ_FFT_H
#define _LLZ_FFT_H

#ifdef __cplusplus 
extern "C"
{ 
#endif  


unsigned long llz_fft_init(int size);
void llz_fft_uninit(unsigned long handle);

void llz_fft(unsigned long handle, double *data);
void llz_ifft(unsigned long handle, double* data);

#ifdef __cplusplus 
}
#endif  


#endif
