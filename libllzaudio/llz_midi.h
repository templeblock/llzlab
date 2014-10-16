#ifndef	_LLZ_MIDI_H_
#define	_LLZ_MIDI_H_

//this is simple midi synthesis file, only one track

double      midi_to_freq (int midi);
int         freq_to_midi (double f);
double      midi_to_logf (int midi);
int         logf_to_midi (double logf);

unsigned long   llz_midi_init();
void        llz_midi_uninit(unsigned long handle);

int         llz_midi_open(unsigned long handle, char *filename, int tempo, int div);
int         llz_midi_close(unsigned long handle);

int         llz_midi_addnote(unsigned long handle, 
                             int note, int on, double duration, double amp);

#endif 
