/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_sine.c 
  time    : 2013/04/14 18:33 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "llz_sine.h"

#ifndef M_PI
#define M_PI			3.14159265358979323846
#endif

typedef struct _llz_sine_t {
    float freq;         //the real frequency derived from pitch
    float duration;     //the duration time of this pitch
    float amp;          //amplitude

    int   samples_num;  //the total sample number of the duration
    int   sample_index; //current sample index (how many samples has been generate)

    float phs;
    float phs_inc;
} llz_sine_t;

unsigned long llz_sine_init(llz_synthcfg_t *cfg, float freq, float duration, float amp, float phs)
{
    llz_sine_t *f = (llz_sine_t *)malloc(sizeof(llz_sine_t));
    memset(f, 0, sizeof(llz_sine_t));

    f->freq     = freq;
    f->duration = duration;
    f->amp      = amp;

    f->samples_num = (int)((cfg->sample_rate * duration)+0.5);
    f->sample_index= 0;

    f->phs      = phs;
    f->phs_inc  = cfg->freq_radians * freq;

    return (unsigned long)f;
}

void llz_sine_uninit(unsigned long handle)
{
    llz_sine_t *f = (llz_sine_t *)handle;

    if (f) {
        free(f);
        f = NULL;
    }
}

int llz_sine_get_samplenum(unsigned long handle)
{
    llz_sine_t *f = (llz_sine_t *)handle;

    return f->samples_num;
}

int llz_sine_get_sampleindex(unsigned long handle)
{
    llz_sine_t *f = (llz_sine_t *)handle;

    return f->sample_index;
}

float llz_sine_tick(unsigned long handle)
{
    llz_sine_t *f = (llz_sine_t *)handle;
    float val;

    val = f->amp * sin(f->phs);

    if ((f->phs+=f->phs_inc) >= (2*M_PI))
        f->phs -= (2*M_PI);

    f->sample_index += 1;

    return val;
}

static float sine_wave(float A, float f, float t, float phi)
{
    return A*sin(2*M_PI*f*t + phi);
}

/*be careful the phase*/
float llz_sine_wav_frame(float *y, int len, 
                         float fs, float f, float A, float phi_start)
{
    int i;
    float dt = 1./fs;
    float phi_last;

    /*each i represent dt*i samples, time hop is dt*/
    for (i = 0; i < len; i++) {
        y[i] =  sine_wave(A, f, dt*i, phi_start);
    }

    /*return the last phase of the last one sample*/
    phi_last = phi_start + 2*M_PI*f*dt*len;

    return phi_last;
}


