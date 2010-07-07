/*
 * This is the audio encoding/decoding API of the HDVIPER Media Services
 */

#include <stdio.h>
#include "audio_codec.h"
#include "speex/speex.h"

/*
 *  Set up the Speex audio codec
 */
int hdviper_setup_speex_encoder(AudioCodec *c) {
  int i;

  speex_bits_init(&c->speex_bits); 
  c->speex_mode = speex_lib_get_mode(SPEEX_MODEID_WB);
  c->speex_state = speex_encoder_init(c->speex_mode); 
  if(!c->speex_state) {
    fprintf(stderr, "Speex encoder initialization failed.\n");
    return 0;
  }

  c->sample_rate = 16000;
  i = speex_encoder_ctl(c->speex_state, SPEEX_SET_SAMPLING_RATE, &c->sample_rate);
  if(i<0) {
    fprintf(stderr, "Speex encoder sample rate setting failed (%d).\n", i);
    return 0;
  }
  i = speex_encoder_ctl(c->speex_state, SPEEX_GET_FRAME_SIZE, &c->frame_size);
  if(i<0) {
    fprintf(stderr, "Speex encoder frame size query failed (%d).\n", i);
    return 0;
  }

  return 1;
}

/*
 *  Set up the G.711 codecs, i.e. PCMA and PCMU
 */
int hdviper_setup_g711_encoder(AudioCodec *c, int type) {

  fprintf(stderr, "G.711 audio codec not yet implemented\n");
  return 0;
}

/*
 *  Set up the audio encoder
 */
int hdviper_setup_audio_encoder(AudioCodec *c, int type) {
  int i;

  switch(type) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      i = hdviper_setup_speex_encoder(c);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      i = hdviper_setup_g711_encoder(c, HDVIPER_AUDIO_CODEC_PCMA);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      i = hdviper_setup_g711_encoder(c, HDVIPER_AUDIO_CODEC_PCMU);
      break;
    default:
      fprintf(stderr, "Unknown codec %d\n", type);
      i = 0;
  }

  c->codec_type = type;

  return i;
}

/*
 * Encode one frame of audio using the Speex codec
 */
int hdviper_speex_encode(AudioCodec *c, Audio *a) {
  speex_bits_reset(&c->speex_bits);
  speex_encode_int(c->speex_state, a->unencoded_buf, &c->speex_bits);
  a->encoded_n = speex_bits_write(&c->speex_bits, a->encoded_buf, 640);

  return a->encoded_n;
}

/*
 * Encode one frame of audio using the G.711 codec
 */
int hdviper_g711_encode(AudioCodec *c, Audio *a, int type) {

  return 0;
}

/*
 *  Encode one frame of audio
 */
int hdviper_audio_encode(AudioCodec *c, Audio *a) {
  int i;

  switch(c->codec_type) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      i = hdviper_speex_encode(c, a);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      i = hdviper_g711_encode(c, a, HDVIPER_AUDIO_CODEC_PCMA);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      i = hdviper_g711_encode(c, a, HDVIPER_AUDIO_CODEC_PCMU);
      break;
    default:
      fprintf(stderr, "Unknown codec %d\n", c->codec_type);
      i = 0;
  }
  a->encoding = c->codec_type;

  return i;
}

/*
 *  Destroy the Speex encoder
 */
void hdviper_destroy_speex_encoder(AudioCodec *c) {
  speex_bits_destroy(&c->speex_bits); 
  speex_encoder_destroy(c->speex_state);
}

void hdviper_destroy_g711_encoder(AudioCodec *c) {
}

void hdviper_destroy_audio_encoder(AudioCodec *c) {
  switch(c->codec_type) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      hdviper_destroy_speex_encoder(c);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      hdviper_destroy_g711_encoder(c);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      hdviper_destroy_g711_encoder(c);
      break;
    default:
      fprintf(stderr, "Unknown codec %d\n", c->codec_type);
  }
}


/*
 *  Set up the Speex audio decoder
 */
int hdviper_setup_speex_decoder(AudioCodec *c) {
  int i;

  speex_bits_init(&c->speex_bits); 
  c->speex_mode = speex_lib_get_mode(SPEEX_MODEID_WB);
  c->speex_state = speex_decoder_init(c->speex_mode); 
  if(!c->speex_state) {
    fprintf(stderr, "Speex decoder initialization failed.\n");
    return 0;
  }

  c->sample_rate = 16000;
  i = speex_decoder_ctl(c->speex_state, SPEEX_SET_SAMPLING_RATE, &c->sample_rate);
  if(i<0) {
    fprintf(stderr, "Speex decoder sample rate setting failed (%d).\n", i);
    return 0;
  }
  i = speex_decoder_ctl(c->speex_state, SPEEX_GET_FRAME_SIZE, &c->frame_size);
  if(i<0) {
    fprintf(stderr, "Speex decoder frame size query failed (%d).\n", i);
    return 0;
  }

  return 1;
}

/*
 *  Set up the G.711 codecs, i.e. PCMA and PCMU, for decoding
 */
int hdviper_setup_g711_decoder(AudioCodec *c, int type) {

  fprintf(stderr, "G.711 audio codec not yet implemented\n");
  return 0;
}

int hdviper_setup_audio_decoder(AudioCodec *c, int type) {
  int i;

  switch(type) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      i = hdviper_setup_speex_decoder(c);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      i = hdviper_setup_g711_decoder(c, HDVIPER_AUDIO_CODEC_PCMA);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      i = hdviper_setup_g711_decoder(c, HDVIPER_AUDIO_CODEC_PCMU);
      break;
    default:
      fprintf(stderr, "Unknown codec %d\n", type);
      i = 0;
  }

  c->codec_type = type;

  return i;
}

/*
 * Decode one frame of Speex audio
 */
int hdviper_speex_decode(AudioCodec *c, Audio *a) {
  int ret;

  speex_bits_read_from(&c->speex_bits, a->encoded_buf, a->encoded_n);
  ret = speex_decode_int(c->speex_state, &c->speex_bits, a->unencoded_buf);
  if(ret!=0) {
    fprintf(stderr, "Error decoding Speex frame: %d\n", ret);
    return 0;
  }
  return 1;
}

/*
 * Decode one frame of PCMU/PCMA audio
 */
int hdviper_g711_decode(AudioCodec *c, Audio *a, int type) {
  fprintf(stderr, "G.711 decoding not yet implemented\n");
  return 0;
}

int hdviper_audio_decode(AudioCodec *c, Audio *a) {
  int i;

  switch(c->codec_type) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      i = hdviper_speex_decode(c, a);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      i = hdviper_g711_decode(c, a, HDVIPER_AUDIO_CODEC_PCMA);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      i = hdviper_g711_decode(c, a, HDVIPER_AUDIO_CODEC_PCMU);
      break;
    default:
      fprintf(stderr, "Unknown codec %d\n", c->codec_type);
      i = 0;
  }

  return i;
}

/*
 *  Destroy the Speex decoder
 */
void hdviper_destroy_speex_decoder(AudioCodec *c) {
  speex_bits_destroy(&c->speex_bits); 
  speex_decoder_destroy(c->speex_state);
}

void hdviper_destroy_g711_decoder(AudioCodec *c) {
}

void hdviper_destroy_audio_decoder(AudioCodec *c) {
  switch(c->codec_type) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      hdviper_destroy_speex_decoder(c);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      hdviper_destroy_g711_decoder(c);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      hdviper_destroy_g711_decoder(c);
      break;
    default:
      fprintf(stderr, "Unknown codec %d\n", c->codec_type);
  }
}



