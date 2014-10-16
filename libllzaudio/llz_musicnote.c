#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "llz_musicnote.h"
#include "llz_midi.h"

#define LLZ_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define LLZ_MAX(a, b)   ((a) > (b) ? (a) : (b))

static int freq2note(llz_musicnote_t *f, double freq)
{
    int i;
    int index;
    double diff1, diff2;

    index = -1;
    for (i = 1; i < TUNING_CNT; i++) {
        if (f->tuning[i] > freq) {
            diff1 = fabs(f->tuning[i] - freq);
            diff2 = fabs(freq - f->tuning[i-1]);
            if (diff1 < diff2)
                index = i;
            else 
                index = i - 1;

            break;
        }
    }

    return index;
}

static void event_one_note(unsigned long handle, double freq, double duration, double enrg)
{
    llz_musicnote_t *f = (llz_musicnote_t *)handle;
    double freq1;
    int index;
    char tmp[1024];

    index = -1;

    if (freq > 0) {
        /*freq *= 2;*/
        freq1 = freq;// * 2; //* 3;
        /*index = freq2note(f, freq1);*/
        index = freq_to_midi (freq1);
        if (-1 == index)
            freq1 = 0.0;
        else 
            freq1 = f->tuning[index];
    } else {
        freq1 = 0.0;
        freq  = 0.0;
    }

    if (f->cur_note_num <= f->note_num_max-1) {
        f->note_info[f->cur_note_num].freq     = freq;
        f->note_info[f->cur_note_num].midi     = index;
        f->note_info[f->cur_note_num].time     = f->cur_time;
        f->note_info[f->cur_note_num].duration = duration;
        f->note_info[f->cur_note_num].enrg     = enrg;
        f->cur_note_num++;
        f->cur_time += duration;

        if (index >= 0) {
            double ratio;

            f->cur_pitchnote_num++;
            f->note_duration_max = LLZ_MAX(f->note_duration_max, duration);
            f->note_duration_min = LLZ_MAX(f->note_duration_min, duration);
            f->note_duration_avg1 = (f->note_duration_avg1 + duration) / 2.;
            if (duration < 3 * f->note_duration_avg1 && duration > 0.3 * f->note_duration_avg1)
                f->note_duration_avg2 = (f->note_duration_avg2 + duration) / 2.;

            if (f->note_duration_avg2 > 0)
                ratio = duration / f->note_duration_avg2;
            else 
                ratio = 1.;

            if (ratio > 0.5 && ratio < 2)
                f->cur_pitchbeat_num += 1.0;
            else {
                if (ratio <= 0.5)
                    f->cur_pitchbeat_num += 0.5;
                if (ratio >= 2 && ratio < 4)
                    f->cur_pitchbeat_num += 2;
                if (ratio >= 4)
                    f->cur_pitchbeat_num += 3;
            }
        }

        switch (f->record_mode) {
            case RECORD_INNER_DAT:
                fwrite(&index, 1, sizeof(int), f->fp_dat);
                fwrite(&freq, 1, sizeof(double), f->fp_dat);
                fwrite(&duration, 1, sizeof(double), f->fp_dat);
                fwrite(&(f->cur_time), 1, sizeof(double), f->fp_dat);
                fwrite(&enrg, 1, sizeof(double), f->fp_dat);
                break;
            case RECORD_INNER_TXT:
                if (index >= 0) {
                    memset(tmp, 0, 1024);
                    sprintf(tmp, "[NOTE  %d][Time: %.2lf - %.2lf][Duration: %.2lf][Note: %d][Freq: %.2lf][Enrg: %.3lf]\n\r",
                            f->cur_pitchnote_num, f->cur_time, f->cur_time+duration, duration, index, freq, enrg);
                    fwrite(tmp, 1, strlen(tmp), f->fp_txt);
                }
                break;
            case RECORD_ALL:
                fwrite(&index, 1, sizeof(int), f->fp_dat);
                fwrite(&freq, 1, sizeof(double), f->fp_dat);
                fwrite(&duration, 1, sizeof(double), f->fp_dat);
                fwrite(&(f->cur_time), 1, sizeof(double), f->fp_dat);
                fwrite(&enrg, 1, sizeof(double), f->fp_dat);
                if (index >= 0) {
                    memset(tmp, 0, 1024);
                    sprintf(tmp, "[NOTE  %d][Time: %.2lf - %.2lf][Duration: %.2lf][Note: %d][Freq: %.2lf][Enrg: %.3lf]\n\r",
                            f->cur_pitchnote_num, f->cur_time, f->cur_time+duration, duration, index, freq, enrg);
                    fwrite(tmp, 1, strlen(tmp), f->fp_txt);
                }
 
                break;
        }
    }

}




unsigned long llz_musicnote_init(int note_num_max, int note_record_mode, char *note_dat_file, char *note_txt_file)
{
    int i;

    double freq;
    double two12;

    llz_musicnote_t *f = (llz_musicnote_t *)malloc(sizeof(llz_musicnote_t));
    memset(f, 0, sizeof(llz_musicnote_t));

    if (note_record_mode < 0 || note_record_mode >= RECORD_MODE_CNT)
        f->record_mode = RECORD_INNER;
    else 
        f->record_mode = note_record_mode;

    f->fp_dat = NULL;
    f->fp_txt = NULL;
    switch (f->record_mode) {
        case RECORD_INNER_DAT:
            if (note_dat_file)
                f->fp_dat= fopen(note_dat_file, "wb+");

            if (f->fp_dat == NULL) {
                free(f);
                f = NULL;

                return 0;
            }
            break;
        case RECORD_INNER_TXT:
            if (note_txt_file)
                f->fp_txt = fopen(note_txt_file, "w+");
            if (f->fp_txt == NULL) {
                free(f);
                f = NULL;

                return 0;
            }
        case RECORD_ALL:
            if (note_dat_file)
                f->fp_dat= fopen(note_dat_file, "wb+");
            if (f->fp_dat == NULL) {
                free(f);
                f = NULL;

                return 0;
            }

            if (note_txt_file)
                f->fp_txt = fopen(note_txt_file, "w+");
            if (f->fp_txt == NULL) {
                fclose(f->fp_dat);
                free(f);
                f = NULL;

                return 0;
            }
            break;
        default:
            break;
    }


    f->func.event_one_note = event_one_note;

    // Equal tempered tuning system at A4=440 (Western standard)
    // Middle C = C4 = index 48
    freq = 13.75 * pow(2.0, 0.25); // C1 = A0*2^(3/12) = 16.35159...
    two12 = pow(2.0, 1.0/12.0);    // 2^(1/12) = 1.059463094...
    for (i = 0; i < TUNING_CNT; i++) {
        f->tuning[i] = (double) freq;
        freq *= two12;
#ifdef DEBUG_PRINT
        LLZ_PRINT("%d = %f\n", i, f->tuning[i]);
#endif
    }

    f->cur_time = 0.0;

    f->note_num_max = note_num_max;
    f->note_info = (note_info_t *)malloc(note_num_max * sizeof(note_info_t));
    memset(f->note_info, 0, note_num_max * sizeof(note_info_t));
    f->cur_note_num = 0;
    f->cur_pitchnote_num = 0;

    f->h_midi = llz_midi_init();

    /*assume the 4/4 note is 0.5 s firstly*/
    f->note_duration_max  = 0.0;
    f->note_duration_min  = 100000000.0;
    f->note_duration_avg1 = 0.5;       // the real average duration
    f->note_duration_avg2 = 0.5;       // the average duration exclude the very long duration and very short duration
    f->cur_pitchbeat_num  = 0.0;

    return (unsigned long)f;
}

void llz_musicnote_uninit(unsigned long handle)
{
    llz_musicnote_t *f = (llz_musicnote_t *)handle;

    if (f) {
        if (f->fp_dat)
            fclose(f->fp_dat);
        if (f->fp_txt)
            fclose(f->fp_txt);

        llz_midi_uninit(f->h_midi);

        free(f->note_info);
        f->note_info = NULL;


        free(f);

        f = NULL;
    }
}

int llz_musicnote_get_curnotenum(unsigned long handle)
{
    llz_musicnote_t *f = (llz_musicnote_t *)handle;

    return f->cur_note_num;
}

int llz_musicnote_get_curpitchnotenum(unsigned long handle)
{
    llz_musicnote_t *f = (llz_musicnote_t *)handle;

    return f->cur_pitchnote_num;
}

int llz_musicnote_getnote(unsigned long handle, int index, note_info_t *note_info)
{
    llz_musicnote_t *f = (llz_musicnote_t *)handle;

    if (index >= 0 && index < f->cur_note_num) {
        memcpy(note_info, &(f->note_info[index]), sizeof(note_info_t));
    }

    return 0;
}


static void analysis_tempo(llz_musicnote_t *f, int *tempo, int *div)
{
    int    bpm;
    double beat_per_sec;

#if 1 
    beat_per_sec = (double)f->cur_pitchbeat_num * 1.2 / f->cur_time;
    /*beat_per_sec = (double)f->cur_pitchnote_num / f->cur_time;*/
    /*beat_per_sec = (double)f->cur_note_num / f->cur_time;*/

    bpm          = (int)   (beat_per_sec * 60);
    *tempo       = (int)   (beat_per_sec * 1000000); 
#else 
    *tempo = 500000;
#endif

    *div = 480;

    printf(">>>>>>>> avg1=%lf, avg2=%lf\n", f->note_duration_avg1, f->note_duration_avg2);
    printf(">>>>>>>> bpm=%d, tempo=%d, div=%d\n", bpm, *tempo, *div);
}

int llz_musicnote_generate_midi(unsigned long handle, char *midi_filename)
{
    int    i;
    int    note, note_prev;
    double time_cur, time_prev;

    int    tempo;
    int    div;
    double vel;

    int    note_flag;

    note_info_t note_info;

    llz_musicnote_t *f = (llz_musicnote_t *)handle;

    analysis_tempo(f, &tempo, &div);

    llz_midi_open(f->h_midi, midi_filename, tempo, div);

    time_cur     = 0.0;
    time_prev    = 0.0;

    note         = 0;
    note_prev    = 0;

    llz_midi_addnote(f->h_midi, note_prev, 1, 0, 0);

    note_flag = 0;

    for (i = 0; i < f->cur_note_num; i++) {
        llz_musicnote_getnote(handle, i, &note_info);
        note     = note_info.midi;
        time_cur = note_info.time;

        /*record current time*/
        /*printf("cur_time=%lf, end_time=%lf, note=%d, prev_note=%d, notefreq=%lf, freq = %lf, duration=%lf\n", */
                  /*time_cur, time_cur+duration, note, note_prev, midi_to_freq(note), freq, duration);*/

        vel = 1.0;

        if (note < 0) {
            if (note_flag == 1) {
                llz_midi_addnote(f->h_midi, note_prev, 1, 1000*(time_cur-time_prev), 0);
                note_flag = 0;
                time_prev    = time_cur;
            }
            continue;
        } else {
            llz_midi_addnote(f->h_midi, note_prev, 1, 1000*(time_cur-time_prev), 0);
            llz_midi_addnote(f->h_midi, note, 1, 0, vel);

            note_prev    = note;
            time_prev    = time_cur;
            note_flag    = 1;
        }
    }


    llz_midi_close(f->h_midi);

    return 0;
}





