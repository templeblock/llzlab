/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_sine.h 
  time    : 2013/04/14 18:33 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#ifndef _LLZ_SINE_H 
#define _LLZ_SINE_H 

#include "llz_synthcfg.h"

unsigned long llz_sine_init(llz_synthcfg_t *cfg, float freq, float duration, float amp, float phs);

void llz_sine_uninit(unsigned long handle);

int llz_sine_get_samplenum(unsigned long handle);

int llz_sine_get_sampleindex(unsigned long handle);

float llz_sine_tick(unsigned long handle);

float llz_sine_wav_frame(float *y, int len, 
                         float fs, float f, float A, float phi_start);

#endif
