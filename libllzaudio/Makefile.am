llzaudio_src=llz_midi.c llz_musicpitch.c llz_pitch2midi.c llz_psychomodel1.c llz_stenrg.c llz_wavfmt.c\
			 llz_musicnote.c llz_pitch.c llz_pitch2pcm.c llz_sine.c llz_synthcfg.c

INCLUDES=-I$(top_srcdir)/libllzfilter\
		 -I$(top_srcdir)/libllzutils

lib_LTLIBRARIES=libllzaudio.la 
libllzaudio_la_SOURCES= $(llzaudio_src)
libllzaudio_la_LIBADD=-lm -lpthread
libllzaudio_la_includedir=$(includedir)/libllzaudio
libllzaudio_la_include_HEADERS=llz_midi.h llz_musicpitch.h llz_pitch2midi.h llz_psychomodel1.h llz_stenrg.h llz_wavfmt.h\
							   llz_musicnote.h llz_pitch.h llz_pitch2pcm.h llz_sine.h llz_synthcfg.h


