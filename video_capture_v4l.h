#ifndef VIDEO_CAPTURE_V4L_H
#define VIDEO_CAPTURE_V4L_H

#include "video.h"

struct mmap_buffer_t {
	char *start;
	size_t length;
};

typedef struct {
	int fd;     /* File descriptor */
	int pixel_format;
	unsigned int n_buffers;
	struct mmap_buffer_t *mmap_buffers;
} v4l2_data_t;

int hdviper_setup_video_capture_v4l(Video *);
int hdviper_capture_video_frame_v4l(Video *);
int hdviper_register_video_callback_v4l(Video *, void(*f)(Video *));
int hdviper_deregister_video_callback_v4l(Video *);
void hdviper_destroy_video_capture_v4l(Video *);

#endif

