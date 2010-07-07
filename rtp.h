#ifndef RTP_H
#define RTP_H

#define HDVIPER_RTP_VER 2
#define RTP_HEADER_LEN 12

/* RTP Payload types */
#define HDVIPER_RTP_PT_PCMU 0
#define HDVIPER_RTP_PT_PCMA 8
#define HDVIPER_RTP_PT_SPEEX 107
#define HDVIPER_RTP_PT_H264 119
#define HDVIPER_RTP_PT_H263 120
#define HDVIPER_RTP_PT_JPEG 26


int send_rtp_packet(char *buffer, int length);

#endif
