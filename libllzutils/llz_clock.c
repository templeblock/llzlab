/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_timeprofile.c 
  time    : 2012/10/20 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/



#include <time.h>
#include <stdio.h>

#define LLZ_CLOCK_CNT 100

double global_time_start[LLZ_CLOCK_CNT] = {0};
double global_time_end[LLZ_CLOCK_CNT]   = {0};
double global_time_cost[LLZ_CLOCK_CNT]  = {0};

void llz_clock_start(int index)
{	
	global_time_start[index] = ((double)clock() / CLOCKS_PER_SEC);
}

void llz_clock_end(int index)
{	
	global_time_end[index] = ((double)clock() / CLOCKS_PER_SEC);
}

void llz_clock_cost(int index)
{	
	global_time_cost[index] += (global_time_end[index] - global_time_start[index]);
}

void llz_prt_time_cost(int index)
{
	printf("The number of %d index cost time is %f\n", index, global_time_cost[index]);
}








