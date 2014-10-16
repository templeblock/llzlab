/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_sort.h 
  time    : 2013/05/25 12:42 
  author  : luolongzhi ( luolongzhi@gmail.com )

  NOTE: this source code comes from glibc, it is the classical qsort code
*/

#ifndef _LLZ_SORT_H
#define _LLZ_SORT_H 


#ifdef __cplusplus 
extern "C"
{ 
#endif  

void llz_qsort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *));


#ifdef __cplusplus 
}
#endif  





#endif
