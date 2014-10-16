/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_print.h 
  time    : 2012/07/08 16:33 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/


#ifndef _LLZ_PRINT_H
#define _LLZ_PRINT_H


#ifdef __cplusplus 
extern "C"
{ 
#endif  



#include <stdio.h>  
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef WIN32
#include <io.h>
#include <direct.h>
#define W_OK 02
#endif

#ifndef bool
#define bool int
#endif

#ifdef WIN32
#undef  stderr
#define stderr stdout
#endif

#define LLZ_PRINT_MAX_LEN     1024


int llz_print_init(bool printenable, 
                  bool fileneed, bool stdoutneed, bool stderrneed, 
                  bool pidneed, 
                  const char *path, const char *fileprefix, const char *printprefix, int thr_day);
int llz_print_uninit();

/*
    when you input print msg, below tag for your reference
    [NOTE, SUCC, FAIL, WARN, TEST]
        NOTE: for normal infomation, just remind you whan happened
        SUCC: means some critical action finish successfully
        FAIL: means error happened
        WARN: warning, not fail, but maybe something abnormal
        TEST: use this tag when you debug
    e.g:   llz_print("SUCC: global init succesfully\n");
           llz_print("TEST: debuging, test init\n");
           llz_print_err("FAIL: init failed\n");
*/
int llz_print(char *fmtstr, ...);
int llz_print_err(char *fmtstr, ...);

#define LLZ_PRINT_ENABLE                1
#define LLZ_PRINT_FILE_ENABLE           1
#define LLZ_PRINT_STDOUT_ENABLE         1
#define LLZ_PRINT_STDERR_ENABLE         1
#define LLZ_PRINT_PID_ENABLE            1

#define LLZ_PRINT_DISABLE               0 
#define LLZ_PRINT_FILE_DISABLE          0 
#define LLZ_PRINT_STDOUT_DISABLE        0 
#define LLZ_PRINT_STDERR_DISABLE        0  
#define LLZ_PRINT_PID_DISABLE           0 


#define USE_LLZ_PRINT         

#ifdef USE_LLZ_PRINT
    #ifdef __DEBUG__
    #define LLZ_PRINT_INIT       llz_print_init
    #define LLZ_PRINT_UNINIT     llz_print_uninit
    #define LLZ_PRINT            llz_print
    #define LLZ_PRINT_ERR        llz_print_err
    #define LLZ_PRINT_DBG        llz_print
    #else
    #define LLZ_PRINT_INIT       llz_print_init
    #define LLZ_PRINT_UNINIT     llz_print_uninit
    #define LLZ_PRINT            llz_print
    #define LLZ_PRINT_ERR        llz_print_err
    #ifdef __GNUC__
    #define LLZ_PRINT_DBG(...)   
    #else 
    #define LLZ_PRINT_DBG   
    #endif
    #endif
#else
    #ifdef __DEBUG__
    #ifdef __GNUC__
    #define LLZ_PRINT_INIT(...)      
    #define LLZ_PRINT_UNINIT(...)     
    #else 
    #define LLZ_PRINT_INIT      
    #define LLZ_PRINT_UNINIT     
    #endif
    #define LLZ_PRINT            printf 
    #define LLZ_PRINT_ERR        printf 
    #define LLZ_PRINT_DBG        printf 
    #else
    #ifdef __GNUC__
    #define LLZ_PRINT_INIT(...)      
    #define LLZ_PRINT_UNINIT(...)     
    #else 
    #define LLZ_PRINT_INIT      
    #define LLZ_PRINT_UNINIT     
    #endif
    #define LLZ_PRINT            printf 
    #define LLZ_PRINT_ERR        printf 
    #ifdef __GNUC__
    #define LLZ_PRINT_DBG(...)   
    #else 
    #define LLZ_PRINT_DBG   
    #endif
    #endif
#endif




#ifdef __cplusplus 
}
#endif  




#endif
