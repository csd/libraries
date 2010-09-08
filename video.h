#ifndef VIDEO_H
#define VIDEO_H

/*
 * Uncompressed video formats
 */
#define HDVIPER_VIDEO_FORMAT_RESERVED 0
#define HDVIPER_VIDEO_FORMAT_YCbCr_411 1
#define HDVIPER_VIDEO_FORMAT_YCrCb_411 2
#define HDVIPER_VIDEO_FORMAT_YCbCr_422 3
#define HDVIPER_VIDEO_FORMAT_YCrCb_422 4
#define HDVIPER_VIDEO_FORMAT_RGB 5
#define HDVIPER_VIDEO_FORMAT_BGR 6
#define HDVIPER_VIDEO_FORMAT_RGBA 7
#define HDVIPER_VIDEO_FORMAT_BGRA 8

/*
 * Compressed video formats (i.e. compression algorithms)
 */
#define HDVIPER_VIDEO_CODEC_RESERVED 0
#define HDVIPER_VIDEO_CODEC_H264 1
#define HDVIPER_VIDEO_CODEC_H263 2
#define HDVIPER_VIDEO_CODEC_JPEG 3

/*
 * This is the struct keeping all data related to a video frame, such as
 * width, height, frames per second, and the actual pixel buffers.
 */

typedef struct {
  int format; /* The uncompressed video format: YCrCb, RGB, BGRA, etc... */
  unsigned char *yuv; /* uncompressed pixel values in YUV (YCrCb) colorspace */
  unsigned char *rgb; /* uncompressed pixel values in RGB colorspace */
  char *compressed; /* compressed data */
  int compression; /* compression algorithm */
  int width; /* the width of the video frame in pixels */
  int height; /* the height of the video frame in pixels */
  int fps; /* frames per second */
  /*TID*/
  int size_out;
#ifdef HDVIPER_VIDEO_CAPTURE_BLACKMAGIC
  struct ds_capture ds_cap;
#endif
  char *rtp_pkt; /* buffer for RTP packet */
  int rtp_pkt_len; /* length of RTP packet */
  unsigned short rtp_seq; /* RTP sequence number */
  unsigned int ssrc; /* SSRC identifier for the audio RTP stream */
  int rtp_pt; /* RTP payload type identifier for this video stream */
  int profile;
} Video;

#define VIDEO_DGM_LEN 1400

#endif
