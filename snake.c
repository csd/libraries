/*
 * Test program for HDVIPER Media Service lib (i.e. "Snake reference 
 *  implementation")
 */

#include <stdio.h>
#include "audio.h"
#include "video.h"
#include "audio_capture.h"
#include "video_capture.h"
#include "audio_codec.h"
#include "video_codec.h"
#include "packetize.h"

/* Media data structures */
Video video;
Audio audio;
VideoCodec video_codec;
AudioCodec audio_codec;

int main(int argc, char **argv) {
  printf("Snake test application...\n");


  /* Initialize the video capture device and data structures */
  hdviper_setup_video_capture(&video);

  /* Initialize the audio capture device and data structures */
  hdviper_setup_audio_capture(&audio);

  /* Initialize the video encoder */
  hdviper_setup_video_encoder(&video_codec, VIDEO_CODEC_H264, &video);

  /* Initialize the audio encoder */
  hdviper_setup_audio_encoder(&audio_codec, HDVIPER_AUDIO_CODEC_SPEEX);

  while(1) {
    /* Capture one frame of video */
    hdviper_capture_video_frame(&video);

    /* Capture one frame of audio */
    hdviper_capture_audio_frame(&audio);

    /* Compress */
    hdviper_video_encode(&video_codec, &video);
    hdviper_audio_encode(&audio_codec, &audio);

    /* Packetize */
    hdviper_rtp_packetize_video_frame(&video);
    hdviper_rtp_packetize_audio_frame(&audio);

    /* Send ... */
  }
}
