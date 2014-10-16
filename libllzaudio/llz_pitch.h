#ifndef _LLZ_PITCH_H 
#define _LLZ_PITCH_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  

enum {
    PITCH_YIN = 0,
    PITCH_METHOD_CNT,
};


unsigned long   llz_pitch_init(int frame_len, int method);
void        llz_pitch_uninit(unsigned long handle);

double      llz_pitch_getpitch(unsigned long handle, double *sample_buf);

#ifdef __cplusplus 
}
#endif  




#endif
