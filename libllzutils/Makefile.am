llzutils_src=getopt.c llz_print.c llz_sort.c llz_clock.c

lib_LTLIBRARIES=libllzutils.la 
libllzutils_la_SOURCES= $(llzutils_src)
libllzutils_la_LIBADD=-lm -lpthread
libllzutils_la_includedir=$(includedir)/libllzutils
libllzutils_la_include_HEADERS=getopt.h llz_print.h llz_sort.h llz_clock.h

