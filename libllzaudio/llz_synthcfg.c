/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_synthcfg.c 
  time    : 2013/04/14 18:33 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "llz_synthcfg.h"

/*#define DEBUG_PRINT*/

void llz_synthcfg_init(llz_synthcfg_t *f, int sample_rate, int wavtb_len)
{
    int i;
    float freq;
    float two12;

    float scl, phs, phs_inc, sq, sq_inc;
    float lvl;

    f->sample_rate  = sample_rate;
    f->wavtb_len    = wavtb_len;
    f->freq_max     = (float)sample_rate*0.5;
    f->freq_radians = (2*M_PI)/(float)sample_rate;
    f->freq_ti      = (float)wavtb_len/sample_rate;
    f->radians_ti   = (float)wavtb_len/(2*M_PI);

    f->wavtb_phs_maxinc = 0.5*wavtb_len;      // maximum phase increment for wavetables ((float)wt_len/2)

    // Equal tempered tuning system at A4=440 (Western standard)
    // Middle C = C4 = index 48
    freq = 13.75 * pow(2.0, 0.25); // C1 = A0*2^(3/12) = 16.35159...
    two12 = pow(2.0, 1.0/12.0);    // 2^(1/12) = 1.059463094...
    for (i = 0; i < 128; i++) {
        f->tuning[i] = (float) freq;
        freq *= two12;
#ifdef DEBUG_PRINT
        printf("%d = %f\n", i, f->tuning[i]);
#endif
    }

    freq = -1200;
    for (i = 0; i <= 2400; i++) {
        f->cents[i] = (float) pow(2.0, freq/1200.0);
        freq += 1.0;
    }

    // lookup tables for non-linear panning
    scl     = sqrt(2.0) / 2.0;
    phs     = 0.0;
    phs_inc = (M_PI/2) / PAN_TBLEN;
    sq      = 0.0;
    sq_inc  = 1.0 / PAN_TBLEN;
    for (i = 0; i < PAN_TBLEN; i++) {
        f->sqrt_tb[i]    = sqrt(sq) * scl;
        f->sinquad_tb[i] = sin(phs) * scl;
        phs += phs_inc;
        sq += sq_inc;
    }

    // lookup table for centibels attenuation to amplitude
    lvl = 0;
    for (i = 0; i < MAX_AMPCB; i++) {
        f->cb_tb[i] = (float) pow(10.0, (float) lvl / -200.0);
        lvl += 1.0;
    }

}

/*
    Convert pitch index to frequency.
    The frequency table is built in the constructor based on an equal-tempered
    scale with middle C at index 48. Since the tuning array is a public member, it is
    possible to overwrite the values from anywhere in the system by updating the array
    directly. Negative pitch values are allowed but are calculated directly using
    the equation for equal-tempered tuning (f * 2^n/12)
    @param pitch pitch index
    @returns frequency in Hz
*/
float llz_pitch2freq(llz_synthcfg_t *f, int pitch_index)
{
    if (pitch_index < 0 || pitch_index > (TUNING_NUM-1))
        return f->tuning[0] * pow(2.0, (float) pitch_index / 12.0);
    return f->tuning[pitch_index];
}


/*
    Convert cents to frequency multiplier.
    Cents represent a frequency variation of 1/100 semitone
    and should range +/- one octave [-1200,+1200]. Values outside
    that range result in a direct calculation. The returned
    value can be multiplied by a base frequency to get the detuned
    frequency.
    @param c deviation in pitch in 1/100 of a semitone.
    @returns frequency multiplier
*/
float llz_get_centsmult(llz_synthcfg_t *f, int c)
{
    if (c < -1200 || c > 1200)
        return pow(2.0, (double) c / 1200);
    return f->cents[c + 1200];
}

/*
    Convert cB (centibel) of attenuation into amplitude level.
    cb = -200 log10(amp). Each bit in the sample represents ~60cB. 
    For 16-bits resolution the useful range is 0-960, 
    24 bits 0-1440, 32 bits 0-1920, etc.
    @param cb centi-bels of attenuation
    @returns linear amplitude value
*/
float llz_attencb(llz_synthcfg_t *f, int cb)
{
    if (cb <= 0)
        return 1.0;
    if (cb >= MAX_AMPCB)
        return 0.0;
    return f->cb_tb[cb];
}

