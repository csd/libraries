/*****************************************************************************
 * h264: h264 encoder testing program.
 *****************************************************************************
 *****************************************************************************/

#include "x264.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>


#ifndef _MSC_VER
#include "config.h"
#endif


enum
{
  ARG_0,
  ARG_SLICES,
  ARG_BITRATE,
};

#define ARG_THREADS_DEFAULT 2
#define ARG_SLICES_DEFAULT 1
#define ARG_DEBLOCKING_DEFAULT 0
#define ARG_MAXVBR_DEFAULT 1000
#define ARG_PASS_DEFAULT 0
#define ARG_BYTE_STREAM_DEFAULT 1
#define ARG_BITRATE_DEFAULT (2 * 1024)
#define ARG_VBV_BUF_CAPACITY_DEFAULT 600
#define ARG_ME_DEFAULT X264_ME_HEX
#define ARG_SUBME_DEFAULT 1
#define ARG_ANALYSE_DEFAULT 0
#define ARG_DCT8x8_DEFAULT 0
#define ARG_REF_DEFAULT 1
#define ARG_BFRAMES_DEFAULT 0
#define ARG_B_PYRAMID_DEFAULT 0
#define ARG_WEIGHTB_DEFAULT 0
#define ARG_SPS_ID_DEFAULT 0
#define ARG_TRELLIS_DEFAULT 1
#define ARG_KEYINT_MAX_DEFAULT 0
#define ARG_CABAC_DEFAULT 1


static inline int x264_clip3( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}

/* initialize the new element
 * instantiate pads and add them to element
 * set functions
 * initialize structure
 */
void
tidx264_enc_init (TIDX264Enc * encoder)
{
  /* initialize internals */
  encoder->h264enc = NULL;

  encoder->threads = ARG_THREADS_DEFAULT;
  encoder->slices = ARG_SLICES_DEFAULT;
  encoder->maxvbr = ARG_MAXVBR_DEFAULT;
  encoder->pass = ARG_PASS_DEFAULT;
  encoder->byte_stream = ARG_BYTE_STREAM_DEFAULT;
//  encoder->bitrate = ARG_BITRATE_DEFAULT;
  encoder->framerate_num = encoder->framerate;
  encoder->framerate_den = 1;
  encoder->vbv_buf_capacity = ARG_VBV_BUF_CAPACITY_DEFAULT;
  encoder->me = ARG_ME_DEFAULT;
  encoder->subme = ARG_SUBME_DEFAULT;
  encoder->analyse = ARG_ANALYSE_DEFAULT;
  encoder->dct8x8 = ARG_DCT8x8_DEFAULT;
  encoder->ref = ARG_REF_DEFAULT;
  encoder->bframes = ARG_BFRAMES_DEFAULT;
  encoder->b_pyramid = ARG_B_PYRAMID_DEFAULT;
  encoder->weightb = ARG_WEIGHTB_DEFAULT;
  encoder->deblock = ARG_DEBLOCKING_DEFAULT;
  encoder->sps_id = ARG_SPS_ID_DEFAULT;
  encoder->trellis = ARG_TRELLIS_DEFAULT;
  encoder->keyint_max = ARG_KEYINT_MAX_DEFAULT;
  encoder->cabac = ARG_CABAC_DEFAULT;
  
  encoder->buffer_size = 1040000;
  encoder->buffer = (unsigned char *)malloc ((sizeof(unsigned char))*encoder->buffer_size);

  encoder->h264param = (tidx264_param_t *)malloc(sizeof(x264_param_t));
  x264_param_default ((x264_param_t *)encoder->h264param);
}

/*
 * tidx264_enc_init_encoder
 * @encoder:  Encoder which should be initialized.
 *
 * Initialize tidh264 encoder.
 *
 */
int
tidx264_enc_init_encoder (TIDX264Enc * encoder, int profile)
{

  tidx264_enc_init (encoder);
  encoder->cabac = profile;
  fprintf(stderr, "TTA: Set the profile to %d\n", profile);
  
  /* set up encoder parameters */
  ((x264_param_t *)encoder->h264param)->i_threads = encoder->threads;
  ((x264_param_t *)encoder->h264param)->i_slices = encoder->slices;
  ((x264_param_t *)encoder->h264param)->i_fps_num = encoder->framerate_num;
  ((x264_param_t *)encoder->h264param)->i_fps_den = encoder->framerate_den;
  ((x264_param_t *)encoder->h264param)->i_width = encoder->width;
  ((x264_param_t *)encoder->h264param)->i_height = encoder->height;

  ((x264_param_t *)encoder->h264param)->i_keyint_max = encoder->keyint_max ? encoder->keyint_max :
      (2 * encoder->framerate_num / encoder->framerate_den);
  ((x264_param_t *)encoder->h264param)->b_cabac = encoder->cabac;
//  ((x264_param_t *)encoder->h264param)->b_aud = 1;
  ((x264_param_t *)encoder->h264param)->i_sps_id = encoder->sps_id;
  if ((((encoder->height == 576) && ((encoder->width == 720)
                  || (encoder->width == 704) || (encoder->width == 352)))
          || ((encoder->height == 288) && (encoder->width == 352)))
      && (encoder->framerate_den == 1) && (encoder->framerate_num == 25)) {
    ((x264_param_t *)encoder->h264param)->vui.i_vidformat = 1;     /* PAL */
  } else if ((((encoder->height == 480) && ((encoder->width == 720)
                  || (encoder->width == 704) || (encoder->width == 352)))
          || ((encoder->height == 240) && (encoder->width == 352)))
      && (encoder->framerate_den == 1001) && ((encoder->framerate_num == 30000)
          || (encoder->framerate_num == 24000))) {
    ((x264_param_t *)encoder->h264param)->vui.i_vidformat = 2;     /* NTSC */
  } else
    ((x264_param_t *)encoder->h264param)->vui.i_vidformat = 5;     /* unspecified */
//  ((x264_param_t *)encoder->h264param)->analyse.i_trellis = encoder->trellis ? 1 : 0;
  ((x264_param_t *)encoder->h264param)->analyse.b_psnr = 0;
  ((x264_param_t *)encoder->h264param)->analyse.b_ssim = 0;
  ((x264_param_t *)encoder->h264param)->analyse.i_me_method = encoder->me;
  ((x264_param_t *)encoder->h264param)->analyse.i_subpel_refine = encoder->subme;
  ((x264_param_t *)encoder->h264param)->analyse.inter = encoder->analyse;
  ((x264_param_t *)encoder->h264param)->analyse.b_transform_8x8 = encoder->dct8x8;
  ((x264_param_t *)encoder->h264param)->analyse.b_weighted_bipred = encoder->weightb;
  /*((x264_param_t *)encoder->h264param)->analyse.i_noise_reduction = 600; */
  ((x264_param_t *)encoder->h264param)->i_frame_reference = encoder->ref;
  ((x264_param_t *)encoder->h264param)->i_bframe = encoder->bframes;
  ((x264_param_t *)encoder->h264param)->b_bframe_pyramid = encoder->b_pyramid;
  ((x264_param_t *)encoder->h264param)->b_bframe_adaptive = 0;
  ((x264_param_t *)encoder->h264param)->b_deblocking_filter = encoder->deblock;
  ((x264_param_t *)encoder->h264param)->i_deblocking_filter_alphac0 = 0;
  ((x264_param_t *)encoder->h264param)->i_deblocking_filter_beta = 0;
#ifdef X264_RC_ABR
  ((x264_param_t *)encoder->h264param)->rc.i_rc_method = X264_RC_ABR;
#endif
  ((x264_param_t *)encoder->h264param)->rc.i_bitrate = encoder->bitrate;
  ((x264_param_t *)encoder->h264param)->rc.i_vbv_max_bitrate = encoder->bitrate;
  ((x264_param_t *)encoder->h264param)->rc.i_vbv_buffer_size
    = ((x264_param_t *)encoder->h264param)->rc.i_vbv_max_bitrate
    * encoder->vbv_buf_capacity / 1000;

  switch (encoder->pass) {
    case 0:
      ((x264_param_t *)encoder->h264param)->rc.b_stat_read = 0;
      ((x264_param_t *)encoder->h264param)->rc.b_stat_write = 0;
      break;
    case 1:
      /* Turbo mode parameters. */
      ((x264_param_t *)encoder->h264param)->i_frame_reference = (encoder->ref + 1) >> 1;
      ((x264_param_t *)encoder->h264param)->analyse.i_subpel_refine =
          x264_clip3 (encoder->subme - 1, 1, 3);
      ((x264_param_t *)encoder->h264param)->analyse.inter &= ~X264_ANALYSE_PSUB8x8;
      ((x264_param_t *)encoder->h264param)->analyse.inter &= ~X264_ANALYSE_BSUB16x16;
      ((x264_param_t *)encoder->h264param)->analyse.i_trellis = 0;

      ((x264_param_t *)encoder->h264param)->rc.b_stat_read = 0;
      ((x264_param_t *)encoder->h264param)->rc.b_stat_write = 1;
      break;
    case 2:
      ((x264_param_t *)encoder->h264param)->rc.b_stat_read = 1;
      ((x264_param_t *)encoder->h264param)->rc.b_stat_write = 1;
      break;
    case 3:
      ((x264_param_t *)encoder->h264param)->rc.b_stat_read = 1;
      ((x264_param_t *)encoder->h264param)->rc.b_stat_write = 0;
      break;
  }
  ((x264_param_t *)encoder->h264param)->rc.psz_stat_in = NULL;
  ((x264_param_t *)encoder->h264param)->rc.psz_stat_out = NULL;

  encoder->h264enc = (tidx264_t *)x264_encoder_open ((x264_param_t *)encoder->h264param);
  if (!encoder->h264enc) {
    printf ("Can not initialize x264 encoder.");
    return 0;
  }

  return 1;
}

/* tidx264_enc_close_encoder
 * @encoder:  Encoder which should close.
 *
 * Close tidh264 encoder.
 */
void
tidx264_enc_close_encoder (TIDX264Enc * encoder)
{
  free (encoder->buffer);
  encoder->buffer = NULL;
  
  if (encoder->h264enc != NULL) {
    x264_encoder_close ((x264_t *)encoder->h264enc);
    encoder->h264enc = NULL;
  }
}

void
tidx264_enc_init_frame (tidx264_picture_t * pic_in, char * buf, int width, int height)
{
  /* create tidx264_picture_t from the buffer */
  memset (pic_in, 0, sizeof (x264_picture_t));

  ((x264_picture_t *)pic_in)->img.i_csp = X264_CSP_I420;
  ((x264_picture_t *)pic_in)->img.i_plane = 3;

  /* FIXME: again, this looks wrong for odd widths/heights (tpm) */
  ((x264_picture_t *)pic_in)->img.plane[0] = (uint8_t *) buf;
  ((x264_picture_t *)pic_in)->img.i_stride[0] = width;

  ((x264_picture_t *)pic_in)->img.plane[1] = ((x264_picture_t *)pic_in)->img.plane[0] + (width * height);
  ((x264_picture_t *)pic_in)->img.i_stride[1] = width / 2;

  ((x264_picture_t *)pic_in)->img.plane[2] = ((x264_picture_t *)pic_in)->img.plane[1] + ((width * height) / 4);
  ((x264_picture_t *)pic_in)->img.i_stride[2] = width / 2;

  ((x264_picture_t *)pic_in)->img.plane[3] = NULL;
  ((x264_picture_t *)pic_in)->img.i_stride[3] = 0;

  ((x264_picture_t *)pic_in)->i_type = X264_TYPE_AUTO;
}

/* chain function
 * this function does the actual processing
 */
int
tidx264_enc_encode_frame (TIDX264Enc * encoder, tidx264_picture_t * pic_in, char *buffer, int *buffersize)
{
  x264_picture_t pic_out;
  x264_nal_t *nal;
  int i_size;
  int nal_size;
  int encoder_return;
  int i;
  int i_nal;
  unsigned char * buf_aux;


  encoder_return = x264_encoder_encode ((x264_t *)encoder->h264enc, &nal, &i_nal, (x264_picture_t *)pic_in, &pic_out);

  if (encoder_return < 0) {
    printf ("Encode h264 frame failed. tidx264encoder_encode return code=%d", encoder_return);
    return 0;
  }

  if (!i_nal) {
    *buffersize=0;
    return 1;
  }

  i_size = 0;
  for (i = 0; i < i_nal; i++) {
    int i_data = encoder->buffer_size - i_size - 4;

    if (i_data < encoder->buffer_size / 2) {
      encoder->buffer_size *= 2;
      i_data = encoder->buffer_size - i_size;
      buf_aux = (unsigned char *)malloc(sizeof(unsigned char)*encoder->buffer_size);
      memcpy(buf_aux, encoder->buffer, i_data);
      free(encoder->buffer);
      encoder->buffer = buf_aux;
    }

    nal_size =
       x264_nal_encode (encoder->buffer + i_size + 4, &i_data, 0, &nal[i]);
    if (encoder->byte_stream) {
      encoder->buffer[i_size + 0] = 0;
      encoder->buffer[i_size + 1] = 0;
      encoder->buffer[i_size + 2] = 0;
      encoder->buffer[i_size + 3] = 1;
    } else {
      encoder->buffer[i_size + 0] = (nal_size >> 24) & 0xff;
      encoder->buffer[i_size + 1] = (nal_size >> 16) & 0xff;
      encoder->buffer[i_size + 2] = (nal_size >> 8) & 0xff;
      encoder->buffer[i_size + 3] = nal_size & 0xff;
    }

    i_size += nal_size + 4;
  }

  memcpy (buffer, encoder->buffer, i_size);
  *buffersize = i_size; 

  return i_size;
}

