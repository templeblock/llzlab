/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_musicpitch.h 
  time    : 2012/07/16 - 2012/07/18  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_MUSCIPITCH_H
#define _LLZ_MUSCIPITCH_H

#ifdef __cplusplus 
extern "C"
{ 
#endif  

typedef struct _llz_music_event_func_t {
    void (* event_one_note)(unsigned long handle, double freq, double duration, double enrg);
} llz_music_event_func_t;


int  llz_musicpitch_get_framelen(int sample_rate);

void llz_musicpitch_rom_init();

unsigned long llz_musicpitch_init(int fs, int frame_len_in, 
                              llz_music_event_func_t *func, unsigned long h_event);
void llz_musicpitch_uninit(unsigned long handle);

void llz_musicpitch_analysis(unsigned long handle, double *sample);

void llz_musicpitch_analysis_flush(unsigned long handle);


double llz_musicpitch_getcurtime(unsigned long handle);

#ifdef __cplusplus 
}
#endif  


#endif
