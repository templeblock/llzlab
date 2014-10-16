noinst_PROGRAMS=llz_resample
llz_resample_SOURCES=main.c llz_parseopt.c
llz_resample_INCLUDES=llz_parseopt.h
llz_resample_LDADD=-lm -lpthread\
				   $(top_srcdir)/libllzfilter/libllzfilter.la\
				   $(top_srcdir)/libllzaudio/libllzaudio.la\
				   $(top_srcdir)/libllzutils/libllzutils.la

INCLUDES=-I$(top_srcdir)/libllzfilter\
		 -I$(top_srcdir)/libllzaudio\
		 -I$(top_srcdir)/libllzutils

