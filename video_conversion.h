#ifndef HDVIPER_VIDEO_CONVERSION_H
#define HDVIPER_VIDEO_CONVERSION_H

/*
 * Video frame conversion functions
 */

void yuv420p_to_bgra(int width, int height, const unsigned char *src, unsigned char *dst);
void yuyv_to_bgra(int width, int height, const unsigned char *src, unsigned char *dst);

#endif
