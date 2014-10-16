#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([llzlab], [1.0], [luolongzhi@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([example/llz_resample/llz_parseopt.c])
AC_CONFIG_HEADERS([config.h])

LT_INIT

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h memory.h stdint.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor memset mkdir pow sqrt])

AC_OUTPUT([Makefile libllzaudio/Makefile libllzfilter/Makefile libllzutils/Makefile\
           example/Makefile\
           example/llz_resample/Makefile])
