#ifndef PACKETIZE_H
#define PACKETIZE_H

#include "audio.h"
#include "video.h"
#include "rtp.h"


/* Unassigned codec type */
#define HDVIPER_CODEC_NONE 0

/* RTP header struct */

struct rtp_header {
  unsigned char version;
  unsigned char padding;
  unsigned char extension;
  unsigned char cc;
  unsigned char marker;
  unsigned char payload_type;
  unsigned short sequence_nr;
  unsigned int timestamp;
  unsigned int ssrc;
  unsigned int csrc[15];
};

void hdviper_rtp_packetize_audio_frame(Audio *);
void hdviper_rtp_packetize_video_frame(Video *);
void hdviper_rtp_depacketize_audio_frame(Audio *); 
void hdviper_rtp_depacketize_video_frame(Video *);

#endif
