#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "llz_pitch.h"

typedef struct _yin_t {
    int    len;
    double *x;
    double *d;

    double tol;
} yin_t;

typedef struct _llz_pitch_t {
    int    frame_len;
    int    method;
    double *sample_buf;

    yin_t  *yin;

    double  (* getpitch)(struct _llz_pitch_t *f, double *sample_buf);
} llz_pitch_t;

int pitch_yin_init(yin_t *s, int frame_len, double tol)
{
    s->len = frame_len;

    s->x   = (double *)malloc(2 * frame_len * sizeof(double));
    if (NULL == s->x) return -1;
    memset(s->x, 0, frame_len * sizeof(double));

    s->d   = (double *)malloc(frame_len * sizeof(double));
    if (NULL == s->d) return -1;
    memset(s->d, 0, frame_len * sizeof(double));

    s->tol = tol;

    return 0;
}

void pitch_yin_uninit(yin_t *s)
{
    if (s) {
        free(s->x);
        s->x = NULL;

        free(s->d);
        s->d = NULL;
    }
}

double quadfrac(double s0, double s1, double s2, double pf) {
  double tmp = s0 + (pf/2.) * (pf * ( s0 - 2.*s1 + s2 ) - 3.*s0 + 4.*s1 - s2);
  return tmp;
}

double quadint_min(double * data, int len, int pos, int span) 
{
#if 1 
    double real_pos;

    real_pos = (double)pos;

    return (double)real_pos;

#else
    double step = 1./200.;
    /* init resold to - something (in case x[pos+-span]<0)) */
    double res, frac, s0, s1, s2, exactpos = (double)pos, resold = 100000.;
    if ((pos > span) && (pos < len-span)) {
        s0 = data[pos-span];
        s1 = data[pos]     ;
        s2 = data[pos+span];
        /* increase frac */
        for (frac = 0.; frac < 2.; frac = frac + step) {
            res = quadfrac(s0, s1, s2, frac);
            if (res < resold) {
                resold = res;
            } else {				
                exactpos += (frac-step)*span - span/2.;
                break;
            }
        }
    }

    return exactpos;
#endif
}





double pitch_yin_getpitch(llz_pitch_t *f, double *sample_buf)
{
    int     j;
    int     tau = 0;
    int     period = 0;
    int     frame_len;

    double  diff;
    double  cum;

    int     min_pos;
    double  tmp;

    yin_t *s = f->yin;
    frame_len = s->len;

    for (j = 0; j < frame_len; j++) {
        s->x[j]             = s->x[j + frame_len];
        s->x[j + frame_len] = sample_buf[j];
    }

    memset(s->d, 0, frame_len * sizeof(double));

    s->d[0] = 1;
    cum     = 0.0;
    for (tau = 1; tau < frame_len; tau++) {
        s->d[tau] = 0.0;

        for (j = 0; j < frame_len; j++) {
            diff = s->x[j] - s->x[j+tau];
            s->d[tau] += diff * diff;
        }
        cum += s->d[tau];
        s->d[tau] *= tau / cum;

        period = tau - 3;
        if (tau >  3 && 
            s->d[period] < s->tol &&
            s->d[period] < s->d[period+1])

            return quadint_min(s->d, s->len, period, 1);
    }

    min_pos = 0;
    tmp     = s->d[0];
    for (j = 0; j < frame_len; j++) {
        if (s->d[j] < tmp) {
            tmp     = s->d[j];
            min_pos = j;
        }
    }

    return quadint_min(s->d, s->len, min_pos, 1);
}




unsigned long  llz_pitch_init(int frame_len, int method)
{
    llz_pitch_t *f = (llz_pitch_t *)malloc(sizeof(llz_pitch_t));

    if (NULL == f)
        return 0;

    memset(f, 0, sizeof(llz_pitch_t));

    f->getpitch = NULL;
    f->frame_len = frame_len;

    switch (method) {
        case PITCH_YIN:
            f->method = method;
            f->yin    = (yin_t *)malloc(sizeof(yin_t));
            pitch_yin_init(f->yin, frame_len, 0.1);
            f->getpitch = pitch_yin_getpitch;
            break;
        default:
            f->method = PITCH_YIN;

    }

    f->method    = method;

    return (unsigned long)f;
}

void llz_pitch_uninit(unsigned long handle)
{
    llz_pitch_t *f = (llz_pitch_t *)handle;

    if (f) {
        switch (f->method) {
            case PITCH_YIN:
                pitch_yin_uninit(f->yin);
                free(f->yin);
                f->yin = NULL;
                break;
        }

        free(f);
        f = NULL;
    }
}

double llz_pitch_getpitch(unsigned long handle, double *sample_buf)
{
    llz_pitch_t *f = (llz_pitch_t *)handle;

    if (f->getpitch) 
        return f->getpitch(f, sample_buf);

    return 0;
}



