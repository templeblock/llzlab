/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_levinson.h 
  time    : 2012/11/17 15:19 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "llz_levinson.h"


/*
 * - r     : the input vector which defines the toeplitz matrix
 * - p     : size of the system to solve. order must be < size -1
 * - acof  : solution (ie ar coefficientss). Size must be at last p+1
 * - kcof  : reflexion coefficients. Size must be atcoefficients last equal to equal to order.
 * - err   : *prediction* errocoefficientsr (scalar)
 * - tmp   : cache, mnust have acoft least order elements
 *
 */

void llz_levinson(double *r,    int p, 
                  double *acof, double *kcof, double *err)
{
    int   i, j;
    double acc;
    double tmp[LLZ_LEVINSON_ORDER_MAX+1];
    
    memset(tmp, 0, sizeof(double)*(LLZ_LEVINSON_ORDER_MAX+1));
    if (r[0] == 0.0) {
        for (i = 1; i <= p; i++) {
            acof[i] = 0.0;
            kcof[i] = 0.0;
        }
        *err = 0.0;
    } else {
        /* order 0 */
        acof[0] = (double)1.0;
        *err    = r[0];

        /* order >= 1 */
        for (i = 1; i <= p; ++i) {
            acc = r[i];

            for ( j = 1; j <= i-1; ++j) 
                acc += acof[j]*r[i-j];

            kcof[i-1] = -acc/(*err);
            acof[i]   = kcof[i-1];

            for (j = 0; j < p; ++j) 
                tmp[j] = acof[j];

            for (j = 1; j < i; ++j) 
                acof[j] += kcof[i-1]*tmp[i-j];

            *err *= (1-kcof[i-1]*kcof[i-1]);
        }
    }
}

/* 
 * same definition with the llz_levinson, 
 * WARN: result of acof is negative with llz_levinson
*/
void llz_levinson1(double *r, int p, 
                   double *acof, double *kcof, double *err)
{
    int   i;
    int   k, s, m;
    double am1[LLZ_LEVINSON_ORDER_MAX+1];
    double km, em1, em;
    double errtmp;                    

    memset(am1, 0, sizeof(double)*(LLZ_LEVINSON_ORDER_MAX+1));
    if (r[0] == 0.0) {
        for (i = 1; i <= p; i++) {
            kcof[i] = 0.0;
            acof[i] = 0.0;
        }
    } else {
        for (k = 0; k <= p; k++) {
            acof[k] = 0.0;
            am1[k]  = 0.0; 
        }

        acof[0] = 1.0;
        am1[0]  = 1.0;
        km      = 0.0;
        em1     = r[0];
        for (m = 1; m <= p; m++) {                    //m=2:N+1
            errtmp = 0.0;                             //err = 0;

            for (k = 1; k <= m-1; k++)                //for k=2:m-1
                errtmp += am1[k] * r[m-k];            // err = err + am1(k)*r(m-k+1);

            km        = (r[m] - errtmp) / em1;        //km=(r(m)-err)/em1;
            kcof[m-1] = -km;
            acof[m]   = km;                           //am(m)=km;

            for (k = 1; k <= m-1; k++)                //for k=2:m-1
                acof[k] = am1[k] - km * am1[m-k];     // am(k)=am1(k)-km*am1(m-k+1);

            em = (1 - km * km) * em1;                 //em=(1-km*km)*em1;

            for (s = 0; s <= p; s++)                  //for s=1:N+1
                am1[s] = acof[s];                     // am1(s) = am(s)
            em1 = em;                                 //em1 = em;
        }
    }

    *err = em;
}


int  llz_atlvs(double *r, int n, double *b, 
               double *x, double *kcof, double *err)
{
    int   i, j, k;
    double a, beta, q, c, h;
    double s[LLZ_LEVINSON_ORDER_MAX];
    double y[LLZ_LEVINSON_ORDER_MAX];

    memset(s, 0, sizeof(double)*LLZ_LEVINSON_ORDER_MAX);
    memset(y, 0, sizeof(double)*LLZ_LEVINSON_ORDER_MAX);

    a = r[0];
    if (fabs(a)+1.0 == 1.0) 
        return -1;

    y[0] = 1.0; 
    x[0] = b[0]/a;
    for (k = 1; k <= n-1; k++) {
        beta = 0.0; 
        q    = 0.0;

        for (j = 0; j <= k-1; j++) { 
            beta = beta + y[j]*r[j+1];
            q    = q    + x[j]*r[k-j];
        }
        if (fabs(a)+1.0==1.0) 
            return -1;

        c         = -beta/a; 
        kcof[k-1] = c;
        s[0]      = c * y[k-1]; 
        y[k]      = y[k-1];

        if (k != 1) 
            for (i = 1; i <= k-1; i++)
                s[i] = y[i-1] + c*y[k-i-1];

        a = a + c*beta;
        if (fabs(a)+1.0==1.0) 
            return -1;

        h = (b[k] - q) / a;
        for (i = 0; i <= k-1; i++) { 
            x[i] = x[i] + h*s[i]; 
            y[i] = s[i];
        }

        x[k] = h * y[k];
    }

    *err = a;

    return 0;
}

