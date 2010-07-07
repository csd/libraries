
/*
 *  This is the video capture API for the Media Services of the 
 *  HDVIPER project.
 */

#include <stdio.h>
#include <stdlib.h>
#include "video_capture.h"
#ifdef HDVIPER_VIDEO_CAPTURE_MATROX
#include "video_capture_matrox.h"
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
#include "video_capture_blackmagic.h"
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
#include "video_capture_v4l.h"
#endif
#include "rtp.h"

/*
 * Set up the video device for capturing, allocate necessary memory, etc.
 */
int hdviper_setup_video_capture(Video *v) {
  int i;

  /* allocate video buffers, etc */
  v->fps = 25;
  if(v->width == 0 || v->height == 0) {
    /* default video deminsions are 1920x1280 */
    v->width = 1920;
    v->height = 1280;
  }
  v->yuv = (unsigned char *)malloc(v->width*v->height*3);
  v->rgb = (unsigned char *)malloc(v->width*v->height*4);

  /* this is the default, can be changed by the device-specific setup fcns */
  v->format = HDVIPER_VIDEO_FORMAT_YCbCr_411;

  v->rtp_pkt = (char *)malloc(1500);
  v->rtp_seq = 0;
  v->rtp_pt = HDVIPER_RTP_PT_H264;

#ifdef HDVIPER_VIDEO_CAPTURE_MATROX
  i = hdviper_setup_video_capture_matrox(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
  i = hdviper_setup_video_capture_blackmagic(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
  i = hdviper_setup_video_capture_v4l(v);
#endif

  return i;
}

/*
 * Grab a video frame from the capture board
 */
int hdviper_capture_video_frame(Video *v) {
  int i;

#ifdef HDVIPER_VIDEO_CAPTURE_MATROX
  i = hdviper_capture_video_frame_matrox(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
  i = hdviper_capture_video_frame_blackmagic(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
  i = hdviper_capture_video_frame_v4l(v);
#endif

  return i;
}

/*
 * Register a claaback function to be called whenever a new frame is available
 * from the video capture board.
 */
int hdviper_register_video_callback(Video *v, void(*f)(Video *)) {
  int i;

#ifdef HDVIPER_VIDEO_CAPTURE_MATROX
  i = hdviper_register_video_callback_matrox(v, f);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
  i = hdviper_register_video_callback_blackmagic(v, f);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
  i = hdviper_register_video_callback_v4l(v, f);
#endif

  return i;
}

/*
 * Deregister video callback
 */
int hdviper_deregister_video_callback(Video *v) {
  int i;
#ifdef HDVIPER_VIDEO_CAPTURE_MATROX
  i = hdviper_deregister_video_callback_matrox(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
  i = hdviper_deregister_video_callback_blackmagic(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
  i = hdviper_deregister_video_callback_v4l(v);
#endif

  return i;
}

/*
 * Free all memory allocated by the setup function and close capture device
 */
void hdviper_destroy_video_capture(Video *v) {
#ifdef HDVIPER_VIDEO_CAPTURE_MATROX
  hdviper_destroy_video_capture_matrox(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
  hdviper_destroy_video_capture_blackmagic(v);
#endif
#ifdef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
  hdviper_destroy_video_capture_v4l(v);
#endif
  free(v->rtp_pkt);
  free(v->yuv);
  free(v->rgb);
}


