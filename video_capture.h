#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "video.h"

int hdviper_setup_video_capture(Video *);
int hdviper_capture_video_frame(Video *);
int hdviper_register_video_callback(Video *, void(*f)(Video *));
int hdviper_deregister_video_callback(Video *);
void hdviper_destroy_video_capture(Video *);

#endif

