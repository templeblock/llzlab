#include <stdio.h>
#include <stdlib.h>
#include "llz_stenrg.h"


static int sgn(double x)
{
	if (x >= 0)
		return 1;
	else
		return -1;
}

int llz_framelen_choice(int sample_rate)
{
    int frame_len;

	switch(sample_rate) {
	case 48000:
		frame_len = 1024;	//21ms
		break;
	case 44100:
		frame_len = 1024;	//23ms
		break;
	case 32000:
		frame_len = 1024;	//32ms
		break;
	case 24000:
		frame_len = 512;	//21ms
		break;
	case 22050:
		frame_len = 512;	//23ms
		break;
	case 16000:
		frame_len = 512;	//32ms
		break;
	case 12000:
		frame_len = 256;	//21ms
		break;
	case 11025:
		frame_len = 256;	//23ms
		break;
	case 8000:
		frame_len = 256;	//32ms
		break;
	}

    return frame_len;
}


double llz_shortenrg(double *buf, double *win, int frame_len)
{
    int i;
    double x;
    double enrg;

    enrg = 0.0;
    x = 0.0;
#if 0
    for (i = 0; i < frame_len; i++) {
        x    =  buf[i] * win[i];
        enrg += x * x;
    }
#else 
    for (i = 0; i < frame_len; i++) {
        x =  buf[i];
        x *= win[i];
        enrg += x * x;
    }

#endif

    return enrg;
}

double llz_shortenrg2(double *buf, int frame_len)
{
    int i;
    double x;
    double enrg;

    enrg = 0.0;
    x = 0.0;
    for (i = 0; i < frame_len; i++) {
        x    =  buf[i];
        enrg += x * x;
    }

    return enrg;
}

double llz_zrc(double *buf, int frame_len)
{
    int i;
    int zrc_cnt;

    zrc_cnt = 0;
    for (i = 1; i < frame_len; i++) 
        zrc_cnt += abs(sgn(buf[i]) - sgn(buf[i-1])) / 2;

    return (double)zrc_cnt / frame_len;
    /*return (double)zrc_cnt;*/
}





