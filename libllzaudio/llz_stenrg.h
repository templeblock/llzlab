#ifndef _LLZ_STENRG_H 
#define _LLZ_STENRG_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  

int llz_framelen_choice(int sample_rate);

double llz_shortenrg(double *buf, double *win, int frame_len);
double llz_shortenrg2(double *buf, int frame_len);
double llz_zrc(double *buf, int frame_len);


#ifdef __cplusplus 
}
#endif  

#endif
