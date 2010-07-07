#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

#include "audio.h"
#include "speex/speex.h"

#define HDVIPER_AUDIO_CODEC_SPEEX 100
#define HDVIPER_AUDIO_CODEC_PCMA 101
#define HDVIPER_AUDIO_CODEC_PCMU 102


typedef struct {
  int codec_type;
  SpeexBits speex_bits;
  const SpeexMode *speex_mode;
  void *speex_state;
  int sample_rate;
  int frame_size;
} AudioCodec;


int hdviper_setup_audio_encoder(AudioCodec *, int);
int hdviper_audio_encode(AudioCodec *, Audio *);
void hdviper_destroy_audio_encoder(AudioCodec *);
int hdviper_setup_audio_decoder(AudioCodec *, int);
int hdviper_audio_decode(AudioCodec *, Audio *);
void hdviper_destroy_audio_decoder(AudioCodec *);

#endif
