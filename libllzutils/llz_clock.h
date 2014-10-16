/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_timeprofile.h 
  time    : 2012/10/20 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_CLOCK_H
#define _LLZ_CLOCK_H

#define USE_TIME

#define CLOCK_MAX_NUM  100

#ifdef USE_TIME

void llz_clock_start(int index);
void llz_clock_end(int index);
void llz_clock_cost(int index);
void llz_prt_time_cost(int index);

#define LLZ_CLOCK_START(index)      llz_clock_start(index)
#define LLZ_CLOCK_END(index)        llz_clock_end(index)
#define LLZ_CLOCK_COST(index)       llz_clock_cost(index)
#define LLZ_GET_TIME_COST(index)    llz_prt_time_cost(index) 

#else
	
#define LLZ_CLOCK_START(index) 
#define LLZ_CLOCK_END(index) 
#define LLZ_CLOCK_COST(index) 
#define LLZ_GET_TIME_COST(index)

#endif

#endif






























