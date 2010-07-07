#
# Makefile for HDVIPER Media Service library
#

# ------------------------------------------------------------------------
#       C Compiler options 
# ------------------------------------------------------------------------

VERSION	     = 0.1
DEFINES      = -D HDVIPER_AUDIO_CAPTURE_ALSA -D HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
CFLAGS       = -g
#CFLAGS       = -O2
CC 	     = gcc
AR	     = ar
ARFLAGS	     = cr


# ------------------------------------------------------------------------
#       Include directives
# ------------------------------------------------------------------------

SPEEX_INCLUDE   = -I speex/include
X264_INCLUDE    = -I./x264


# ------------------------------------------------------------------------
#       Libraries directives 
# ------------------------------------------------------------------------

LIB	     = -L/usr/lib 
EXTRA_LIBS   = -L/usr/local/lib
SPEEX_LIB    = -Lspeex/libspeex/.libs -lspeex
ALSA_LIB     = -lasound
X264_LIB     = ./x264/libtidx264.a


# ------------------------------------------------------------------------
#       Don't edit anything beyond this point
# ------------------------------------------------------------------------

INCLUDES     = -I. ${SPEEX_INCLUDE} $(X264_INCLUDE)
CC_SWITCHES  = $(CFLAGS) $(DEFINES) $(INCLUDES)
LIBRARIES    = $(LIB) $(EXTRA_LIBS) ${SPEEX_LIB} $(ALSA_LIB) $(X264_LIB)

OBJECTS      = audio_capture.o audio_codec.o packetize.o video_capture.o \
		video_codec.o audio_capture_alsa.o video_capture.o \
		video_conversion.o packetize_h264.o rtp.o

ifeq (HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX,$(findstring HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX,$(DEFINES)))
OBJECTS += video_capture_v4l.o
endif

ifeq (HDVIPER_VIDEO_CAPTURE_MATROX,$(findstring HDVIPER_VIDEO_CAPTURE_MATROX,$(DEFINES)))
OBJECTS += video_capture_matrox.o
endif

RANLIB 	     = ranlib
SHELL 	     = /bin/sh
RM 	     = rm -f
CP	     = cp

all: libspeex hdviper-ms snake

libspeex:	speex/libspeex/.libs/libspeex.a

speex/libspeex/.libs/libspeex.a:
	cd speex && ./configure && make

hdviper-ms:  $(OBJECTS)
	@echo "building HDVIPER Media Services lib"
	$(AR) $(ARFLAGS) hdviper-ms.lib $(OBJECTS)
	$(RANLIB) hdviper-ms.lib
	@echo "make finished on `date`"

snake:  snake.o hdviper-ms
	$(CC) $(CC_SWITCHES) -o snake snake.o hdviper-ms.lib ${LIBRARIES}

%.o:	%.c
	$(CC) -c $(CC_SWITCHES) $<

clean:
	rm *.o hdviper-ms.lib snake
