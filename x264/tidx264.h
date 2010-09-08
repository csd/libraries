/*****************************************************************************
 * h264.h: h264 encoder library
 *****************************************************************************
 *****************************************************************************/

#ifndef H264_H264_H
#define H264_H264_H

#if !defined(_STDINT_H) && !defined(_STDINT_H_) && \
    !defined(_INTTYPES_H) && !defined(_INTTYPES_H_)
# ifdef _MSC_VER
#  pragma message("You must include stdint.h or inttypes.h before h264.h")
# else
#  warning You must include stdint.h or inttypes.h before h264.h
# endif
#endif

#include <stdarg.h>
#include <stdint.h>


/*      opaque handler for encoder */
typedef struct tidx264_t tidx264_t;
/*      opaque handler for encoder */
typedef struct tidx264_param_t tidx264_param_t;

/****************************************************************************
 * Picture structures and functions.
 ****************************************************************************/
typedef struct
{
    int     i_csp;

    int     i_plane;
    int     i_stride[4];
    uint8_t *plane[4];
} tidx264_image_t;

typedef struct
{
    /* In: force picture type (if not auto) XXX: ignored for now
     * Out: type of the picture encoded */
    int     i_type;
    /* In: force quantizer for > 0 */
    int     i_qpplus1;
    /* In: user pts, Out: pts of encoded picture (user)*/
    int64_t i_pts;

    /* In: raw data */
    tidx264_image_t img;
} tidx264_picture_t;

typedef struct _TIDX264Enc TIDX264Enc;
 
struct _TIDX264Enc
{
  tidx264_t *h264enc;
  tidx264_param_t *h264param;

  unsigned int threads;
  unsigned int slices;
  unsigned int deblock;
  unsigned int maxvbr;
  unsigned int pass;
  int byte_stream;
  unsigned int bitrate;
  unsigned int framerate;
  int me;
  unsigned int subme;
  unsigned int analyse;
  int dct8x8;
  unsigned int ref;
  unsigned int bframes;
  int b_pyramid;
  int weightb;
  unsigned int sps_id;
  int trellis;
  unsigned int vbv_buf_capacity;
  unsigned int keyint_max;
  int cabac;

  unsigned int width, height;
  unsigned int stride, luma_plane_size;
  int framerate_num, framerate_den;

  unsigned char *buffer;
  unsigned long buffer_size;
  int *i_nal;
  
};


void tidx264_enc_init_frame (tidx264_picture_t * pic_in, char * buf, int width, int height);

int tidx264_enc_init_encoder (TIDX264Enc *encoder, int profile);
int tidx264_enc_encode_frame (TIDX264Enc *encoder, tidx264_picture_t *pic_in, char *buffer, int *buffersize);
void tidx264_enc_close_encoder (TIDX264Enc *encoder);

#endif
