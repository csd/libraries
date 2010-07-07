#ifndef AUDIO_H
#define AUDIO_H

typedef struct {
  short *unencoded_buf; /* audio samples before encoding (or after decoding) */
  int unencoded_n; /* number of unencoded audio samples */
  char *encoded_buf; /* encoded audio samples */
  int encoded_n; /* number of encoded audio bytes */
  int encoding; /* the codec used */
  int sample_rate;
  int channels; /* 1=mono, 2=stereo, ... */
  int bits_per_sample;
  char *rtp_pkt; /* buffer for RTP packet */
  int rtp_pkt_len; /* length of RTP packet */
  unsigned short rtp_seq; /* RTP sequence number */
  unsigned int ssrc; /* SSRC identifier for the audio RTP stream */
} Audio;

#endif
