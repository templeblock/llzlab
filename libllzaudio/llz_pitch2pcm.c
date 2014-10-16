#include <stdio.h>
#include <stdlib.h>
#include "llz_pitch2pcm.h"
#include "llz_sine.h"


#define LLZ_MAX(a,b) ((a) > (b) ? (a) : (b))
#define LLZ_MIN(a,b) ((a) < (b) ? (a) : (b))


#define OCTAVE_CNT 10
                                 /*C       #C      D       #D      E       F       #F      G      #G      A      #A      B*/
static float octave0_freq[12] = {16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.500, 25.957, 27.5, 29.135, 30.868};
static float octave_freq[OCTAVE_CNT][12];

#define PITCH_CNT 7
static char str_pitch[PITCH_CNT][2] = {"C\0","D\0","E\0","F\0","G\0","A\0","B\0"};
static int  pitch_offset[PITCH_CNT] = { 0   , 2   , 4   , 5   , 7   , 9   , 11};

#define PITCH_POWER_MIN 0
#define PITCH_POWER_MAX 9 

/*
    e.g
    <C4062>: means C pitch, GROUP_BIG0, no ascend and desencd key, 6 duration mutiply the base speed, 2 level power
    note: the second one, 4 is the C pitch tone, commonly use
          the third one, 0 is always use, keep basis
          the fourth one, duration determined by the tempo

    continuous example
    <C4062><E4031><A4033>...
*/
typedef struct _pitch_unit_t {
    int pitch;          // (0~6) 7 base key pitch  
    int group;          // (0~9) 10 octaves
    int tune;           // (0,1) 0 is basis, 1 is half-raise key pitch
    int duration;       // multiply tempo, and get the total duration time of this music note
    int power;          // temp power

    float freq;
} pitch_unit_t;

typedef struct _llz_pitch2pcm_t {
    pitch_unit_t pu;

    int   sample_rate;
    int   frame_len;  // the frame len of the pcm sample will be generate
    int   tempo;      // base speed, the tempo*duration*t(frame_len) is the time of samples which be synthesised
    float power_cof;  // the power coffients of the key pitch power

    float phi;        // record the phase

    float *y;
    int   zero_flag;

    float *sample;
    float *win;
} llz_pitch2pcm_t;

void llz_pitch2pcm_rom_init()
{
    int i, j;
    float cof;

    cof = 1.0;
    for (i = 0; i < OCTAVE_CNT; i++) {
        for (j = 0; j < 12; j++)
            octave_freq[i][j] = octave0_freq[j] * cof;
        cof *= 2;
    }

}

unsigned long llz_pitch2pcm_init(int sample_rate, int frame_len, int tempo, float power_cof)
{
    llz_pitch2pcm_t *f = NULL;

    f = (llz_pitch2pcm_t *)malloc(sizeof(llz_pitch2pcm_t));
    memset(f, 0, sizeof(llz_pitch2pcm_t));

    f->sample_rate = sample_rate;
    f->frame_len = frame_len;
    f->tempo     = tempo;
    f->power_cof = power_cof;

    f->phi = 0;

    f->y = (float *)malloc(sizeof(float)*f->frame_len*2);
    memset(f->y, 0, sizeof(float)*f->frame_len*2);
    f->zero_flag = 0;

    f->sample = (float *)malloc(sizeof(float)*f->frame_len*2);
    memset(f->sample, 0, sizeof(float)*f->frame_len*2);

    f->win = (float *)malloc(sizeof(float)*f->frame_len*2);
    memset(f->win, 0, sizeof(float)*f->frame_len*2);
    llz_blackman(f->win, f->frame_len*2);

    return (unsigned long)f;
}

void llz_pitch2pcm_uninit(unsigned long handle)
{
    llz_pitch2pcm_t *f = (llz_pitch2pcm_t *)handle;

    if (f) {
        if (f->y) {
            free(f->y);
            f->y = NULL;
        }

        memset(f, 0, sizeof(llz_pitch2pcm_t));

        free(f);
        f = NULL;
    }
}



void llz_pitch2pcm_perpare(unsigned long handle, char note[MUSIC_NOTE_NUM], int *process_cnt)
{
    llz_pitch2pcm_t *f = (llz_pitch2pcm_t *)handle;
    int i;
    char pitch;
    int index;

#if 0 
    /*calculate freq using below 3 para*/
    f->pu.pitch = 1;
    f->pu.group = 4;
    f->pu.tune  = 0;

    /*time duration and power strengh using below 2 para*/
    f->pu.duration = 4;
    f->pu.power = 2;
#else 
    sscanf(note, "%c%1d%1d%1d%1d", &pitch, &(f->pu.group), &(f->pu.tune), &(f->pu.duration), &(f->pu.power));
    /*printf("pitch = %c, group=%d, tune=%d, duration=%d, power=%d\n", */
            /*pitch, f->pu.group, f->pu.tune, f->pu.duration, f->pu.power);*/

    if (0 == strncmp(&pitch, "Z\0", 1)) {
        f->zero_flag = 1;
    } else {
        f->zero_flag = 0;

        for (i = 0; i < PITCH_CNT; i++) {
            if (0 == strncmp(&pitch, str_pitch[i], 1))
                break;
        }
        if (i == PITCH_CNT)
            f->pu.pitch = PITCH_CNT-1;
        else
            f->pu.pitch = i;

        index = pitch_offset[f->pu.pitch] + f->pu.tune;
        if (index >= 12) {
            f->pu.group += 1;
            if (f->pu.group >= OCTAVE_CNT)
                f->pu.group = OCTAVE_CNT - 1;
            index = 0;
        }

        f->pu.freq = octave_freq[f->pu.group][index];
    }
#endif

    *process_cnt = f->pu.duration * f->tempo;
}

void llz_pitch2pcm_gen(unsigned long handle, short *sample_buf, float atten)
{
    int i;
    llz_pitch2pcm_t *f = (llz_pitch2pcm_t *)handle;
    int frame_len;
    float tmp;

    frame_len = f->frame_len;
    memset(f->y, 0, sizeof(float)*frame_len*2);

    for (i = 0; i < frame_len; i++) {
        f->sample[i] = f->sample[i+frame_len];
        f->sample[i+frame_len] = 0.;
    }

    if(f->zero_flag) {
        /*memset(sample_buf, 0, frame_len*sizeof(short));*/
        f->phi = llz_sine_wav_frame(f->y, frame_len, 
                                    f->sample_rate, f->pu.freq, 0., f->phi);

        for (i = 0; i < frame_len; i++) {
            tmp = atten * f->y[i]; // + f->sample[i];
            f->sample[i] = tmp;
        }

        for (i = 0; i < frame_len; i++) {
            tmp = f->sample[i] * 32768;
            tmp = LLZ_MIN(tmp, 32767);
            tmp = LLZ_MAX(tmp, -32768);
            sample_buf[i] = (short)tmp;
        }



    } else {

#if 0 
        f->phi = llz_sine_wav_frame(f->y, frame_len*2, 
                                    f->sample_rate, f->pu.freq, f->pu.power*f->power_cof, f->phi);

        for (i = 0; i < frame_len*2; i++) {
            tmp = atten * f->win[i] * f->y[i] + f->sample[i];
            f->sample[i] = tmp;
        }

        for (i = 0; i < frame_len; i++) {
            tmp = f->sample[i] * 32768;
            tmp = LLZ_MIN(tmp, 32767);
            tmp = LLZ_MAX(tmp, -32768);
            sample_buf[i] = (short)tmp;
        }

#else 
        f->phi = llz_sine_wav_frame(f->y, frame_len, 
                                    f->sample_rate, f->pu.freq, f->pu.power*f->power_cof, f->phi);

        for (i = 0; i < frame_len; i++) {
            tmp = atten * f->y[i]; // + f->sample[i];
            f->sample[i] = tmp;
        }

        for (i = 0; i < frame_len; i++) {
            tmp = f->sample[i] * 32768;
            tmp = LLZ_MIN(tmp, 32767);
            tmp = LLZ_MAX(tmp, -32768);
            sample_buf[i] = (short)tmp;
        }


#endif
    }

}









