#ifndef _LLZ_PITCH2PCM_H 
#define _LLZ_PITCH2PCM_H


#ifdef __cplusplus 
extern "C"
{ 
#endif  


#define MUSIC_NOTE_NUM 5

unsigned long llz_pitch2pcm_init(int sample_rate, int frame_len, int tempo, float power_cof);

void llz_pitch2pcm_uninit(unsigned long handle);

void llz_pitch2pcm_perpare(unsigned long handle, char note[MUSIC_NOTE_NUM], int *process_cnt);

//void llz_pitch2pcm_gen(unsigned long handle, short *sample_buf);
void llz_pitch2pcm_gen(unsigned long handle, short *sample_buf, float atten);


#ifdef __cplusplus 
}
#endif  


#endif
