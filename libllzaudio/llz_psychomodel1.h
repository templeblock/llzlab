/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_psychomodel1.h 
  time    : 2012/07/16 - 2012/07/18  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_PSYCHOMODEL_H
#define _LLZ_PSYCHOMODEL_H

#ifdef __cplusplus 
extern "C"
{ 
#endif  


#define CBANDS_NUM        25

void llz_psychomodel1_rom_init();

unsigned long llz_psychomodel1_init(int fs, int fft_len);
void llz_psychomodel1_uninit(unsigned long handle);
void llz_psy_global_threshold(unsigned long handle, double *fft_buf, double *gth);
void llz_psy_global_threshold_usemdct(unsigned long handle, double *mdct_buf, double *gth);
void llz_psychomodel1_get_gth(unsigned long handle, double *gth);


#ifdef __cplusplus 
}
#endif  


#endif
