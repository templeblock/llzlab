/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_synthcfg.h 
  time    : 2013/04/14 18:33 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_SYNTHCFG_H 
#define _LLZ_SYNTHCFG_H


#ifdef __cplusplus 
extern "C"
{ 
#endif  


#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#define WAVTB_LEN_DEFAULT  16384

#define PAN_TBLEN 4096
#define MAX_AMPCB 1440


#define TUNING_NUM  128
#define CENTS_NUM   2401

typedef struct _llz_synthcfg_t {
    int sample_rate;             //fs, frequency sample rate

    float freq_max;              //nquist frequency, 0.5*fs=0.5*sample_rate
    float freq_radians;          //multipler for frequency to radians,  2*pi/fs
    float freq_ti;               //multipler for frequency to table index, table_len/fs
    float radians_ti;            //multipler for radians to table index, table_len/2*pi

    int   wavtb_len;             //wavtable length
 
    float tuning[TUNING_NUM];    // table to convert pitch index into frequency
    float cents[CENTS_NUM];      // table to convert cents +/-1200 into frequency multiplier

    float wavtb_phs_maxinc;      // maximum phase increment for wavetables ((float)wt_len/2)
    float sinquad_tb[PAN_TBLEN]; // first quadrant of a sine wave (used by Panner)
    float sqrt_tb[PAN_TBLEN];    // square roots of 0-1 (used by Panner)

    float cb_tb[MAX_AMPCB];

} llz_synthcfg_t;


void  llz_synthcfg_init(llz_synthcfg_t *f, int sample_rate, int wavtb_len);
float llz_pitch2freq(llz_synthcfg_t *f, int pitch_index);
float llz_get_centsmult(llz_synthcfg_t *f, int c);
float llz_attencb(llz_synthcfg_t *f, int cb);


#ifdef __cplusplus 
}
#endif  




#endif
