/*
 * This is the video encoding/decoding API of the HDVIPER Media Services
 */

#include "video_codec.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>



int hdviper_setup_video_encoder(VideoCodec *c, int type, Video *v) {
    int resul = 0;

    if ( type == VIDEO_CODEC_H264 )
    {
	c->object = (TIDX264Enc *)malloc(sizeof(TIDX264Enc));
    }

    c->codec_type = type;
    ((TIDX264Enc *)c->object)->width = v->width;
    ((TIDX264Enc *)c->object)->height = v->height;
    ((TIDX264Enc *)c->object)->bitrate = c->bitrate;
    ((TIDX264Enc *)c->object)->framerate = v->fps;
    ((TIDX264Enc *)c->object)->slices = 2;

    if ( type == VIDEO_CODEC_H264 )
    {
	resul = tidx264_enc_init_encoder ((TIDX264Enc *)c->object);
    }
    return resul;
}

int hdviper_video_encode(VideoCodec *c, Video *v) {
    void * picture;
    int resul = 0;

    if ( c->codec_type == VIDEO_CODEC_H264 )
    {
	tidx264_picture_t picture;
	tidx264_enc_init_frame (&picture, v->yuv, v->width, v->height);
	resul = tidx264_enc_encode_frame ((TIDX264Enc *)c->object, (tidx264_picture_t *)&picture, v->compressed, &(v->size_out));
    }
    return resul;
}

void hdviper_destroy_video_encoder(VideoCodec *c) {
    if ( c->codec_type == VIDEO_CODEC_H264 )
    {
	tidx264_enc_close_encoder ((TIDX264Enc *)c->object);
    }
}


int hdviper_setup_video_decoder(VideoCodec *c, int type) {
}

int hdviper_video_decode(VideoCodec *c, Video *v) {
}

void hdviper_destroy_video_decoder(VideoCodec *c) {
}


