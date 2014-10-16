/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_musicpitch.c 
  time    : 2012/07/16 - 2012/07/18  
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <assert.h>
#include "llz_musicpitch.h"
#include "llz_fft.h"
#include "llz_fir.h"
#include "llz_iir.h"
#include "llz_sort.h"
#include "llz_stenrg.h"
#include "llz_pitch.h"

#define  OVERLAP_COF   2 

#define CBANDS_NUM        25

#ifndef LLZ_PRINT 
#define LLZ_PRINT(...)     
/*#define LLZ_PRINT       printf*/
#define LLZ_PRINT_ERR   LLZ_PRINT
#define LLZ_PRINT_DBG   LLZ_PRINT
#endif

#define LLZ_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define LLZ_MAX(a, b)   ((a) > (b) ? (a) : (b))

static int nm_pos_geomean(int l, int u);

/*this psd normalizer is for the SPL estimation */
/*#define SAMPLE16*/
#ifdef SAMPLE16 
#define PSD_NORMALIZER  0  
#else
#define PSD_NORMALIZER  90.302
#endif

typedef struct _llz_psychomodel1_t {
    double fs;
    int   fft_len;
    int   psd_len;

    double *psd;
    double *psd_ath;
    double *psd_bark;
    double *ptm;
    double *pnm;
    int   *tone_flag;


    double *spread_effect;
    double *tone_thres;
    double *noise_thres;
    double *global_thres;

    int   cb_hopbin[CBANDS_NUM];
    double geomean_table[CBANDS_NUM];
    int   splnum1;
    int   splnum2;

}llz_psychomodel1_t;

#define DB_FAST

#define MAXDB  (300)
#define MINDB  (-300)
#define DBCNT  (MAXDB-MINDB+1)
static double db_table[DBCNT];

static void psy_dbtable_init()
{
    int i;
    double db;

    for (i = 0; i < DBCNT; i++) {
        db = (double)(i + MINDB);
        db_table[i] = pow(10, 0.1*db);
    }
}

static double db_pos(double power)
{
#if 0 //def DB_FAST
    int i;

    for (i = 0; i < DBCNT; i++) {
        if (power < db_table[i]) 
            return (i+MINDB);
    }

    return (DBCNT-1);

#else 
    return 10*log10(power);
#endif
}

/*#define TEST_MAXMINDB*/

#ifdef TEST_MAXMINDB
static double dbmax=-300.;
static double dbmin= 1000;
#endif

static double db_inv(double db)
{
#ifdef TEST_MAXMINDB
    /*printf("db=%f\n", db);*/
    if (db > dbmax)
        dbmax = db;
    if (db < dbmin)
        dbmin = db;
#endif

#ifdef DB_FAST
    int i;

    i = (int)db;

    return db_table[i-MINDB];
#else 
    return pow(10, 0.1*db);
#endif
}

/*input fft product(re,im)*/
static double psd_estimate(double re, double im)
{
    double power;
    double psd;

    power = re * re + im * im;
    /*use PSD_NORMALIZER to transform is to SPL estimation in dB*/
    /*psd = PSD_NORMALIZER + db_pos(power/64);*/
    psd = PSD_NORMALIZER + db_pos(power);

    return psd;
}

static double psd_estimate2(double re, double im, double *power_out)
{
    double power;
    double psd;

    power = re * re + im * im;
    /*use PSD_NORMALIZER to transform is to SPL estimation in dB*/
    /*psd = PSD_NORMALIZER + db_pos(power/64);*/
    psd = PSD_NORMALIZER + db_pos(power);

    *power_out = power;

    return psd;
}




static double psd_estimate_usemdct(double mdct_line, double cof)
{
    double power;
    double psd;

    power = (mdct_line * mdct_line+1)/cof;
    /*use PSD_NORMALIZER to transform is to SPL estimation in dB*/
    psd = PSD_NORMALIZER + db_pos(power);

    return psd;
}



/*
 * input: freq in hz  
 * output: barks 
 */
static double freq2bark(double freq)
{
    if (freq < 0)
        freq = 0;
    freq = freq * 0.001;
    return 13.0 * atan(.76 * freq) + 3.5 * atan(freq * freq / (7.5 * 7.5));
}

/*cover f(Hz) to SPL(dB)*/
static double ath(double f)
{
    double ath;

    f /= 1000.;
 
    if (f > 0.0)
        ath = 3.64 * pow(f, -0.8) - 6.5 * exp(-0.6 * pow(f-3.3, 2)) + 0.001 * pow(f,4);
    else  {
        f = 0.01;
        ath = 3.64 * pow(f, -0.8) - 6.5 * exp(-0.6 * pow(f-3.3, 2)) + 0.001 * pow(f,4);
    }

    return ath;
}


/*freqbin = 0 means direct constant component*/
static double freqbin2freq(double fs, int fft_len, int freqbin)
{
    double delta_f;
    double f;

    delta_f = fs / fft_len; 

    f = freqbin * delta_f;

    return f;
}

static int freq2freqbin(double fs, int fft_len, double f)
{
    double delta_f;
    double freqbin;

    delta_f = fs / fft_len; 

    freqbin = f/delta_f;

    return freqbin;
}

static int caculate_cb_info(double fs, int psd_len, 
                            double *psd_ath, int cb_hopbin[CBANDS_NUM], double geomean_table[CBANDS_NUM],
                            double *psd_bark)
{
    int   i;
    int   k;
    double f;
    int   band;

    band = 0;
    memset(cb_hopbin, 0, sizeof(int)*CBANDS_NUM);
    for (i = 0; i < CBANDS_NUM; i++)
        geomean_table[i] = 1;

    for(k = 0; k < psd_len; k++) {
        f = freqbin2freq(fs, psd_len*2, k);
        psd_ath[k]  = ath(f);
        /*LLZ_PRINT("psd_ath[%d]=%f\n", k, psd_ath[k]);*/
        psd_bark[k] = freq2bark(f);

        if((floor(psd_bark[k]) > band) &&
           (band < CBANDS_NUM)){
            cb_hopbin[band] = k;
            /*LLZ_PRINT("the hopbin of band[%d] = %d\n", band, k);*/
            if (band == 0)
                geomean_table[band] = nm_pos_geomean(1, cb_hopbin[band])-1;
            else 
                geomean_table[band] = nm_pos_geomean(cb_hopbin[band-1], cb_hopbin[band])-1;
            band++;
        }

        /*LLZ_PRINT("the %d freqbin's bark value psd_bark[%d] = %f\n", k, k, psd_bark[k]);*/
    }

    /*exit(0);*/
    return band;

}

/*fs: Hz*/
static int deltak_splitenum(double fs, int fft_len, int *splnum1, int *splnum2)
{
    int freqbin1, freqbin2;
/*
    deltak = 2     (0.17-5.5kHz)
             [2 3] (5.5-11kHz)
             [2 6] (11-max kHz)
*/
    freqbin1 = freq2freqbin(fs, fft_len, 5500);
    freqbin2 = freq2freqbin(fs, fft_len, 11000);

    *splnum1 = freqbin1;
    *splnum2 = freqbin2;

    return 0;
}

/*
    psd[0 : psd_len-1]
    St = P(k) {P(k) > P(k+), P(k) > P(k-1), P(k) > P(k+deltak)+7, P(k) > P(k-deltak)+7}
    deltak = 2     freq=[0.17~5.5kHz]
             [2 3] freq=[5.5~11kHz]
             [2 6] freq=[11 fmax]
    for example:
        fft_len=512, psd_len=512/2=256
        deltak = 2     2  <  k < 63
                 [2 3] 63 <= k < 127
                 [2 6] 127<= k <=256
 */
static int istone(double *psd, int psd_len, int freqbin, 
                  int splnum1, int splnum2, int *tone_flag)
{
    int i;
    int kr, kl;
    /*neighbour psd*/
    double psd_nr[6], psd_nl[6];
   
    if(freqbin == 0 || freqbin >= (psd_len-6))
        goto not_tone;

    /*0 means very low, minima*/
    for(i = 0; i < 6; i++) {
        kr = freqbin + (i + 1);
        kl = freqbin - (i + 1);
        if(kr < 0 || kr >= psd_len)
            psd_nr[i] = 0.0;
        else 
            psd_nr[i] = psd[kr];

        if(kl < 0 || kl >= psd_len)
            psd_nl[i] = 0.0;
        else 
            psd_nl[i] = psd[kl];
    }

    if(psd[freqbin] < psd_nr[0] || psd[freqbin] < psd_nl[0]) {
        goto not_tone;
    }else if(freqbin > 1 && freqbin < splnum1) {          //0.17~5.5 kHz
        if(psd[freqbin] <= (psd_nr[1]+7) || psd[freqbin] <= (psd_nl[1]+7))
            goto not_tone;
        tone_flag[freqbin] = 1;
        tone_flag[freqbin-1] = tone_flag[freqbin+1] = 1;
        tone_flag[freqbin-2] = tone_flag[freqbin+2] = 1;
    }else if(freqbin >= splnum1 && freqbin < splnum2) {   //5.5~11   kHz
        for(i = 1; i <= 2; i++) {
            if(psd[freqbin] <= (psd_nr[i]+7) || psd[freqbin] <= (psd_nl[i]+7))
                goto not_tone;
        }
        tone_flag[freqbin] = 1;
        tone_flag[freqbin-1] = tone_flag[freqbin+1] = 1;
        tone_flag[freqbin-2] = tone_flag[freqbin+2] = 1;
        tone_flag[freqbin-3] = tone_flag[freqbin+3] = 1;
    }else if(freqbin >= splnum2 && freqbin < psd_len){                                                 //11~max   kHz
        for(i = 1; i <= 5; i++) {
            if(psd[freqbin] <= (psd_nr[i]+7) || psd[freqbin] <= (psd_nl[i]+7))
                goto not_tone;
        }
        tone_flag[freqbin] = 1;
        tone_flag[freqbin-1] = tone_flag[freqbin+1] = 1;
        tone_flag[freqbin-2] = tone_flag[freqbin+2] = 1;
        tone_flag[freqbin-3] = tone_flag[freqbin+3] = 1;
        tone_flag[freqbin-4] = tone_flag[freqbin+4] = 1;
        tone_flag[freqbin-5] = tone_flag[freqbin+5] = 1;
        tone_flag[freqbin-6] = tone_flag[freqbin+6] = 1;
    }else {
        goto not_tone;
    }

    return 1;
not_tone:
    return 0;
}

#define TONE_THR  6 //7 

static int ismusictone(double *psd, int psd_len, int freqbin, 
                       int splnum1, int splnum2, int *tone_flag)
{
    int i;
    int kr, kl;
    /*neighbour psd*/
    double psd_nr[6], psd_nl[6];
   
    if(freqbin == 0 || freqbin >= (psd_len-6))
        goto not_tone;

    /*0 means very low, minima*/
    for(i = 0; i < 6; i++) {
        kr = freqbin + (i + 1);
        kl = freqbin - (i + 1);
        if(kr < 0 || kr >= psd_len)
            psd_nr[i] = 0.0;
        else 
            psd_nr[i] = psd[kr];

        if(kl < 0 || kl >= psd_len)
            psd_nl[i] = 0.0;
        else 
            psd_nl[i] = psd[kl];
    }

    if(psd[freqbin] < psd_nr[0] || psd[freqbin] < psd_nl[0]) {
        goto not_tone;
    }else if(freqbin > 1 && freqbin < splnum1) {          //0.17~5.5 kHz
#if 1 
        if(psd[freqbin] <= (psd_nr[1]+TONE_THR) || psd[freqbin] <= (psd_nl[1]+TONE_THR))
            goto not_tone;
#else 
        if(psd[freqbin] <= (psd_nr[1]+4) || psd[freqbin] <= (psd_nl[1]+4))
            goto not_tone;
        if(psd[freqbin] <= (psd_nr[2]+5) || psd[freqbin] <= (psd_nl[1]+5))
            goto not_tone;
        if(psd[freqbin] <= (psd_nr[3]+6) || psd[freqbin] <= (psd_nl[1]+6))
            goto not_tone;
#endif
        tone_flag[freqbin] = 1;
    }else if(freqbin >= splnum1 && freqbin < splnum2) {   //5.5~11   kHz
        for(i = 1; i <= 2; i++) {
            if(psd[freqbin] <= (psd_nr[i]+TONE_THR) || psd[freqbin] <= (psd_nl[i]+TONE_THR))
                goto not_tone;
        }
        tone_flag[freqbin] = 1;
    }else if(freqbin >= splnum2 && freqbin < psd_len){                                                 //11~max   kHz
        for(i = 1; i <= 5; i++) {
            if(psd[freqbin] <= (psd_nr[i]+TONE_THR) || psd[freqbin] <= (psd_nl[i]+TONE_THR))
                goto not_tone;
        }
        tone_flag[freqbin] = 1;
    }else {
        goto not_tone;
    }

    return 1;
not_tone:
    return 0;
}




/*caculate ptm*/
static int psd_tonemasker(double *psd , int psd_len, 
                          int splnum1, int splnum2, 
                          double *ptm , int *tone_flag)
{
    int k;

    memset(ptm, 0, sizeof(double)*psd_len);
    memset(tone_flag, 0, sizeof(int)*psd_len);

    for(k = 0; k < psd_len; k++) {
        if(istone(psd, psd_len, k, splnum1, splnum2, tone_flag)) {
            ptm[k] = db_pos(db_inv(psd[k-1]) + db_inv(psd[k]) + db_inv(psd[k+1]));
            LLZ_PRINT("ptm[%d]=%lf, psd[k]=%lf\n", k, ptm[k], psd[k]);
        }
    }

    return 0;
}

static int nm_pos_geomean(int l, int u)
{
    int i;
    int n;
    double tmp;  //must be double, if use double(maybe not enough to store tmp)
    int pos;

    tmp = 1;
    n   = u - l + 1;

    for(i = l; i <=u; i++) 
        tmp *= i;

    pos = floor(pow(tmp, 1./n));

    return pos;

}

static int noisemasker_band(double *psd, int *tone_flag, 
                            int lowbin, int highbin,
                            double *nm , int *nm_pos)
{
    int k;
    double tmp;

    *nm  = 0;
    tmp = 0;
    for(k = lowbin; k <= highbin; k++) {
        if(!tone_flag[k]) {
            tmp += db_inv(psd[k]);
        }
    }

    *nm     = db_pos(tmp);
    *nm_pos = nm_pos_geomean(lowbin+1, highbin+1)-1;

    return 0;
}


static int noisemasker_band_fast(double *psd, int *tone_flag, 
                                 int lowbin, int highbin,
                                 int band, double *geomean_table,
                                 double *nm , int *nm_pos)
{
    int k;
    double tmp;

    *nm  = 0;
    tmp = 0;
    for(k = lowbin; k <= highbin; k++) {
        if(!tone_flag[k]) {
            tmp += db_inv(psd[k]);
        }
    }

#if 1 
    *nm = db_pos(tmp);
#else 
    if (tmp > 0)
        *nm = db_pos(tmp);
    else 
        *nm = 0;
#endif

    /**nm_pos = nm_pos_geomean(lowbin+1, highbin+1)-1;*/
    *nm_pos = geomean_table[band]; //nm_pos_geomean(lowbin+1, highbin+1)-1;

    return 0;
}



static int psd_noisemasker(double *psd, int psd_len,
                    int *tone_flag, int *cb_hopbin, 
                    double *geomean_table,
                    double *pnm)
{
    int band;
    int lowbin, highbin;
    double nm; 
    int   nm_pos;

    lowbin = 0;
    memset(pnm, 0, sizeof(double)*psd_len);
    for(band = 0; band < CBANDS_NUM; band++) {
        nm_pos = 0;
        if(cb_hopbin[band] == 0)
            break;

        highbin = cb_hopbin[band] - 1; 
        /*noisemasker_band(psd, tone_flag, lowbin, highbin, &nm, &nm_pos);*/
        noisemasker_band_fast(psd, tone_flag, lowbin, highbin, band, geomean_table, &nm, &nm_pos);
        lowbin  = highbin + 1;
        pnm[nm_pos] = nm;
        /*LLZ_PRINT("pnm[%d]=%f\n", nm_pos, pnm[nm_pos]);*/
    }

    return 0;
}

static int ptm_pnm_filter_by_ath(double *ptm, double *pnm, double *psd_ath, int psd_len)
{
    int k;

    for(k = 0; k < psd_len; k++) {
        if(ptm[k] < psd_ath[k])
            ptm[k] = 0.;
        if(pnm[k] < psd_ath[k])
            pnm[k] = 0.;
    }

    return 0;
}

static int psd_bark2bin(double *psd_bark, int psd_len, double bark)
{
    int k;

    if(bark <= 0)
        return 0;

    if(bark >= CBANDS_NUM)
        return psd_len-1;

    for(k = 1; k < psd_len; k++) {
        if(psd_bark[k] > bark)
            return k-1;
    }

    return psd_len-1;
}

static int check_near_masker(double *ptm, double *pnm, 
                             double *psd_ath, double *psd_bark,
                             int   psd_len)
{
    int k, j;
    int tone_found, noise_found;
    double masker_bark;
    double bw_low_bark, bw_high_bark;
    int   bw_low_bin, bw_high_bin;


    ptm_pnm_filter_by_ath(ptm, pnm, psd_ath, psd_len);


    for(k = 0; k < psd_len; k++) {
        tone_found  = 0;
        noise_found = 0;

        if(ptm[k] > 0)
            tone_found = 1;

        if(pnm[k] > 0)
            noise_found = 1;

        if(tone_found || noise_found) {
            masker_bark = psd_bark[k];
            bw_low_bark = masker_bark - 0.5;
            bw_high_bark= masker_bark + 0.5;

            bw_low_bin  = psd_bark2bin(psd_bark, psd_len, bw_low_bark) + 1;
            bw_high_bin = psd_bark2bin(psd_bark, psd_len, bw_high_bark);

            for(j = bw_low_bin; j <= bw_high_bin; j++) {
                if(tone_found) {
                    if(j != k && ptm[k] < ptm[j]) {
                        ptm[k] = 0;
                        break;
                    }else if(j != k){
                        ptm[j] = 0;
                    }else {
                        ;
                    }

                    if(ptm[k] < pnm[j]) {
                        ptm[k] = 0;
                        break;
                    }else {
                        pnm[j] = 0;
                    }
                }else if(noise_found) {
                    if(j != k && pnm[k] < pnm[j]) {
                        pnm[k] = 0;
                        break;
                    }else if(j != k){
                        pnm[j] = 0;
                    }else {
                        ;
                    }

                    if(pnm[k] < ptm[j]) {
                        pnm[k] = 0;
                        break;
                    }else {
                        ptm[j] = 0;
                    }
                }else{
                    ;
                }
            }

        }

    }

    return 0;
}


static int spread_function(double power, double *psd_bark,  
                           int masker_bin, int low_bin, int high_bin,
                           double *spread_effect)
{
    int   i;
    double masker_bark;
    double maskee_bark;
    double delta_bark;

    masker_bark = psd_bark[masker_bin]; 

    for(i = low_bin; i <= high_bin; i++) {
        maskee_bark = psd_bark[i];
        delta_bark  = maskee_bark - masker_bark;

        if(delta_bark >= -3.5 && delta_bark < -1)
            spread_effect[i] = 17*delta_bark - 0.4*power + 11;
        else if(delta_bark >= -1 && delta_bark < 0)
            spread_effect[i] = (0.4*power + 6.)*delta_bark;
        else if(delta_bark >= 0 && delta_bark < 1)
            spread_effect[i] = -17*delta_bark;
        else if(delta_bark >= 1 && delta_bark < 8.5)
            spread_effect[i] = (0.15*power-17)*delta_bark - 0.15*power;
        else 
            ;
    }

    return 0;

}

static int tone_mask_threshold(double *ptm, double *psd_bark, 
                               double *spread_effect, double *tone_thres, int psd_len)
{
    int   k, j;
    double masker_bark;
    double low_bark, high_bark;
    int   low_bin , high_bin;

    memset(tone_thres, 0, sizeof(double)*psd_len);

    for(k = 0; k < psd_len; k++) {
        memset(spread_effect, 0, sizeof(double)*psd_len);
        if(ptm[k] > 0) {
            masker_bark = psd_bark[k];
            low_bark    = masker_bark - 3;
            high_bark   = masker_bark + 8;
            low_bin     = psd_bark2bin(psd_bark, psd_len, low_bark);
            high_bin    = psd_bark2bin(psd_bark, psd_len, high_bark);

            spread_function(ptm[k], psd_bark, k, low_bin, high_bin, spread_effect);
            for(j = low_bin; j <= high_bin; j++)
                tone_thres[j] += db_inv(ptm[k] - 0.275*masker_bark + spread_effect[j] - 6.025);
        }

    }

    return 0;
}

static int noise_mask_threshold(double *pnm, double *psd_bark, 
                                double *spread_effect, double *noise_thres, int psd_len)
{
    int   k, j;
    double masker_bark;
    double low_bark, high_bark;
    int   low_bin , high_bin;

    memset(noise_thres, 0, sizeof(double)*psd_len);

    for(k = 0; k < psd_len; k++) {
        memset(spread_effect, 0, sizeof(double)*psd_len);
        if(pnm[k] > 0) {
            masker_bark = psd_bark[k];
            low_bark    = masker_bark - 3;
            high_bark   = masker_bark + 8;
            low_bin     = psd_bark2bin(psd_bark, psd_len, low_bark);
            high_bin    = psd_bark2bin(psd_bark, psd_len, high_bark);

            spread_function(pnm[k], psd_bark, k, low_bin, high_bin, spread_effect);
            for(j = low_bin; j <= high_bin; j++)
                noise_thres[j] += db_inv(pnm[k] - 0.175*masker_bark + spread_effect[j] - 2.025);
        }

    }

    return 0;
}

/*this valued by dB*/
static int global_threshold(double *tone_thres, double *noise_thres, double *psd_ath,
                            double *global_thres, int psd_len)
{
    int k;

    for(k = 0; k < psd_len; k++) 
        global_thres[k] = db_pos(tone_thres[k] + noise_thres[k] + db_inv(psd_ath[k]));

    return 0;
}

/*this valued by magnitude*/
static int global_threshold_mag(double *tone_thres, double *noise_thres, double *psd_ath,
                                double *global_thres, int psd_len)
{
    int k;

    for(k = 0; k < psd_len; k++) 
        global_thres[k] = (tone_thres[k] + noise_thres[k] + db_inv(psd_ath[k]));

    return 0;
}

static void llz_psychomodel1_rom_init()
{
    psy_dbtable_init();
}

static unsigned long llz_psychomodel1_init(int fs, int fft_len)
{
    int psd_len;
    llz_psychomodel1_t *f = NULL;

    f = (llz_psychomodel1_t *)malloc(sizeof(llz_psychomodel1_t));
    psd_len = fft_len >> 1;

    f->fs      = fs;
    f->fft_len = fft_len;
    f->psd_len = psd_len;

    f->psd        = (double *)malloc(sizeof(double)*psd_len);
    f->psd_ath    = (double *)malloc(sizeof(double)*psd_len);
    f->psd_bark   = (double *)malloc(sizeof(double)*psd_len);
    f->ptm        = (double *)malloc(sizeof(double)*psd_len);
    f->pnm        = (double *)malloc(sizeof(double)*psd_len);
    f->tone_flag  = (int    *)malloc(sizeof(int)*psd_len);

    f->spread_effect = (double *)malloc(sizeof(double)*psd_len);
    f->tone_thres    = (double *)malloc(sizeof(double)*psd_len);
    f->noise_thres   = (double *)malloc(sizeof(double)*psd_len);
    f->global_thres  = (double *)malloc(sizeof(double)*psd_len);


    deltak_splitenum(fs, fft_len, &f->splnum1, &f->splnum2);

    caculate_cb_info(fs, psd_len, f->psd_ath, f->cb_hopbin, f->geomean_table, f->psd_bark);


    return (unsigned long)f;
}

static void llz_psychomodel1_uninit(unsigned long handle)
{
    llz_psychomodel1_t *f = (llz_psychomodel1_t *)handle;

    if(f) {
        if(f->psd) {
            free(f->psd);
            f->psd = NULL;
        }
        if(f->psd_ath) {
            free(f->psd_ath);
            f->psd_ath = NULL;
        }
        if(f->psd_bark) {
            free(f->psd_bark);
            f->psd_bark = NULL;
        }
        if(f->ptm) {
            free(f->ptm);
            f->ptm = NULL;
        }
        if(f->pnm) {
            free(f->pnm);
            f->pnm = NULL;
        }
        if(f->tone_flag) {
            free(f->tone_flag);
            f->tone_flag = NULL;
        }
        if(f->spread_effect) {
            free(f->spread_effect);
            f->spread_effect = NULL;
        }
        if(f->tone_thres) {
            free(f->tone_thres);
            f->tone_thres = NULL;
        }
        if(f->noise_thres) {
            free(f->noise_thres);
            f->noise_thres = NULL;
        }
        if(f->global_thres) {
            free(f->global_thres);
            f->global_thres = NULL;
        }

        free(f);
        f = NULL;
    }

}

static void llz_psy_global_threshold(unsigned long handle, double *fft_buf, double *gth)
{
    int k;
    double re, im;
    llz_psychomodel1_t *f = (llz_psychomodel1_t *)handle;

    /*step1: psd estimate and normalize to SPL*/
    for(k = 0; k < f->psd_len; k++) {
        re = fft_buf[k+k];
        im = fft_buf[k+k+1];
        f->psd[k] = psd_estimate(re, im);
    }
    
    /*step2: tone and noise masker estimate*/
    psd_tonemasker(f->psd, f->psd_len, f->splnum1, f->splnum2, f->ptm, f->tone_flag);
    /*psd_noisemasker(f->psd, f->psd_len, f->tone_flag, f->cb_hopbin, f->pnm);*/
    psd_noisemasker(f->psd, f->psd_len, f->tone_flag, f->cb_hopbin, f->geomean_table, f->pnm);

    /*step3: check near masker, merge it if near*/
    check_near_masker(f->ptm, f->pnm, f->psd_ath, f->psd_bark, f->psd_len);

    /*step4: caculate individule threshold (tone, noise)*/
    tone_mask_threshold(f->ptm, f->psd_bark, f->spread_effect, f->tone_thres, f->psd_len);
    noise_mask_threshold(f->pnm, f->psd_bark, f->spread_effect, f->noise_thres, f->psd_len);

    /*step5: caculate global threshold*/
    global_threshold(f->tone_thres, f->noise_thres, f->psd_ath, f->global_thres, f->psd_len);
    memcpy(gth, f->global_thres, sizeof(double)*f->psd_len);

}

static void llz_psy_global_threshold_usemdct(unsigned long handle, double *mdct_buf, double *gth)
{
    int k;
    double re;
    llz_psychomodel1_t *f = (llz_psychomodel1_t *)handle;
    double cof;

#ifdef TEST_MAXMINDB
    dbmax = -300.;
    dbmin = 1000;
#endif

    /*cof = (100. *  (double)f->psd_len)/1024.;*/
    cof = (64. *  (double)f->psd_len)/1024.;
    /*step1: psd estimate and normalize to SPL*/
    for(k = 0; k < f->psd_len; k++) {
        re = mdct_buf[k];
        f->psd[k] = psd_estimate_usemdct(re, cof);
    }
    
    /*step2: tone and noise masker estimate*/
    psd_tonemasker(f->psd, f->psd_len, f->splnum1, f->splnum2, f->ptm, f->tone_flag);
    psd_noisemasker(f->psd, f->psd_len, f->tone_flag, f->cb_hopbin, f->geomean_table, f->pnm);

    /*step3: check near masker, merge it if near*/
    check_near_masker(f->ptm, f->pnm, f->psd_ath, f->psd_bark, f->psd_len);

    /*step4: caculate individule threshold (tone, noise)*/
    tone_mask_threshold(f->ptm, f->psd_bark, f->spread_effect, f->tone_thres, f->psd_len);
    noise_mask_threshold(f->pnm, f->psd_bark, f->spread_effect, f->noise_thres, f->psd_len);

    /*step5: caculate global threshold*/
    global_threshold_mag(f->tone_thres, f->noise_thres, f->psd_ath, f->global_thres, f->psd_len);
    memcpy(gth, f->global_thres, sizeof(double)*f->psd_len);

#ifdef TEST_MAXMINDB
    LLZ_PRINT("dbmax=%f, dbmin=%f\n", dbmax, dbmin);
#endif
}

static void llz_psychomodel1_get_gth(unsigned long handle, double *gth)
{
    llz_psychomodel1_t *f = (llz_psychomodel1_t *)handle;

    memcpy(gth, f->global_thres, sizeof(double)*f->psd_len);
}


#define TONE_FREQ_MIN    100
#define TONE_FREQ_MAX    3500
#define TONE_CNT_MAX     220

#define TONE_FREQ_LFMAX  2500 //2000 //2500
#define TONE_LF_CNT_MAX  110 
#define TONE_HF_CNT_MAX  110
#define TONE_PSD_LF_THR  40 //30
#define TONE_PSD_HF_THR  30 //10 //30 //15 //30
#define TONE_PSD_LF_THR1 10 //30
#define TONE_PSD_HF_THR1 8 //30 //15 //30

#define ENRG_RATIO_THR   5

typedef struct _tone_info_t {
    int    tone_flag;
    int    bin_index;
    double freq;
    double power;
    double psd;
} tone_info_t;

typedef struct _hop_info_t {
    int hop;
    int cnt;
} hop_info_t;

typedef struct _hop_cof_t {
    int     base_hop;
    double  base_freq;
    double  base_hop_cof;
    double  hop_trust_cof;
} hop_cof_t;

#define BASE_CNT 5 
typedef struct _base_info_t {
    int     base_hop;
    double  base_freq;
    int     cnt;
} base_info_t;

typedef struct _note_group_t {
    int    have_note;

    int    notegrp_frame_min;   //80
    int    notegrp_frame_max;   //100
    int    notegrp_maxnote;     //9

    int    notegrp_frame_cnt;

    int    *silence_flag;
    int    *hot_pitch_flag;
    double *enrg;
    double *slope_ratio;
    int    *main_hop;
    double *main_freq;
    int    *dominate_hop;
    double *dominate_freq;

} note_group_t;

static void note_group_init(note_group_t *f, 
                           int notegrp_frame_min, int notegrp_frame_max, int notegrp_maxnote)
{
    memset(f, 0, sizeof(note_group_t));

    f->have_note = 0;

    f->notegrp_frame_min = notegrp_frame_min;
    f->notegrp_frame_max = notegrp_frame_max;
    f->notegrp_maxnote   = notegrp_maxnote;

    f->notegrp_frame_cnt = 0;

    f->silence_flag      = (int    *)malloc(notegrp_frame_max * sizeof(int));
    f->hot_pitch_flag    = (int    *)malloc(notegrp_frame_max * sizeof(int));
    f->enrg              = (double *)malloc(notegrp_frame_max * sizeof(double));
    f->slope_ratio       = (double *)malloc(notegrp_frame_max * sizeof(double));
    f->main_hop          = (int    *)malloc(notegrp_frame_max * sizeof(int));
    f->main_freq         = (double *)malloc(notegrp_frame_max * sizeof(double));
    f->dominate_hop      = (int    *)malloc(notegrp_frame_max * sizeof(int));
    f->dominate_freq     = (double *)malloc(notegrp_frame_max * sizeof(double));

    memset(f->silence_flag,     0, notegrp_frame_max * sizeof(int));
    memset(f->hot_pitch_flag,   0, notegrp_frame_max * sizeof(int));
    memset(f->enrg,             0, notegrp_frame_max * sizeof(double));
    memset(f->slope_ratio,      0, notegrp_frame_max * sizeof(double));
    memset(f->main_hop,         0, notegrp_frame_max * sizeof(int));
    memset(f->main_freq,        0, notegrp_frame_max * sizeof(double));
    memset(f->dominate_hop,     0, notegrp_frame_max * sizeof(int));
    memset(f->dominate_freq,    0, notegrp_frame_max * sizeof(double));
}

static void note_group_uninit(note_group_t *f)
{
    if (f) {
        f->have_note = 0;

        free(f->silence_flag);
        f->silence_flag = NULL;

        free(f->hot_pitch_flag);
        f->hot_pitch_flag = NULL;

        free(f->enrg);
        f->enrg = NULL;

        free(f->slope_ratio);
        f->slope_ratio = NULL;

        free(f->main_hop);
        f->main_hop = NULL;

        free(f->main_freq);
        f->main_freq = NULL;

        free(f->dominate_hop);
        f->dominate_hop = NULL;

        free(f->dominate_freq);
        f->dominate_freq = NULL;
    }
}

static double slope_ratio_calc(double enrg_prev, double enrg_cur)
{
    double slope_ratio;

    if (enrg_prev == 0.0)
        enrg_prev = 0.0001;

    slope_ratio = enrg_cur / enrg_prev;

    return slope_ratio;
}

typedef struct _thr_para_t {
    double enrg__start_thr;           // 0.2  [0.0, 1.0]   >=
    double hop_cof__start_thr1;       // 1.4  [0.0. 2.0]   >= 
    double hop_cof__start_thr2;       // 1.8  [0.0, 2.0]   >=

    double ratio__stop_thr;           // 0.9  [0.0, 1.0]   <
    double hop_cof__stop_thr1;        // 1.2  [0.0, 2.0]   <
    double hop_cof__stop_thr2;        // 0.7  [0.0, 2.0]   <
    double enrg__stop_thr;            // 0.4  [0.0, 1.0]   <

    double silence__est_thr;          // 0.001

    int    status_keep_cnt__thr;       // 1    [1, 3]       >=


    double base_hop_cof__pitch_thr;   // 0.8  [0.0, 1.0]   >=
    double hop_trust_cof__pitch_thr;  // 0.8  [0.0, 1.0]   >
    double hop_cof__pitch_thr;        // 1.6  [0.0, 2.0]   >
    double base_hop_cof__trans_thr;   // 0.5  [0.0, 1.0]   > 
    double hop_trust_cof__trans_thr;  // 0.6  [0.0, 1.0]   <
    
} thr_para_t;


typedef struct _llz_musicpitch_t {
    int         frame_index;

    double      fs;
    int         frame_len;
    double      frame_duration;
    double      *sample_buf;
    int         iir_flag;
    double      *sample_flt_buf;

    /*fft and energy var*/
    unsigned long   h_fft;
    double      *fft_buf;
    double      *win;
    double      *power;

    llz_psychomodel1_t *psy;

    unsigned long   h_iir;

    /*feature*/
    double      enrg_time[2];      // time domain power
    double      enrg_freq[2];      // freq domain power
    double      zrc[2];            // zero crossing rate
    double      pitch_hop_est[2];  // pitch hop estimate, maybe the very base pitch
    double      variance[2];       // variance of current frame

    double      psd_notone_avg;
    double      psd_maxavg_diff;             // max is the lf maxpsd, avg is the notone avgpsd

    thr_para_t  thr;

    /*tone info*/
    tone_info_t tone_info[TONE_CNT_MAX];
    tone_info_t tone_lf_info[TONE_LF_CNT_MAX];
    tone_info_t tone_hf_info[TONE_HF_CNT_MAX];
    int         tone_cnt;
    int         tone_lf_cnt;
    int         tone_hf_cnt;

    /*freq hop info*/
    hop_info_t  hop_info[2][TONE_CNT_MAX];
    int         hop_info_cnt[2];
    hop_cof_t   hop_cof[2];

    /*status var*/
    int         cur_status;
    int         status_keep_cnt;

    llz_music_event_func_t *func;
    unsigned long h_event;

    /*note group*/
    int          first_pitch_flag;
    note_group_t note_grp;

    /*time domain pitch analysis*/
    unsigned long h_pitch;
    double    *time_pitch_contour;
    double    *time_pitch_temp;

} llz_musicpitch_t;

int  llz_musicpitch_get_framelen(int sample_rate)
{
    switch (sample_rate) {
        case 8000:
            return 256;
        case 11025:
            return 256;
        case 22050:
            return 512;
        case 32000:
            return 512;
        case 44100:
            return 1024;
        default:
            return 512;
    }

}

void llz_musicpitch_rom_init()
{
    llz_psychomodel1_rom_init();
}

static void set_default_thr(thr_para_t *thr)
{
    thr->enrg__start_thr            = 0.2;       // 0.2  [0.0, 1.0]   >=
    thr->hop_cof__start_thr1        = 1.4;       // 1.4  [0.0. 2.0]   >= 
    thr->hop_cof__start_thr2        = 1.8;       // 1.8  [0.0, 2.0]   >=

    thr->ratio__stop_thr            = 0.9;       // 0.9  [0.0, 1.0]   <
    thr->hop_cof__stop_thr1         = 1.2;       // 1.2  [0.0, 2.0]   <
    thr->hop_cof__stop_thr2         = 0.7;       // 0.7  [0.0, 2.0]   <
    thr->enrg__stop_thr             = 0.4;       // 0.4  [0.0, 1.0]   <

    thr->silence__est_thr           = 0.001;     // 0.001

    thr->status_keep_cnt__thr       = 1;         // 1    [1, 3]       >=

    thr->base_hop_cof__pitch_thr    = 0.8;       // 0.8  [0.0, 1.0]   >=
    thr->hop_trust_cof__pitch_thr   = 0.8;       // 0.8  [0.0, 1.0]   >
    thr->hop_cof__pitch_thr         = 1.5;       // 1.6  [0.0, 2.0]   >
    thr->base_hop_cof__trans_thr    = 0.5;       // 0.5  [0.0, 1.0]   > 
    thr->hop_trust_cof__trans_thr   = 0.6;       // 0.6  [0.0, 1.0]   <
}


unsigned long llz_musicpitch_init(int fs, int frame_len_in, 
                              llz_music_event_func_t *func, unsigned long h_event)
{
    llz_musicpitch_t *f;
    double a_cof[10], b_cof[10];

    int    frame_len;

    int    note_group_frame_min = 80;
    int    note_group_frame_max = 100;

    frame_len = OVERLAP_COF * frame_len_in;

    f = (llz_musicpitch_t *)malloc(sizeof(llz_musicpitch_t));
    memset(f, 0, sizeof(llz_musicpitch_t));

    f->fs               = (double)fs;
    f->frame_len        = frame_len;
    f->frame_index      = 0;
    f->frame_duration   = (double)frame_len_in/(double)fs;

    f->sample_buf       = (double *)malloc(frame_len * sizeof(double));
    memset(f->sample_buf, 0, frame_len * sizeof(double));

    f->iir_flag         = 0;
    f->sample_flt_buf   = (double *)malloc(frame_len * sizeof(double));
    memset(f->sample_flt_buf, 0, frame_len * sizeof(double));

    f->fft_buf          = (double *)malloc(2 * frame_len * sizeof(double));
    memset(f->fft_buf   , 0, 2 * frame_len * sizeof(double));

    f->h_fft            = llz_fft_init(frame_len);

    f->win              = (double *)malloc(frame_len * sizeof(double));
    memset(f->win       , 0, frame_len * sizeof(double));
    llz_hamming(f->win, frame_len);

    f->power            = (double *)malloc((frame_len>>1)*sizeof(double));
    memset(f->power     , 0, (frame_len>>1)*sizeof(double));

    f->psy = (llz_psychomodel1_t *)llz_psychomodel1_init(fs, frame_len);


    memset(a_cof, 0, 10*sizeof(double));
    memset(b_cof, 0, 10*sizeof(double));
#if 0 
    a_cof[0] = 1.0;
    a_cof[1] = -0.7478;
    a_cof[2] = 0.2722;
    b_cof[0] = 1.0;
    b_cof[1] = 0.5050;
    b_cof[2] = -1.01;
    b_cof[3] = 0.5050;
#else 
    a_cof[0] = 1.0;
    a_cof[1] = -0.3695;
    a_cof[2] = 0.1958;
    b_cof[0] = 1.0;
    b_cof[1] = 0.2066;
    b_cof[2] = 0.4131;
    b_cof[3] = 0.2066;
#endif
    f->h_iir = llz_iir_filter_init(3, a_cof, 3, b_cof);

    f->psd_notone_avg   = 0;
    f->psd_maxavg_diff  = 0;

    set_default_thr(&(f->thr));

    memset(f->tone_info   , 0, TONE_CNT_MAX * sizeof(tone_info_t));
    memset(f->tone_lf_info, 0, TONE_LF_CNT_MAX * sizeof(tone_info_t));
    memset(f->tone_hf_info, 0, TONE_HF_CNT_MAX * sizeof(tone_info_t));
    f->tone_cnt         = 0;
    f->tone_lf_cnt      = 0;
    f->tone_hf_cnt      = 0;

    f->cur_status      = 0;
    f->status_keep_cnt = 0;

    if (func && h_event) {
        f->func = func;
        f->h_event = h_event;
    } else {
        f->func = NULL;
        f->h_event = 0;
    }

    /*note_group_frame_min = (int)(1.8 / f->frame_duration);*/
    /*note_group_frame_max = (int)(2.3 / f->frame_duration);*/
    note_group_frame_min = (int)(0.5 / f->frame_duration);
    note_group_frame_max = (int)(5.0 / f->frame_duration);
    LLZ_PRINT("note group frame, min = %d, max = %d\n", note_group_frame_min, note_group_frame_max);

    note_group_init(&(f->note_grp), note_group_frame_min, note_group_frame_max, 9);
    f->first_pitch_flag = 0;

    /*time domain pitch analysis*/
    f->h_pitch = llz_pitch_init(frame_len_in, PITCH_YIN);
    f->time_pitch_contour = (double *)malloc(note_group_frame_max * sizeof(double));
    memset(f->time_pitch_contour, 0, note_group_frame_max * sizeof(double));
    f->time_pitch_temp    = (double *)malloc(note_group_frame_max * sizeof(double));
    memset(f->time_pitch_temp, 0, note_group_frame_max * sizeof(double));



    return (unsigned long)f;
}

void llz_musicpitch_uninit(unsigned long handle)
{
    llz_musicpitch_t *f = (llz_musicpitch_t *)handle;

    if (f) {
        note_group_uninit(&(f->note_grp));

        free(f->time_pitch_contour);
        f->time_pitch_contour = NULL;
        llz_pitch_uninit(f->h_pitch);

        free(f->time_pitch_temp);
        f->time_pitch_temp = NULL;


        llz_iir_filter_uninit(f->h_iir);

        llz_psychomodel1_uninit((unsigned long)(f->psy));


        free(f->fft_buf);
        f->fft_buf = NULL;

        llz_fft_uninit(f->h_fft);

        free(f->win);
        f->win = NULL;

        free(f->power);
        f->power = NULL;

        free(f->sample_buf);
        f->sample_buf = NULL;

        free(f->sample_flt_buf);
        f->sample_flt_buf = NULL;

        free(f);
        f = NULL;
    }
}

static int tone_sort_function_power(const void *a, const void *b)   
{   
    tone_info_t *a_tone, *b_tone;

    a_tone = (tone_info_t *) a;
    b_tone = (tone_info_t *) b;

    if((*a_tone).power > (*b_tone).power)
        return -1;
    if((*a_tone).power == (*b_tone).power)
        return 0;
    if((*a_tone).power < (*b_tone).power)
        return 1;

    return 0;
}   

static int tone_sort_function_freq(const void *a, const void *b)   
{   
    tone_info_t *a_tone, *b_tone;

    a_tone = (tone_info_t *) a;
    b_tone = (tone_info_t *) b;

    if((*a_tone).freq > (*b_tone).freq)
        return 1;
    if((*a_tone).freq == (*b_tone).freq)
        return 0;
    if((*a_tone).freq < (*b_tone).freq)
        return -1;

    return 0;
}   

enum {
    TONE_SORT_POWER = 0,
    TONE_SORT_FREQ,
};
     
static void tone_sort(int sort_type, tone_info_t *data, int size)
{
    switch (sort_type) {
        case TONE_SORT_POWER:
            llz_qsort((void   *)data,   size,   sizeof(data[0]),   tone_sort_function_power);   
            break;
        case TONE_SORT_FREQ:
            llz_qsort((void   *)data,   size,   sizeof(data[0]),   tone_sort_function_freq);   
            break;
    }
}

static void find_tone(llz_musicpitch_t *f)
{
    int k;
    llz_psychomodel1_t *psy = f->psy;

    double  notone_power;
    int     notone_cnt;

    /*get the music tone roughtly */
    memset(psy->tone_flag, 0, psy->psd_len*sizeof(int));
    for (k = 0; k < psy->psd_len; k++) 
        ismusictone(psy->psd, psy->psd_len, k, psy->splnum1, psy->splnum2, psy->tone_flag);

    notone_power = 0.0;
    notone_cnt = 0;

    f->tone_cnt = 0;
    memset(f->tone_info, 0, TONE_CNT_MAX * sizeof(tone_info_t));
    for (k = 0; k < psy->psd_len; k++) {
        double freq;

        freq = freqbin2freq(psy->fs, psy->psd_len*2, k);
        if (psy->tone_flag[k]) {
            /*update the tone info*/
            if (f->tone_cnt == 0) {
                if (freq > TONE_FREQ_MIN) {
                    f->tone_info[f->tone_cnt].tone_flag = 1;
                    f->tone_info[f->tone_cnt].bin_index = k;
                    f->tone_info[f->tone_cnt].freq      = freq;
                    f->tone_info[f->tone_cnt].power     = f->power[k];
                    f->tone_info[f->tone_cnt].psd       = psy->psd[k];
                    f->tone_cnt++;
                }
            } else {
                if ((freq - f->tone_info[f->tone_cnt-1].freq) > TONE_FREQ_MIN) {
                    f->tone_info[f->tone_cnt].tone_flag = 1;
                    f->tone_info[f->tone_cnt].bin_index = k;
                    f->tone_info[f->tone_cnt].freq      = freq;
                    f->tone_info[f->tone_cnt].power     = f->power[k];
                    f->tone_info[f->tone_cnt].psd       = psy->psd[k];
                    f->tone_cnt++;
                }
            }
        } else {
            /*only consider the low frequency, high frequency can not be taken into acount*/
            if (freq < TONE_FREQ_LFMAX) {
                int j;
                int flag;
                int pos_a, pos_b;

                /*tone neighbour near bin can not be taken into acount*/
                flag  = 0;
                pos_a = k - 5;
                pos_a = LLZ_MAX(pos_a, 0);

                pos_b = k + 5;
                pos_b = LLZ_MIN(pos_b, psy->psd_len-1);

                for (j = pos_a; j <= pos_b; j++) {
                    if (psy->tone_flag[j]) {
                        flag = 1;
                        break;
                    }
                }

                if (0 == flag) {
                    notone_power += f->power[k];
                    notone_cnt++;
                }
            }
        }
    }

    /*generate the bottom average notone psd level shape*/
    if (notone_cnt > 0)
        f->psd_notone_avg = PSD_NORMALIZER + db_pos(notone_power / notone_cnt);
    else 
        f->psd_notone_avg = PSD_NORMALIZER;

    /*LLZ_PRINT(">>> notonecnt=%d, psd_notone_avg = %lf\n", notone_cnt, f->psd_notone_avg);*/
}


static void splite_tone(llz_musicpitch_t *f)
{
    int     i;
    int     flag;

    double  lf_psd_thr, hf_psd_thr;

    double  tone_lf_psd_max;
    double  tone_hf_psd_max;

    tone_sort(TONE_SORT_POWER, f->tone_info, f->tone_cnt);

    /*first hf flag*/
    flag = 0;

    /*tone_info is already sort by power, so the first one is the maxium*/
    f->tone_lf_cnt     = f->tone_hf_cnt = 0;
    tone_lf_psd_max    = f->tone_info[0].psd;
    f->psd_maxavg_diff = fabs(tone_lf_psd_max - f->psd_notone_avg);

    /*fix the the threshold, max avg diff means the max psd and the low frequency average psd level*/
    if (f->psd_maxavg_diff > 20) 
        lf_psd_thr = tone_lf_psd_max - TONE_PSD_LF_THR + 10;
    else if (f->psd_maxavg_diff > 15)
        lf_psd_thr = tone_lf_psd_max - TONE_PSD_LF_THR;
    else 
        lf_psd_thr = tone_lf_psd_max - TONE_PSD_LF_THR1;


    for (i = 0; i < f->tone_cnt; i++) {
        /* 
         * get the low freq and high freq tone info, filter the freq out of range(TONE_FREQ_LFMAX, .) and the 
         * freq which is less TONE_PSD_LF_THR dB than max tone psd
         */
        if (f->tone_info[i].freq < TONE_FREQ_LFMAX) {
            if (f->tone_lf_cnt < TONE_LF_CNT_MAX && f->tone_info[i].psd > lf_psd_thr) {
                memcpy(&(f->tone_lf_info[f->tone_lf_cnt]), &(f->tone_info[i]), sizeof(tone_info_t));
                f->tone_lf_cnt++;
            }
        } else {
            /*because sort by power, first in this branch is the max psd in hf domain*/
            if (0 == flag) {
                tone_hf_psd_max = f->tone_info[i].psd;
                if (f->iir_flag)
                    hf_psd_thr = tone_hf_psd_max - TONE_PSD_HF_THR - 20;
                else
                    hf_psd_thr = tone_hf_psd_max - TONE_PSD_HF_THR;

                flag = 1;
            }
            if (f->tone_hf_cnt < TONE_HF_CNT_MAX && f->tone_info[i].psd > hf_psd_thr) {
                memcpy(&(f->tone_hf_info[f->tone_hf_cnt]), &(f->tone_info[i]), sizeof(tone_info_t));
                f->tone_hf_cnt++;
            }
        }
    }

    LLZ_PRINT(">>> curtime=%lf, lfcnt=%d, max_lfpsd=%lf, hfcnt=%d, max_hfpsd=%lf\n", 
               f->frame_duration*f->frame_index, f->tone_lf_cnt, tone_lf_psd_max, f->tone_hf_cnt, tone_hf_psd_max);

    tone_sort(TONE_SORT_FREQ, f->tone_lf_info, f->tone_lf_cnt);
    tone_sort(TONE_SORT_FREQ, f->tone_hf_info, f->tone_hf_cnt);
}



static int hop_sort_function(const void *a, const void *b)   
{   
    hop_info_t *a_hop, *b_hop;

    a_hop = (hop_info_t *) a;
    b_hop = (hop_info_t *) b;

    if((*a_hop).cnt > (*b_hop).cnt)
        return -1;
    if((*a_hop).cnt == (*b_hop).cnt)
        return 0;
    if((*a_hop).cnt < (*b_hop).cnt)
        return 1;

    return 0;
}   
     
static void hop_sort(hop_info_t *data, int size)
{
    llz_qsort((void   *)data,   size,   sizeof(data[0]),   hop_sort_function);   
}

#define DIFF_THR 2 
void analysis_tone(llz_musicpitch_t *f)
{
    int i, j;
    int flag;
    int hop_cnt, hop_info_real_cnt, base_hop;
    int freq_hop[TONE_CNT_MAX];

    memset(freq_hop, 0, TONE_CNT_MAX*sizeof(int));
    /*compute lf&hf hop and hop cnt, hop_cnt is the sum of lf and hf*/
    hop_cnt = 0;
    if (f->tone_lf_cnt > 1) {
        for (i = 1; i < f->tone_lf_cnt; i++) {
            freq_hop[hop_cnt] = f->tone_lf_info[i].bin_index - f->tone_lf_info[i-1].bin_index;
            hop_cnt++;
        }
    } else if (f->tone_lf_cnt == 1) {
        freq_hop[hop_cnt] = f->tone_lf_info[0].bin_index;
        hop_cnt = 1;
    }

    if (f->tone_hf_cnt > 1) {
        for (i = 1; i < f->tone_hf_cnt; i++) {
            freq_hop[hop_cnt] = f->tone_hf_info[i].bin_index - f->tone_hf_info[i-1].bin_index;
            hop_cnt++;
        }
    } else if (f->tone_hf_cnt == 1 && f->tone_lf_cnt == 0) {
        freq_hop[hop_cnt] = f->tone_hf_info[0].bin_index;
        hop_cnt = 1;
    }

    if (hop_cnt > 1) {
        /*compute hop_info_cnt, hop_info is the group of different hops*/
        f->hop_info_cnt[1] = 0;
        for (i = 0; i < hop_cnt; i++) {
            /*same hop flag, reset each time*/
            flag = 0;

            for (j = 0; j < f->hop_info_cnt[1]; j++) {
                if (freq_hop[i] == f->hop_info[1][j].hop) {
                    flag = 1;
                    f->hop_info[1][j].cnt++;
                }
            }

            if (flag == 0) {
                f->hop_info[1][f->hop_info_cnt[1]].hop = freq_hop[i];
                f->hop_info[1][f->hop_info_cnt[1]].cnt = 1;
                f->hop_info_cnt[1]++;
            }
        }

        /*appeared most time , maybe the dominate freq hop*/
        hop_sort(f->hop_info[1], f->hop_info_cnt[1]);

        /*refilte step1: we assume the first hop is the base dominate hop, but maybe not the one */
        base_hop = f->hop_info[1][0].hop;
        if (f->hop_info_cnt[1] >= 2) {
            int cmp_hop;
            cmp_hop = f->hop_info[1][1].hop;

            /*we should verify if the cmp_hop is the more base hop than base_hop, the diff use 3 to limited*/
            if (cmp_hop < base_hop) {
                int ref_hop;
                int diff;

                /*only concern 5 time harmonic, so only 5 times base hop is can be concern about*/
                for (j = 2; j < 5; j++) {
                    ref_hop = cmp_hop * j;
                    diff = fabs(base_hop - ref_hop);
                    if (diff <= DIFF_THR) {
                        /*update base_hop*/
                        base_hop = cmp_hop; 

                        /*update the first hop_info using new base_hop*/
                        f->hop_info[1][0].hop = base_hop;
                        f->hop_info[1][0].cnt += f->hop_info[1][1].cnt;
                        f->hop_info[1][1].cnt = 0;

                        break;
                    }
                }
            }
        }

        /*refilte step2: merge the hop which maybe the same harmonic group*/
        for (i = 1; i < f->hop_info_cnt[1]; i++) {
            int diff;

            diff = fabs(f->hop_info[1][i].hop - base_hop);
            if (diff <= 2) {
                f->hop_info[1][0].cnt += f->hop_info[1][i].cnt;
                f->hop_info[1][i].cnt = 0;
            } else {
                int ref_hop;
                for (j = 1; j < 5; j++) {
                    ref_hop = base_hop * j;
                    diff = fabs(f->hop_info[1][i].hop - ref_hop);
                    if (diff <= DIFF_THR) {
                        f->hop_info[1][0].cnt += f->hop_info[1][i].cnt;
                        f->hop_info[1][i].cnt = 0;
                        break;
                    }
                }
            }
        }

        /*calculate the hop cofficient*/
        hop_info_real_cnt = 0;
        for (i = 0; i < f->hop_info_cnt[1]; i++) {
            if (f->hop_info[1][i].cnt > 0)
                hop_info_real_cnt++;
        }

        /*
         * caculate the hop_cof, use current hop_info.hop to update the base_hop
         * base_hop:     the base freq hop, is the base harmonic freq hop
         * base_hop_cof: the base hop appeared times / total hop cnt
         * hop_trust_cof:the all freq hop(not only base freq hop) cnt / total hop cnt
         */
        f->hop_cof[1].base_hop      = f->hop_info[1][0].hop;
        f->hop_cof[1].base_hop_cof  = (double)(f->hop_info[1][0].cnt) / hop_cnt;
        f->hop_cof[1].hop_trust_cof = 1.0 - (double)hop_info_real_cnt / hop_cnt;

        /*readjust the hop_cof and trust_cof*/
        if (f->psd_maxavg_diff > 20) {
            f->hop_cof[1].base_hop_cof  += 0.2;
            f->hop_cof[1].hop_trust_cof += 0.2;
        } else if (f->psd_maxavg_diff > 15) {
            f->hop_cof[1].base_hop_cof  += 0.1;
            f->hop_cof[1].hop_trust_cof += 0.2;
        }
    } else if (hop_cnt == 1) {
        if (f->psd_maxavg_diff > 20) {
            f->hop_cof[1].base_hop      = freq_hop[0];
            f->hop_cof[1].base_hop_cof  = 1.0;
            f->hop_cof[1].hop_trust_cof = 1.0;;
        } else if (f->psd_maxavg_diff > 15) {
            f->hop_cof[1].base_hop      = freq_hop[0];
            f->hop_cof[1].base_hop_cof  = 1.0;
            f->hop_cof[1].hop_trust_cof = 0.5;;
        } else {
            f->hop_cof[1].base_hop      = 0;
            f->hop_cof[1].base_hop_cof  = 0.0;
            f->hop_cof[1].hop_trust_cof = 0.0;;
        }
    }
    
    /*convert base hop to base freq*/
    f->hop_cof[1].base_freq = freqbin2freq(f->psy->fs, f->psy->psd_len*2, f->hop_cof[1].base_hop);

}


/*hot pitch rotate*/
static void reanalysis_hot_pitch(llz_musicpitch_t *f)
{
    note_group_t *s;
    int i, j;

    s = &(f->note_grp);

    /*if not sharply down, change 0 to 1*/
    for (i = 1; i < s->notegrp_frame_cnt-1; i++) {
        if (s->hot_pitch_flag[i] == 0 && s->silence_flag[i] == 0) {
            if (s->hot_pitch_flag[i-1] == 1 && s->hot_pitch_flag[i+1] && 
                (s->slope_ratio[i] > 0.7 && s->slope_ratio[i] < 2.0) )
                s->hot_pitch_flag[i] = 1;
        }
    }

#if 1 
    /*recheck the pitch, if sharply down and repeate more times, will change 1 to 0*/
    for (i = 3; i < s->notegrp_frame_cnt-2; i++) {
        int cnt;
        double change_ratio1, change_ratio2, change_ratio3;

        if (s->hot_pitch_flag[i] == 1) {
            /*if (s->slope_ratio[i] < 0.3) {*/
            if (s->slope_ratio[i] < 0.2) {
                s->hot_pitch_flag[i] = 0;
            /*} else if (s->slope_ratio[i] < 0.4) {*/
            } else if (s->slope_ratio[i] < 0.3) {
                cnt = 0;
                for (j = 1; j <= 2; j++) {
                    if (s->slope_ratio[i-j] < 1)
                        cnt++;
                    if (s->slope_ratio[i+j] < 1)
                        cnt++;
                }
                if (cnt >= 2)
                    s->hot_pitch_flag[i] = 0;
            /*} else if (s->slope_ratio[i] < 0.7) {*/
            } else if (s->slope_ratio[i] < 0.6) {
                change_ratio1 = s->slope_ratio[i-2] * s->slope_ratio[i-1] * s->slope_ratio[i];
                change_ratio2 = s->slope_ratio[i-1] * s->slope_ratio[i]   * s->slope_ratio[i+1];
                change_ratio3 = s->slope_ratio[i]   * s->slope_ratio[i+1] * s->slope_ratio[i+2];
                /*if (change_ratio1 < 0.4 || change_ratio2 < 0.5 || change_ratio3 < 0.4)*/
                if (change_ratio1 < 0.3 || change_ratio2 < 0.4 || change_ratio3 < 0.3)
                    s->hot_pitch_flag[i] = 0;
            }
        }
    }
#endif 

}


static int time_pitch_sort(const void *a, const void *b)   
{   
    double *a1, *b1;

    a1 = (double *)a;
    b1 = (double *)b;

    if(*a1 > *b1)
        return 1;
    if(*a1 == *b1)
        return 0;
    if(*a1 < *b1)
        return -1;

    return 0;
}   


static void note_group_analysis(llz_musicpitch_t *f)
{
    note_group_t *s;

    int i, j, k;

    int     pitch_flag;
    int     start_pos;
    int     end_pos;

    int     hop_cnt;
    int     hop[200];
    int     hop_same_cnt[200];
    double  freq[200];

    int     analysis_flag;
    int     dominate_hop;
    double  dominate_freq;

    int     temp_cnt;
    double  trust_cof;

    double  duration;
    int     note_duration_cnt;

    double  enrg_avg;

    s = &(f->note_grp);

    hop_cnt = 0;
    memset(hop,          0, 200 * sizeof(int));
    memset(freq,         0, 200 * sizeof(double));
    memset(hop_same_cnt, 0, 200 * sizeof(int));
    dominate_hop  = 0;
    dominate_freq = 0.0;
    temp_cnt      = 0;
    trust_cof     = 0.0;

    pitch_flag    = 0;
    start_pos     = 0;
    end_pos       = 0;

    analysis_flag = 0;
    for (i = 1; i < s->notegrp_frame_cnt; i++) {
        if (s->hot_pitch_flag[i] == 0) {
            if (pitch_flag == 0)
                continue;

            end_pos       = i;
            analysis_flag = 1;
            LLZ_PRINT(">>> start_pos=%d, end_pos=%d\n", f->frame_index - s->notegrp_frame_cnt +start_pos, f->frame_index - s->notegrp_frame_cnt +end_pos);
        } else {
            if (0 == pitch_flag) {
                start_pos  = i;
                pitch_flag = 1;
            } else {
                end_pos = i;
                if (i == s->notegrp_frame_cnt - 1)
                    analysis_flag = 1;
            }
        }

        if (1 == analysis_flag) {
            /*analysis this pitch duration dominate hop and freq*/
            if (end_pos - start_pos >= 4) {
                /*statistic the different hop*/
                for (j = start_pos; j < end_pos; j++) {
                    int flag;

                    flag = 0;
                    for (k = 0; k < hop_cnt; k++) {
                        if (hop[k] == s->main_hop[j]) {
                            flag = 1;
                            break;
                        }
                    }

                    if (1 == flag) {
                        hop_same_cnt[k] += 1;
                    } else {
                        hop[hop_cnt]          = s->main_hop[j];
                        hop_same_cnt[hop_cnt] = 1;
                        freq[hop_cnt]         = s->main_freq[j];
                        hop_cnt++;
                        assert(hop_cnt < 200);
                    }
                }

                /*now can get the max of hop_same_cnt array, it is the dominate*/
                temp_cnt      = hop_same_cnt[0];
                dominate_hop  = hop[0];
                dominate_freq = freq[0];
                for (k = 1; k < hop_cnt; k++) {
                    if (hop_same_cnt[k] > temp_cnt) {
                        temp_cnt      = hop_same_cnt[k];
                        dominate_hop  = hop[k];
                        dominate_freq = freq[k];
                    }
                }

                /*refilte the dominate hop and main_hop*/
                for (j = start_pos; j < end_pos; j++) {
                    if (s->main_hop[j] == dominate_hop)
                        continue;
                    else {
                        int tmp_hop;
                        int diff;
                        int flag;

                        for (k = 2; k < 4; k++) {
                            flag = 0;

                            tmp_hop = k * dominate_hop;
                            diff    = fabs(tmp_hop - s->main_hop[j]);
                            if (diff <= 2) {
                                s->main_hop[j] = dominate_hop;
                                temp_cnt++;
                                flag = 1;
                            }

                            if (flag == 0) {
                                tmp_hop = k * s->main_hop[j];
                                diff    = fabs(tmp_hop - s->main_hop[j]);
                                if (diff <= 2) {
                                    s->main_hop[j] = dominate_hop;
                                    temp_cnt++;
                                }
                            }
                        }
                    }
                }

                trust_cof = (double)temp_cnt / (end_pos - start_pos);

                if (trust_cof < 0.5) {
                    dominate_hop  = 0;
                    dominate_freq = 0.0;
                }

                for (j = start_pos; j < end_pos; j++) {
                    s->dominate_hop[j]  = dominate_hop;
                    s->dominate_freq[j] = dominate_freq;
                }

#if 1 
                /*this is tuning fine, use time pitch. if time pitch variance is small, replace it*/
                {
                    double pitch_avg;
                    double pitch_var;
                    double pitch_media;
                    double diff;

                    pitch_avg = 0.0;
                    for (j = start_pos+1; j < end_pos; j++) {
                        pitch_avg += f->time_pitch_contour[j];
                        f->time_pitch_temp[j] = f->time_pitch_contour[j];
                    }
                    pitch_avg = pitch_avg / (end_pos- 1 - start_pos);
                    
                    pitch_var = 0.0;
                    for (j = start_pos+1; j < end_pos; j++) {
                        diff = pitch_avg - f->time_pitch_contour[j];
                        pitch_var += diff * diff; 
                    }
                    pitch_var = pitch_var / (end_pos- 1 - start_pos);
                
                    llz_qsort((void   *)(f->time_pitch_temp+start_pos+1),   (end_pos-start_pos-1),   sizeof(f->time_pitch_temp[0]),   time_pitch_sort);   
                    pitch_media = f->time_pitch_temp[start_pos+1+(end_pos-start_pos-1)/2];
                    /*LLZ_PRINT("----------avg=%lf, media=%lf, var=%lf \n", pitch_avg, pitch_media, pitch_var);*/

                    if (pitch_var < pitch_avg * 0.1) {
                        for (j = start_pos; j < end_pos; j++) {
                            s->dominate_hop[j]  = freq2freqbin(f->psy->fs, f->psy->psd_len*2, pitch_avg);
                            s->dominate_freq[j] = pitch_media; //pitch_avg;
                        }
                    }
                }
#endif

            } else {
                for (j = start_pos; j < end_pos; j++) {
                    s->dominate_hop[j]  = 0;
                    s->dominate_freq[j] = 0.0;
                }
            }

            hop_cnt = 0;
            memset(hop, 0, 200 * sizeof(int));
            memset(freq, 0, 200 * sizeof(double));
            memset(hop_same_cnt, 0, 200 * sizeof(int));
            dominate_hop  = 0;
            dominate_freq = 0.0;
            temp_cnt      = 0;
            trust_cof     = 0.0;

            pitch_flag    = 0;
            analysis_flag = 0;
        }
    }

#if 0
    for (i = 1; i < s->notegrp_frame_cnt - 1; i++) {
        double time_pos;

        time_pos = (f->frame_index - s->notegrp_frame_cnt + i) * f->frame_duration;
        LLZ_PRINT(">>> curtime=%lf, sl_flag=%d, pitch_flag = %d, slope=%lf, enrg=%lf, hop=%d, freq=%lf, do_hop=%d, do_freq=%lf, t_pitch=%lf\n", 
                time_pos, s->silence_flag[i], s->hot_pitch_flag[i], 
                s->slope_ratio[i],  
                s->enrg[i], s->main_hop[i], s->main_freq[i],
                s->dominate_hop[i], s->dominate_freq[i],
                f->time_pitch_contour[i]
              );
    }
#endif

    note_duration_cnt = 1;
    enrg_avg = 0.0;
    for (i = 1; i < s->notegrp_frame_cnt; i++) {
        if (s->hot_pitch_flag[i] == s->hot_pitch_flag[i-1]) {
            pitch_flag = s->hot_pitch_flag[i];
            note_duration_cnt++;
            enrg_avg += s->enrg[i];
        } else {
            if (f->func) {
                if (pitch_flag == 0) {
                    duration = f->frame_duration * note_duration_cnt;

                    f->func->event_one_note(f->h_event, 0, duration, 0.0);
                } else {
                    duration = f->frame_duration * (1 + note_duration_cnt);


                    f->func->event_one_note(f->h_event, s->dominate_freq[i-1], duration, enrg_avg/note_duration_cnt);
                    LLZ_PRINT(">>>>>>> freq=%lf, duration=%lf, enrg_avg=%lf\n", s->dominate_freq[i-1], duration, enrg_avg/note_duration_cnt);
                }
            }
            pitch_flag = s->hot_pitch_flag[i];
            note_duration_cnt = 1;
            enrg_avg = 0.0;
        }
    }

    /*if at last pos and not change to 0, now will calculate the remain value*/
    if (note_duration_cnt >= 2) {
        if (f->func) {
            if (pitch_flag == 0) {
                duration = f->frame_duration * note_duration_cnt;

                f->func->event_one_note(f->h_event, 0, duration, 0.0);
            } else {
                duration = f->frame_duration * (1 + note_duration_cnt);

                f->func->event_one_note(f->h_event, s->dominate_freq[s->notegrp_frame_cnt-2], duration, enrg_avg/note_duration_cnt);
                LLZ_PRINT(">>>>>> freq=%lf, duration=%lf, enrg_avg=%lf\n", s->dominate_freq[s->notegrp_frame_cnt-2], duration, enrg_avg/note_duration_cnt);
            }
        }
    }

}

static void reset_note_group_info(note_group_t *s)
{
    s->have_note         = 0;
    s->notegrp_frame_cnt = 0;

    memset(s->silence_flag,     0, s->notegrp_frame_max * sizeof(int));
    memset(s->hot_pitch_flag,   0, s->notegrp_frame_max * sizeof(int));
    memset(s->enrg,             0, s->notegrp_frame_max * sizeof(double));
    memset(s->slope_ratio,      0, s->notegrp_frame_max * sizeof(double));
    memset(s->main_hop,         0, s->notegrp_frame_max * sizeof(int));
    memset(s->main_freq,        0, s->notegrp_frame_max * sizeof(double));
}

static void note_group_addframe(llz_musicpitch_t *f, int is_hot_pitch, int is_silence)
{
    note_group_t *s;

    int     index;
    int     flag;
    double  duration;


    s    = &(f->note_grp);
    flag = 0;

    /*check if shoud splite*/
    if (s->notegrp_frame_cnt >= s->notegrp_frame_max-3)
        flag = 1;
    else {
        if ((s->notegrp_frame_cnt >= s->notegrp_frame_min) &&
            (is_silence == 1 || is_hot_pitch == 0))
            flag = 1;
    }

    /*means should splite one tap*/
    if (flag == 1) {
        if (0 == s->have_note) {
            duration = s->notegrp_frame_cnt * f->frame_duration;
            if (f->func) {
                f->func->event_one_note(f->h_event, 0, duration, 0.0);
            }
            LLZ_PRINT(">>> slience duration = %lf\n", duration);
        } else {
            duration = s->notegrp_frame_cnt * f->frame_duration;
            reanalysis_hot_pitch(f);
            note_group_analysis(f);
            LLZ_PRINT(">>> do group analysis group cnt = %d, duration=%lf\n", s->notegrp_frame_cnt, duration);
        }

        reset_note_group_info(s);

        return;
    }

    /*not slience and not note now, then will begin add note frame until analysis frame enough*/
    if (is_silence == 0 && s->have_note == 0) {
        if (s->notegrp_frame_cnt > 0) {
            duration = s->notegrp_frame_cnt * f->frame_duration;
            if (f->func) {
                f->func->event_one_note(f->h_event, 0, duration, 0.0);
            }
            LLZ_PRINT(">>> slience duration = %lf\n", duration);
        }

        reset_note_group_info(s);
        s->have_note = 1;

        return;
    }

    if (1 == s->have_note) {
        index = s->notegrp_frame_cnt;

        s->silence_flag[index]   = is_silence;
        s->hot_pitch_flag[index] = is_hot_pitch;
        s->enrg[index]           = f->enrg_time[1];
        s->slope_ratio[index]    = slope_ratio_calc(f->enrg_time[0], f->enrg_time[1]);
        s->main_hop[index]       = f->hop_cof[1].base_hop;
        s->main_freq[index]      = f->hop_cof[1].base_freq;
    }

    s->notegrp_frame_cnt++;
}


/*frequency domain statistic and analysis, then use frequency hop feature in time domain to analysis dominate freq*/
static void analysis_note(llz_musicpitch_t *f)
{
    double  hop_cof_prev;
    double  hop_cof;
    double  ratio;
    double  ratio1;

    int     is_silence;
    int     is_hot_pitch;

    thr_para_t *thr = &(f->thr);

    hop_cof_prev = f->hop_cof[0].base_hop_cof + f->hop_cof[0].hop_trust_cof;
    hop_cof      = f->hop_cof[1].base_hop_cof + f->hop_cof[1].hop_trust_cof;

    ratio  = f->enrg_time[1] / f->enrg_time[0];
    ratio1 = f->enrg_time[1] / thr->silence__est_thr;
    /*LLZ_PRINT(">>> ratio=%lf, ratio1=%lf, enrg_start_thr=%lf\n", ratio, ratio1,thr->enrg__start_thr);*/

    is_silence = 1;
    is_hot_pitch = 0;

    switch (f->cur_status) {
        case 0:
            if (f->enrg_time[1] >= thr->enrg__start_thr && 
                hop_cof         >= thr->hop_cof__start_thr1) {

                if (f->status_keep_cnt >= thr->status_keep_cnt__thr || 
                    hop_cof            >= thr->hop_cof__start_thr2  || ratio1 > 2*ENRG_RATIO_THR) {
                    f->cur_status      = 1;
                    f->status_keep_cnt = 0;

                    is_silence         = 0;
                    is_hot_pitch       = 1;
                } else {
                    f->status_keep_cnt++;
                }
            } else {
                if (f->status_keep_cnt > 0)
                    f->status_keep_cnt--;

                /*if big ratio happen, we assume it will change to pitch status and update the start threshold*/
                if (ratio > ENRG_RATIO_THR || ratio1 > ENRG_RATIO_THR) {
                    is_silence = 0;
                    thr->enrg__start_thr = 0.8 * thr->enrg__start_thr  + 0.2 * f->enrg_time[1];
                    thr->enrg__stop_thr  = 2 * thr->enrg__start_thr;
                } else {
                    is_silence = 1;
                }

                is_hot_pitch = 0;
            }
            break;
        case 1:
            if ((ratio   < thr->ratio__stop_thr && 
                 hop_cof < thr->hop_cof__stop_thr1) || 
                 ratio < 0.1) {

                if (f->status_keep_cnt >= thr->status_keep_cnt__thr || ratio < 0.1 ||
                    (hop_cof < thr->hop_cof__stop_thr2 && f->enrg_time[1] < thr->enrg__stop_thr)) {

                    f->cur_status      = 0;
                    f->status_keep_cnt = 0;

                    is_silence         = 1;
                    is_hot_pitch       = 0;
                } else {
                    f->status_keep_cnt++;
                }
            } else {
                if (f->status_keep_cnt > 0)
                    f->status_keep_cnt--;

                is_silence   = 0;
                is_hot_pitch = 1;
            }

            if (ratio1 > ENRG_RATIO_THR)
                is_silence = 0;

            break;
    }

    if (f->cur_status == 1) {
        /*check the pitch transition status*/
        if (f->hop_cof[1].base_hop_cof  > thr->base_hop_cof__trans_thr && 
            f->hop_cof[1].hop_trust_cof < thr->hop_trust_cof__trans_thr) {
            int flag = 0;

            /*last time maybe is pitch status, but this frame base hop changed, so we should check*/
            if (f->enrg_time[0] >= thr->enrg__start_thr && hop_cof_prev >= 1.4 && 
                f->hop_cof[1].base_hop != f->hop_cof[0].base_hop) {
                flag = 1;

                is_silence   = 0;
                is_hot_pitch = 0;
            }

            /*if base_hop change, maybe transition status*/
            if (flag == 0 && f->hop_cof[1].base_hop != f->hop_cof[0].base_hop) {
                is_silence   = 0;
                is_hot_pitch = 0;
            }
        }
    }

    if (is_silence) 
        thr->silence__est_thr = 0.8 * thr->silence__est_thr + 0.2 * f->enrg_time[1]; 

    note_group_addframe(f, is_hot_pitch, is_silence);
}

static double calculate_variance(double *sample, double mean, int len)
{
    int i;
    double var;
    double x;

    var = 0.0;

    for (i = 0; i < len; i++) {
        x = sample[i] - mean;
        var += x * x;
    }

    /*return var/len;*/
    return var;
}

static void analysis_enrg(llz_musicpitch_t *f)
{
    double mean;

    f->enrg_time[1] = llz_shortenrg2(f->sample_buf, f->frame_len);
    mean            = f->enrg_time[1]/f->frame_len;

    f->zrc[1]       = llz_zrc(f->sample_buf, f->frame_len);

    f->variance[1]  = calculate_variance(f->sample_buf, mean, f->frame_len);

    /*LLZ_PRINT(">>> curtime=%lf, enrg=%lf, mean=%lf\n", f->frame_index*f->frame_duration, f->enrg_time[1], mean);*/
}

void update_feature(llz_musicpitch_t *f)
{
    int i;

    /*frequency domain enrg*/
    f->enrg_freq[0] = f->enrg_freq[1];

    /*time domain enrg*/
    f->enrg_time[0] = f->enrg_time[1];
    f->zrc[0]       = f->zrc[1];
    f->variance[0]  = f->variance[1];

    /*hop info*/
    memcpy(&(f->hop_cof[0]), &(f->hop_cof[1]), sizeof(hop_cof_t));
    for (i = 0; i < TONE_CNT_MAX; i++) 
        memcpy(&(f->hop_info[0][i]), &(f->hop_info[1][i]), sizeof(hop_info_t));
    f->hop_info_cnt[0] = f->hop_info_cnt[1];
}


void llz_musicpitch_analysis(unsigned long handle, double *sample)
{
    int i, k;

    double val;
    double re, im;

    llz_musicpitch_t *f     = (llz_musicpitch_t *)handle;
    llz_psychomodel1_t *psy = f->psy;
    note_group_t *s;

    s    = &(f->note_grp);
 
    f->frame_index++;

    /*move sample buf, update current analysis buffer*/
    for (i = 0; i < psy->fft_len - psy->fft_len/OVERLAP_COF; i++) 
        f->sample_buf[i] = f->sample_buf[i+psy->fft_len/OVERLAP_COF];
    for (i = 0; i < psy->fft_len/OVERLAP_COF; i++) 
        f->sample_buf[i+psy->fft_len-psy->fft_len/OVERLAP_COF] = sample[i];

    f->time_pitch_contour[s->notegrp_frame_cnt] = f->fs / llz_pitch_getpitch(f->h_pitch, sample);

    if (f->iir_flag) {
        llz_iir_filter(f->h_iir, f->sample_buf, f->sample_flt_buf, psy->fft_len);

        /*calculate psd and power enrg*/
        for (i = 0; i < psy->fft_len; i++) {
            val = f->sample_flt_buf[i] * f->win[i];
            f->sample_flt_buf[i]  = val;
            f->fft_buf[i+i]   = val;
            f->fft_buf[i+i+1] = 0.0;
        }
    } else {
        /*calculate psd and power enrg*/
        for (i = 0; i < psy->fft_len; i++) {
            val = f->sample_buf[i] * f->win[i];
            f->sample_buf[i]  = val;
            f->fft_buf[i+i]   = val;
            f->fft_buf[i+i+1] = 0.0;
        }
    }

    /*time to frequency*/
    llz_fft(f->h_fft, f->fft_buf);

    /*psd estimate*/
    f->enrg_freq[1] = 0.0;
    for(k = 0; k < psy->psd_len; k++) {
        re = f->fft_buf[k+k];
        im = f->fft_buf[k+k+1];

        psy->psd[k]     =  psd_estimate2(re, im, &(f->power[k]));
        f->enrg_freq[1] += f->power[k];
    }


    /*calculate the main feature of this freame, such as enrg, zrc, variance*/
    analysis_enrg(f);

    /*tone estimate, and find the tone */
    find_tone(f);

    /*sort the tone by power*/
    splite_tone(f);

    /*tone hop analysis and generate low frequency tone cnt and high frequency tone cnt*/
    analysis_tone(f);

    /*frequency domain hop analysis then use note time domain reanalysis */
    analysis_note(f);

    /*update each feature*/
    update_feature(f);

}

void llz_musicpitch_analysis_flush(unsigned long handle)
{
    llz_musicpitch_t *f = (llz_musicpitch_t *)handle;

    reanalysis_hot_pitch(f);
    note_group_analysis(f);

}

double llz_musicpitch_getcurtime(unsigned long handle)
{
    llz_musicpitch_t *f = (llz_musicpitch_t *)handle;

    return f->frame_index * f->frame_duration;
}





