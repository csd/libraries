
/*
 * This is the media packetization API of the HDVIPER Media Services
 */

#include "packetize.h"
#include "audio.h"
#include "video.h"
#include "audio_codec.h"
#include "video_codec.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#include <winsock2.h>
#endif

/*
 * Update this table with PT -> codec mappings.
 */
unsigned char rtp_payload_type_map[255];

/*
 * Initializes the RTP PT -> codec mapping table with default values.
 */
void init_rtp_pt_map() {
  int i;

  for(i=0; i<256; i++)
    rtp_payload_type_map[i] = HDVIPER_CODEC_NONE;

  rtp_payload_type_map[HDVIPER_RTP_PT_PCMU] = HDVIPER_AUDIO_CODEC_PCMU;
  rtp_payload_type_map[HDVIPER_RTP_PT_PCMA] = HDVIPER_AUDIO_CODEC_PCMA;
  rtp_payload_type_map[HDVIPER_RTP_PT_SPEEX] = HDVIPER_AUDIO_CODEC_SPEEX;
  rtp_payload_type_map[HDVIPER_RTP_PT_H264] = HDVIPER_VIDEO_CODEC_H264;
  rtp_payload_type_map[HDVIPER_RTP_PT_H263] = HDVIPER_VIDEO_CODEC_H263;
  rtp_payload_type_map[HDVIPER_RTP_PT_JPEG] = HDVIPER_VIDEO_CODEC_JPEG;
}

/*
 *  calculate time in usecs between 'before' and 'after'
 */

long elapsed_usecs(struct timeval *before, struct timeval *after) {
  return( (after->tv_sec - before->tv_sec)*1000000 +
           (after->tv_usec - before->tv_usec) );
}

unsigned short next_sequence_nr(unsigned short *t) {
  return (*t)++;
}

#ifdef WIN32

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone {
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday_win(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
       
  if (NULL != tv) {
    GetSystemTimeAsFileTime(&ft);
	        
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
     
    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  /*if (NULL != tz) {
    if (!tzflag) {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }*/
											   
  return 0;
}
#endif

/*
 * Generate timestamp. hz is update frequency of clock.
 * timestamp(16000) give timestamps in 16 kHz
 */
unsigned int rtp_timestamp(int hz) {
  static struct timeval starttime;
  struct timeval now;
  static int first=1;
  unsigned int t;
  double d, x;

  if(first) {
    first=0;
#ifdef WIN32
    gettimeofday_win(&starttime, (struct timezone*)NULL);
#else
    gettimeofday(&starttime, (struct timezone*)NULL);
#endif
  }
#ifdef WIN32
  gettimeofday_win(&now, (struct timezone*)NULL);
#else
  gettimeofday(&now, (struct timezone*)NULL);
#endif
  x = 1000000.0 / (double)hz;
  d = ((double) elapsed_usecs(&starttime, &now)) / x;
  t = (unsigned int) d;

  return t;
}

/*
 * This function should map from a RTP payload type identifier to
 * a constant representing a codec in the HDVIPER media services lib.
 * The RTP Payload types are decidec dynamically by the SIP signaling
 * e.g. in the HDVIPER Conrol Services. For now this mapping is 
 * maintained as a table, but implementation may change.
 */

int map_rtp_pt_to_codec(int pt) {
  return rtp_payload_type_map[pt];
}

/*
 * Construct RTP-header 
 */

void hdviper_make_rtp_header(char *buf, unsigned short pad, unsigned short ext, unsigned short cc, unsigned short marker, unsigned short pt, unsigned short seq, unsigned int time, unsigned int ssrc, unsigned int *csrc) {

  unsigned short ver = HDVIPER_RTP_VER;
  unsigned short flags;
  int i;

  flags = ver << 14 | pad << 13 | ext << 12 | cc << 8 | marker << 7 | pt; 
  flags = htons(flags);
  seq = htons(seq);
  time = htonl(time);
  ssrc = htonl(ssrc);
  memcpy(&buf[0], &flags, 2);
  memcpy(&buf[2], &seq, 2);
  memcpy(&buf[4], &time, 4);
  memcpy(&buf[8], &ssrc, 4);

  for(i=0; i<cc; i++){
    csrc[i] = htonl(csrc[i]);
    memcpy(&buf[12+4*i], &csrc[i], 4);
  }
}

/*
 * Parse RTP-header
 */

void hdviper_parse_rtp_header(char *dgm, struct rtp_header *h) {
  unsigned short word;
  unsigned int dword;
  int i;

  memcpy(&word, &dgm[0], 2);
  word = ntohs(word);
  h->version = word >> 14; 
  h->padding = (word >> 13) & 1; 
  h->extension = (word >> 12) & 1; 
  h->cc = (word >> 8) & 15; 
  h->marker = (word >> 7) & 1;
  h->payload_type = word & 127;

  memcpy(&word, &dgm[2], 2);
  h->sequence_nr = ntohs(word);

  memcpy(&dword, &dgm[4], 4);
  h->timestamp = ntohl(dword);

  memcpy(&dword, &dgm[8], 4);
  h->ssrc = ntohl(dword);

  for(i=0; i<h->cc; i++) {
    memcpy(&dword, &dgm[12+4*i], 4);
    h->csrc[i] = ntohl(dword);
  }
}

void hdviper_rtp_packetize_speex_frame(Audio *a) {
  /* make RTP header */
  hdviper_make_rtp_header(a->rtp_pkt, 0, 0, 0, 0, HDVIPER_RTP_PT_SPEEX,
    next_sequence_nr(&a->rtp_seq), rtp_timestamp(a->sample_rate), a->ssrc, 0);

  /* copy audio data */
  memcpy(a->rtp_pkt+RTP_HEADER_LEN, a->encoded_buf, a->encoded_n);
  a->rtp_pkt_len =  a->encoded_n + RTP_HEADER_LEN;
}

void hdviper_rtp_depacketize_speex_frame(Audio *a) {
  /* copy audio data from RTP buf to decode buf */
  memcpy(a->encoded_buf, a->rtp_pkt+RTP_HEADER_LEN, a->rtp_pkt_len-RTP_HEADER_LEN);
}

void hdviper_rtp_packetize_g711_frame(Audio *a, int type) {
  int pt;

  if(type == HDVIPER_AUDIO_CODEC_PCMA)
    pt = HDVIPER_RTP_PT_PCMA;
  else
    pt = HDVIPER_RTP_PT_PCMU;

  /* make RTP header */
  hdviper_make_rtp_header(a->rtp_pkt, 0, 0, 0, 0, pt,
    next_sequence_nr(&a->rtp_seq), rtp_timestamp(a->sample_rate), a->ssrc, 0);

  /* copy audio data */
  memcpy(a->rtp_pkt+RTP_HEADER_LEN, a->encoded_buf, a->encoded_n);
  a->rtp_pkt_len =  a->encoded_n + RTP_HEADER_LEN;
}

void hdviper_rtp_depacketize_g711_frame(Audio *a) {
  /* copy audio data from RTP buf to decode buf */
  memcpy(a->encoded_buf, a->rtp_pkt+RTP_HEADER_LEN, a->rtp_pkt_len-RTP_HEADER_LEN);
}


void hdviper_rtp_packetize_audio_frame(Audio *a) {
  switch(a->encoding) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      hdviper_rtp_packetize_speex_frame(a);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      hdviper_rtp_packetize_g711_frame(a, HDVIPER_AUDIO_CODEC_PCMA);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      hdviper_rtp_packetize_g711_frame(a, HDVIPER_AUDIO_CODEC_PCMU);
      break;
    default:
      fprintf(stderr, "Unknown encoding %d\n", a->encoding);
  }
}

void hdviper_rtp_packetize_h264_frame(Video *v) {
  hdviper_make_rtp_header(v->rtp_pkt, 0, 0, 0, 0, HDVIPER_RTP_PT_H264,
    next_sequence_nr(&v->rtp_seq), rtp_timestamp(90000), v->ssrc, 0);
}

void hdviper_rtp_packetize_h263_frame(Video *v) {
}

void hdviper_rtp_packetize_jpeg_frame(Video *v) {
}

void hdviper_rtp_depacketize_jpeg(Video *v) {
}

void hdviper_rtp_depacketize_h263(Video *v) {
}

void hdviper_rtp_depacketize_h264(Video *v) {
}

void hdviper_rtp_packetize_video_frame(Video *v) {
  switch(v->compression) {
    case HDVIPER_VIDEO_CODEC_H264:
      hdviper_rtp_packetize_h264_frame(v);
      break;
    case HDVIPER_VIDEO_CODEC_H263:
      hdviper_rtp_packetize_h263_frame(v);
      break;
    case HDVIPER_VIDEO_CODEC_JPEG:
      hdviper_rtp_packetize_jpeg_frame(v);
      break;
  }
}

/*
 * Parse audio RTP packet and depacketize based on Payload Type
 */
void hdviper_rtp_depacketize_audio_frame(Audio *a) {
  struct rtp_header rtph;

  hdviper_parse_rtp_header(a->rtp_pkt, &rtph);
  a->encoding = map_rtp_pt_to_codec(rtph.payload_type);
  switch(a->encoding) {
    case HDVIPER_AUDIO_CODEC_SPEEX:
      hdviper_rtp_depacketize_speex_frame(a);
      break;
    case HDVIPER_AUDIO_CODEC_PCMA:
      hdviper_rtp_depacketize_g711_frame(a);
      break;
    case HDVIPER_AUDIO_CODEC_PCMU:
      hdviper_rtp_depacketize_g711_frame(a);
      break;
    default:
      fprintf(stderr, "Unknown audio RTP payload type\n");
  }

}

/*
 * Parse video RTP packet and depacketize based on Payload Type
 */
void hdviper_rtp_depacketize_video_frame(Video *v) {
  struct rtp_header rtph;

  hdviper_parse_rtp_header(v->rtp_pkt, &rtph);
  v->compression = map_rtp_pt_to_codec(rtph.payload_type);
  switch(v->compression) {
    case HDVIPER_VIDEO_CODEC_H264:
      hdviper_rtp_depacketize_h264(v);
      break;
    case HDVIPER_VIDEO_CODEC_H263:
      hdviper_rtp_depacketize_h263(v);
      break;
    case HDVIPER_VIDEO_CODEC_JPEG:
      hdviper_rtp_depacketize_jpeg(v);
      break;
    default:
      fprintf(stderr, "Unknown video RTP payload type\n");
  }

}

