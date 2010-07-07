#ifndef VIDEO_CAPTURE_BLACKMAGIC_H
#define VIDEO_CAPTURE_BLACKMAGIC_H

#include "video.h"

int hdviper_setup_video_capture_blackmagic(Video *);
int hdviper_capture_video_frame_blackmagic(Video *);
int hdviper_register_video_callback_blackmagic(Video *, void(*f)(Video *));
int hdviper_deregister_video_callback_blackmagic(Video *);
void hdviper_destroy_video_capture_blackmagic(Video *);

#endif

