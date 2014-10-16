#ifndef _LLZ_MUSICNOTE_H 
#define _LLZ_MUSICNOTE_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  

#include "llz_musicpitch.h"

#define NOTE_MAX_NUM 6000
#define TUNING_CNT   128

enum {
    RECORD_INNER = 0,
    RECORD_INNER_DAT,
    RECORD_INNER_TXT,
    RECORD_ALL,
    RECORD_MODE_CNT,
};

typedef struct _note_info_t {
    double freq;
    int    midi;

    double time;
    double duration;
    double enrg;
} note_info_t;

typedef struct _llz_musicnote_t {
    //the musicpitch detect call back func
    llz_music_event_func_t  func;

    //record mode
    int                     record_mode;
    FILE                    * fp_dat;
    FILE                    * fp_txt;

    double                  tuning[TUNING_CNT];        // table to convert pitch index into frequency
    double                  cur_time;

    //note info
    int                     note_num_max;
    note_info_t             * note_info;
    int                     cur_note_num;
    int                     cur_pitchnote_num;

    unsigned long               h_midi;

    double                  note_duration_max;
    double                  note_duration_min;
    double                  note_duration_avg1;       // the real average duration
    double                  note_duration_avg2;       // the average duration exclude the very long duration and very short duration
    double                  cur_pitchbeat_num;
} llz_musicnote_t;


unsigned long llz_musicnote_init(int note_num_max, int note_record_mode, char *note_dat_file, char *note_txt_file);
void          llz_musicnote_uninit(unsigned long handle);


int  llz_musicnote_get_curnotenum(unsigned long handle);
int  llz_musicnote_get_curpitchnotenum(unsigned long handle);
int  llz_musicnote_getnote(unsigned long handle, int index, note_info_t *note_info);

int  llz_musicnote_generate_midi(unsigned long handle, char *midi_filename);


#ifdef __cplusplus 
}
#endif  


#endif
