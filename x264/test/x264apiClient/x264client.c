/* H264 encoder API
 */


#include "x264client.h"

#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <stdio.h>


/****************************************************************************
 * main:
 ****************************************************************************/
int main( int argc, char **argv )
{
    int ret = 0;
    unsigned int width = 640;
    unsigned int height = 480;
    unsigned int bitrate = 2000;
    unsigned int framerate = 25;
	FILE *yuvfile;
	FILE *file264;
	unsigned char *buffer_in;
	unsigned char *buffer_out;
	
	VideoCodec videoCodec;
	Video video;
	
	yuvfile = fopen("input.yuv", "r");
	file264 = fopen("output.264", "wb");
	
	buffer_in = (unsigned char *)malloc(sizeof(unsigned char)*width*height*3/2);
	buffer_out = (unsigned char *)malloc(sizeof(unsigned char)*bitrate*1000/8);
	
	video.width = width;
	video.height = height;
	video.fps = framerate;
	videoCodec.bitrate = bitrate;
	hdviper_setup_video_encoder(&videoCodec, VIDEO_CODEC_H264, &video);
	
	video.yuv = buffer_in;
	video.compressed = buffer_out;
	while(fread(video.yuv, sizeof(unsigned char), width*height*3/2, yuvfile))
	{
	    hdviper_video_encode(&videoCodec, &video);
	    fwrite(video.compressed, sizeof(unsigned char), video.size_out, file264);
	}
	
	hdviper_destroy_video_encoder(&videoCodec);

	fclose(yuvfile);
	fclose(file264);
	
    return ret;
}






