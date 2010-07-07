#ifndef VIDEO_CODEC_H
#define VIDEO_CODEC_H


#include <stdint.h>
#if !defined(_STDINT_H) && !defined(_STDINT_H_) && \
    !defined(_INTTYPES_H) && !defined(_INTTYPES_H_)
# ifdef _MSC_VER
#  pragma message("You must include stdint.h or inttypes.h before h264.h")
# else
#  warning You must include stdint.h or inttypes.h before h264.h
# endif
#endif

#include <stdarg.h>
#include "../video.h"
#include <tidx264.h>

#define VIDEO_CODEC_H264 1

typedef struct {
    int codec_type;
    void *object;          /* Encoder instance */
    unsigned int bitrate;  /* In kbps */
} VideoCodec;

int hdviper_setup_video_encoder(VideoCodec *, int, Video *);
int hdviper_video_encode(VideoCodec *, Video *);
void hdviper_destroy_video_encoder(VideoCodec *);

int hdviper_setup_video_decoder(VideoCodec *, int);
int hdviper_video_decode(VideoCodec *, Video *);
void hdviper_destroy_video_decoder(VideoCodec *);

#endif
