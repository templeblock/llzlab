/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_levinson.h 
  time    : 2012/11/17 15:19 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#ifndef _LLZ_LEVINSON_H
#define _LLZ_LEVINSON_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  

#define LLZ_LEVINSON_ORDER_MAX   64

void llz_levinson(double *r,    int p, 
                  double *acof, double *kcof, double *err);
void llz_levinson1(double *r, int p, 
                   double *acof, double *kcof, double *err);
int  llz_atlvs(double *r, int n, double *b, 
               double *x, double *kcof, double *err);

#ifdef __cplusplus 
}
#endif  



#endif
