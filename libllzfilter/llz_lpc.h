/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_lpc.h
  time    : 2012/11/17 23:52
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#ifndef _LLZ_LPC_H
#define _LLZ_LPC_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  


unsigned long llz_lpc_init(int p);
void      llz_lpc_uninit(unsigned long handle);
double llz_lpc(unsigned long handle, double *x, int x_len, double *lpc_cof, double *kcof, double *err);
 

#ifdef __cplusplus 
}
#endif  



#endif
