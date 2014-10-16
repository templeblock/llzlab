#ifndef _LLZ_PITCH2MIDI_H 
#define _LLZ_PITCH2MIDI_H 


int llz_pitch2midi_get_framelen(int sample_rate);

unsigned long llz_pitch2midi_init(int sample_rate, char *midi_file);
void llz_pitch2midi_uninit(unsigned long handle);

int llz_pitch2midi_analysis(unsigned long handle, unsigned char *buf_in); 

int llz_pitch2midi_generate_midi(unsigned long handle);

#endif
