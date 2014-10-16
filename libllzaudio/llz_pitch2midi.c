#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "llz_pitch2midi.h"
#include "llz_musicpitch.h"
#include "llz_musicnote.h"


#define FRAME_SIZE_MAX  2048 
#define FILE_NAME_LEN   256

typedef struct _llz_pitch2midi_t {
    unsigned long h_musicpitch;
    unsigned long h_musicnote;
    int frame_len;
    char midi_file[FILE_NAME_LEN];
} llz_pitch2midi_t;

int llz_pitch2midi_get_framelen(int sample_rate)
{
    return llz_musicpitch_get_framelen(sample_rate);
}

unsigned long llz_pitch2midi_init(int sample_rate, char *midi_file)
{
    llz_pitch2midi_t *f = (llz_pitch2midi_t *)malloc(sizeof(llz_pitch2midi_t));
    llz_musicnote_t *s;

    memset(f, 0, sizeof(llz_pitch2midi_t));
    llz_musicpitch_rom_init();
    f->frame_len = llz_musicpitch_get_framelen(sample_rate);
    memset(f->midi_file, 0, FILE_NAME_LEN);
    strncpy(f->midi_file, midi_file, FILE_NAME_LEN);

    f->h_musicnote = llz_musicnote_init(NOTE_MAX_NUM, RECORD_INNER, NULL, NULL);
    s = (llz_musicnote_t *)(f->h_musicnote);
    f->h_musicpitch = llz_musicpitch_init(sample_rate, f->frame_len, &(s->func), f->h_musicnote);

    return (unsigned long)f;
}

void llz_pitch2midi_uninit(unsigned long handle)
{
    llz_pitch2midi_t *f = (llz_pitch2midi_t *)handle;

    if (f) {
        llz_musicnote_uninit(f->h_musicnote);
        llz_musicpitch_uninit(f->h_musicpitch);

        free(f);
        f = NULL;
    }
}

int llz_pitch2midi_analysis(unsigned long handle, unsigned char *buf_in)
{
    llz_pitch2midi_t *f = (llz_pitch2midi_t *)handle;
	double sample[FRAME_SIZE_MAX];
    short *wavsample = (short *)buf_in;
    int i;

    memset(sample, 0, FRAME_SIZE_MAX * sizeof(double));

    for (i = 0; i < f->frame_len; i++)
       sample[i] = (double)(wavsample[i] / 32768.); 
        
    llz_musicpitch_analysis(f->h_musicpitch, sample);

    return 0;
}

int llz_pitch2midi_generate_midi(unsigned long handle)
{
    llz_pitch2midi_t *f = (llz_pitch2midi_t *)handle;

    llz_musicpitch_analysis_flush(f->h_musicpitch);
    llz_musicnote_generate_midi(f->h_musicnote, f->midi_file);

    return 0;
}


