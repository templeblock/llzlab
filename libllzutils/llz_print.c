/*
  llzlab - luolongzhi algorithm lab 
  Copyright (C) 2013 luolongzhi 罗龙智 (Chengdu, China)

  This program is part of llzlab, all copyrights are reserved by luolongzhi. 

  filename: llz_print.c 
  time    : 2012/07/08 16:33 
  author  : luolongzhi ( luolongzhi@gmail.com )
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> 
#include <math.h>
#ifdef __GNUC__
#include <strings.h> 
#include <dirent.h> 
#endif
#include <assert.h>
#include <stdarg.h>
#ifdef __GNUC__
#include <pthread.h>
#else 
#include <windows.h>
#include <process.h>
#endif

#include "llz_print.h"

#define LOGFILE_NAME_LEN    512
#define LOGPREFIX_LEN       128 

typedef struct _logfile_t{

	FILE* fp_logfile;				        //the file pointer which point the log file

	char str_file[LOGFILE_NAME_LEN];	    //the logfile name

	char str_prefix[LOGPREFIX_LEN];	        //the prefix of the logfile name

    char str_printprefix[LOGPREFIX_LEN];    //log info prefix

	char log_dir[LOGFILE_NAME_LEN];	        //log file directory

}logfile_t;


static void open_logfile(logfile_t *logfile, const char *str_prefix, const char *str_printprefix);

static int  set_logdir(logfile_t *logfile, const char* str_logdir);

static void close_logfile(logfile_t *logfile);

static int  write_log(logfile_t *logfile, const char* str_logmsg, bool pidneed, bool fileneed, FILE *stdfile);


static bool         _printenable = 1;
static bool         _fileneed    = 0;
static bool         _stdoutneed  = 1;
static bool         _stderrneed  = 1;
static bool         _pidneed     = 1;
static logfile_t    _logfile;
static int          _thr_day     = 10;     //delete the log file before or after thr_day according to current date

#ifdef __GNUC__
static pthread_mutex_t  print_mutex;
#else 
static CRITICAL_SECTION  print_mutex;	
#endif

static int _llz_vprintf(char *fmtstr, va_list valist);
static int _llz_err_vprintf(char *fmtstr, va_list valist);

int llz_print_init(bool printenable, 
                  bool fileneed, bool stdoutneed, bool stderrneed, 
                  bool pidneed, 
                  const char *path, const char *fileprefix, const char *printprefix, int thr_day)
{
    _printenable  = printenable;
    _fileneed     = fileneed;
    _stdoutneed   = stdoutneed;
    _stderrneed   = stderrneed;
    _pidneed      = pidneed;
    _thr_day      = thr_day;

    open_logfile(&_logfile, fileprefix, printprefix);
    if(_fileneed) {
        set_logdir(&_logfile, path);
    }

#ifdef __GNUC__
    pthread_mutex_init(&print_mutex, NULL);
#else 
    InitializeCriticalSection(&print_mutex);
#endif

    return 0;
}

int llz_print_uninit()
{
    if(_fileneed)
        close_logfile(&_logfile);

    return 0;
}

static int _llz_vprintf(char *fmtstr, va_list valist)
{
    char logmsg[LLZ_PRINT_MAX_LEN];
    int ret;

    if(!_printenable) return 0;

    vsprintf(logmsg, fmtstr, valist);
    if(_stdoutneed)
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, stdout);
    else
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, NULL);
    if (ret<0) return -1;

    return 0;
}


static int _llz_err_vprintf(char *fmtstr, va_list valist)
{
    char logmsg[LLZ_PRINT_MAX_LEN];
    int ret;

    if(!_printenable) return 0;

    vsprintf(logmsg, fmtstr, valist);
    if(_stderrneed)
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, stderr);
    else
        ret = write_log(&_logfile, logmsg, _pidneed, _fileneed, NULL);

    if (ret<0) return -1;

   return 0;
}

int llz_print(char *fmtstr, ...)
{   
    va_list valist; 

#ifdef __GNUC__
    pthread_mutex_lock(&print_mutex);
#else 
    EnterCriticalSection(&print_mutex);
#endif

    va_start(valist, fmtstr ); 
    _llz_vprintf(fmtstr, valist);
    va_end(valist); 

#ifdef __GNUC__
    pthread_mutex_unlock(&print_mutex);
#else 
    LeaveCriticalSection(&print_mutex);
#endif

    return 0;
}

int llz_print_err(char *fmtstr, ...)
{   
    va_list valist; 

#ifdef __GNUC__
    pthread_mutex_lock(&print_mutex);
#else 
    EnterCriticalSection(&print_mutex);
#endif

    va_start(valist, fmtstr ); 
    _llz_vprintf(fmtstr, valist);
    va_end(valist); 

#ifdef __GNUC__
    pthread_mutex_unlock(&print_mutex);
#else 
    LeaveCriticalSection(&print_mutex);
#endif

    return 0;
}


static void open_logfile(logfile_t *logfile, const char *str_prefix, const char *str_printprefix)
{
	logfile->fp_logfile = NULL;
	if(str_prefix)
		strcpy(logfile->str_prefix,str_prefix);
	else 
		memset(logfile->str_prefix,0,sizeof(logfile->str_prefix));

    if(str_printprefix)
        strcpy(logfile->str_printprefix, str_printprefix);
    else
        memset(logfile->str_printprefix, 0, sizeof(logfile->str_printprefix));

	memset(logfile->str_file,0,sizeof(logfile->str_file));
	memset(logfile->log_dir,0,sizeof(logfile->log_dir));

}

static int set_logdir(logfile_t *logfile, const char* str_logdir)
{
#ifdef __GNUC__
	if(access(str_logdir,W_OK)<0) {
		if(mkdir(str_logdir,0744)<0)
			return -1;
	}
#endif

#ifdef WIN32
	if(_access(str_logdir,W_OK)<0) {
		if(_mkdir(str_logdir)<0)
            return -1;
	}
#endif

	strcpy(logfile->log_dir,str_logdir);
#ifdef __GNUC__
	if(logfile->log_dir[strlen(logfile->log_dir)-1]!='/')
		strcat(logfile->log_dir,"/");
#else 
	if(logfile->log_dir[strlen(logfile->log_dir)-1]!='\\')
		strcat(logfile->log_dir,"\\");
#endif
	return 0;
}

static void close_logfile(logfile_t *logfile)
{
    if(logfile->fp_logfile) {
        fclose(logfile->fp_logfile);
        logfile->fp_logfile = NULL;
    }
}


static unsigned long date2sec(int year, int mon, int day, 
                              int hour, int min, int sec) 
{ 
    if(0 >= (int)(mon-=2)) {
        /*     1..12     ->     11,12,1..10     */     
        mon  += 12;     /*Puts Feb last since it has leap day*/     
        year -= 1;     
    }     

    return ((((unsigned   long)(year/4-year/100+year/400+367*mon/12+day)+year*365-719499)*24+hour)*60+min)*60+sec;   /*finally  seconds */     
} 


/*delete the log file before year-mon-day, the threshold of day is thr_day*/
static int is_outdatefile(char *filename, int year, int mon, int day, int thr_day)
{
    /* XXXXXXXX-20121121.log     */
	int  pos = 12;
	char *p1 = filename;
	char datebuf[9];  			/* yyyymmdd */

    int  year_log, mon_log, day_log;
    int  secs_log, secs;
    int  thr_sec;
    int  diff;

    memset(datebuf, 0, 9);
    thr_sec = thr_day * (3600*24);
    thr_sec = ((thr_sec >= 0) ? thr_sec : 0);
    
	while (*p1) p1++;  
    
	while(pos) {
		pos--;
		p1--;
	}

	strncpy(datebuf, p1, 8);
	datebuf[8] = '\0';
    sscanf(datebuf, "%4d%2d%2d", &year_log, &mon_log, &day_log);
    secs_log = (int)date2sec(year_log, mon_log, day_log, 0, 0, 0);
    secs     = (int)date2sec(year    , mon    , day    , 0, 0, 0);
    diff     = fabs(secs_log - secs);

    if (diff >= thr_sec)
        return 1;
    else 
        return 0;

}

#ifdef __GNUC__

static void deletefile(char *filename)
{
	remove(filename);
}


static void del_outdate_log(char *path, int year, int mon, int day, int thr_day)
{
	int     ret;
    DIR     *dir;
    char    fullpath[LOGFILE_NAME_LEN];
    char    fullname[LOGFILE_NAME_LEN];
    struct dirent   *s_dir;

    memset(fullpath, 0, LOGFILE_NAME_LEN);
    strncpy(fullpath, path, LOGFILE_NAME_LEN);
    dir = opendir(fullpath);
    while ((s_dir = readdir(dir)) != NULL)   {
        if (strncmp(s_dir->d_name, ".", 1) == 0)// || 
            /*(strcmp(s_dir->d_name, "..") == 0))*/
            continue;
        ret = is_outdatefile(s_dir->d_name, year, mon, day, thr_day);
		if (ret) {
            memset(fullname, 0, LOGFILE_NAME_LEN);
            snprintf(fullname, LOGFILE_NAME_LEN, "%s/%s", path, s_dir->d_name);
            deletefile(fullname);
        }
    }
    closedir(dir);	

}

#else 

static int IsDirectory(DWORD attr)
{
    return (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static int IsHidden(DWORD attr)
{
    return (attr & FILE_ATTRIBUTE_HIDDEN);

}

static void deletefile(char *filename)
{
    DeleteFile(filename);
}

static void del_outdate_log(char *path, int year, int mon, int day, int thr_day)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    DWORD attr;

	int     ret;
    char    fullpath[LOGFILE_NAME_LEN];
    char    fullname[LOGFILE_NAME_LEN];

    memset(fullpath, 0, LOGFILE_NAME_LEN);
    _snprintf(fullpath, LOGFILE_NAME_LEN, "%s%s", path, "\\*.*");


    hFind = FindFirstFile(fullpath, &FindFileData);     
    if (hFind == INVALID_HANDLE_VALUE) {
        return -1;
    } else {
        attr = FindFileData.dwFileAttributes;

        if (strncmp(FindFileData.cFileName, ".", 1) != 0) {
            if (!IsDirectory(attr) && !IsHidden(attr)) {
                ret = is_outdatefile(FindFileData.cFileName, year, mon, day, thr_day);
                if (ret) {
                    memset(fullname, 0, LOGFILE_NAME_LEN);
                    _snprintf(fullname, LOGFILE_NAME_LEN, "%s\\%s", path, FindFileData.cFileName);
                    deletefile(fullname);
                }
            }
        }
    }

    while(1) {
        ret = FindNextFile(hFind, &FindFileData);     

        if (ret) {
            if (strncmp(FindFileData.cFileName, ".", 1) == 0) 
                continue;
            attr = FindFileData.dwFileAttributes;
            if (!IsDirectory(attr) && !IsHidden(attr)) {
                ret = is_outdatefile(FindFileData.cFileName, year, mon, day, thr_day);
                if (ret) {
                    memset(fullname, 0, LOGFILE_NAME_LEN);
                    _snprintf(fullname, LOGFILE_NAME_LEN, "%s\\%s", path, FindFileData.cFileName);
                    deletefile(fullname);
                }
            }
        } else {
            break;
        }
    }

    FindClose(hFind);
}

#endif

static int write_log(logfile_t *logfile, const char* str_logmsg, bool pidneed, bool fileneed, FILE *stdfile)
{
	char strTime[50];
	char strYear[5], strMonth[3], strDay[3],strDate[9];
	char strFile[LOGFILE_NAME_LEN];
	time_t ctime;
	struct tm *ptm;

    if(fileneed) {
        //if no assigned directory ,using ./log/
        if(strlen(logfile->log_dir)==0) {
            strcpy(logfile->log_dir,"./defaultlog/");
            #ifdef __GNUC__
            if(access(logfile->log_dir,W_OK)<0) {
                if(mkdir(logfile->log_dir,0744)<0)
                    return -1;
            }
            #endif

            #ifdef WIN32
            if(_access(logfile->log_dir,W_OK)<0) {
                if(_mkdir(logfile->log_dir)<0)
                    return -1;

            }
            #endif
        }
    }

	//get the system time
	time(&ctime);
	ptm = localtime( &ctime );
	strftime( strYear, 5, "%Y", ptm );
	strftime( strMonth, 3, "%m", ptm );
	strftime( strDay, 3, "%d", ptm );
	sprintf(strDate, "%s%s%s", strYear, strMonth, strDay);

    if(fileneed) {
        //generate the file name
        #ifdef __GNUC__
	    snprintf(strFile,sizeof(strFile), "%s%s-localtime-%s.log",logfile->log_dir,logfile->str_prefix,strDate);
        #endif

        #ifdef WIN32
    	_snprintf(strFile,sizeof(strFile),"%s%s-localtime-%s.log",logfile->log_dir,logfile->str_prefix,strDate);
        #endif

        //compare the filelen
        if((strlen(logfile->str_file)==0) || (strcmp(logfile->str_file,strFile))) {
            del_outdate_log(logfile->log_dir, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, _thr_day);
            strcpy(logfile->str_file,strFile);
            logfile->fp_logfile = fopen(strFile,"a");
            if(!logfile->fp_logfile)return -1;
        }else {
            logfile->fp_logfile = fopen(strFile,"a");
            if(!logfile->fp_logfile)return -1;
        }

    }
   
	//wirte the filename using the date
    sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d LOCAL", 
            ptm->tm_year+1900,
            ptm->tm_mon+1,
            ptm->tm_mday,
            ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
    if(pidneed) {
        if(stdfile)
#ifdef __GNUC__
            fprintf(stdfile, "#[%s][%s][P%d]#\t", strTime, logfile->str_printprefix, getpid());
#else 
            fprintf(stdfile, "#[%s][%s][P%d]#\t", strTime, logfile->str_printprefix, _getpid());
#endif
        if(fileneed)
#ifdef __GNUC__
            fprintf( logfile->fp_logfile, "#[%s][%s][P%d]#\t", strTime, logfile->str_printprefix, getpid());
#else 
            fprintf( logfile->fp_logfile, "#[%s][%s][P%d]#\t", strTime, logfile->str_printprefix, _getpid());
#endif
    }else {
        if(stdfile)
            fprintf(stdfile, "#[%s][%s]#\t", strTime, logfile->str_printprefix);
        if(fileneed)
            fprintf( logfile->fp_logfile, "#[%s][%s]#\t", strTime, logfile->str_printprefix );
    }

	//write msg
    if(stdfile)
        fprintf(stdfile, "%s", str_logmsg );

    if(fileneed) {
        fprintf( logfile->fp_logfile, "%s", str_logmsg );
        fflush( logfile->fp_logfile);
        if(logfile->fp_logfile) {
            fclose(logfile->fp_logfile);
            logfile->fp_logfile = NULL;
        }
    }

	return 0;

}




