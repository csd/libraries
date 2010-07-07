#include "video.h"
#include "packetize.h"
#include "packetize_h264.h"

/*
 * HDVIPER packetization functions for H.264
 */

/*
 * Construct H264 RTP header as specified in RFC 3984
 */

void make_h264_header(unsigned char *buf, int packetization_mode, unsigned char nal_unit_octet, int frag_start, int frag_end) {
	unsigned char fu_ind, fu_head;

	switch(packetization_mode) {
		case H264_SINGLE_NAL_UNIT_PACKET:
			*buf = nal_unit_octet;
			break;
		case H264_AGGREGATION_PACKET:
			*buf = nal_unit_octet;
			// to be implemented...
			break;
		case H264_FRAGMENTATION_UNIT:
			fu_ind = (nal_unit_octet & 224) | 28; /* F and NRI bits from NAL octet + FU-A identifier */
			*buf = fu_ind;
      fu_head = (nal_unit_octet & 31) | (frag_start << 7) | (frag_end << 6); /* Type bits from NAL octet */
			*(buf+1) = fu_head;
			break;
	}

}

void hdviper_h264_packetize_nal_unit(Video *v, unsigned char *h264_data, int size, int timecode, int last_nal_unit_of_frame) {
	  unsigned char nal_unit_octet;
		int video_header_size, dgm_payload_size, first_dgm_payload_size, rest, dgms, lastsize, cnt, npad, i;
    unsigned char *video_dgm_ptr;

		/* Does video data + headers fit in one packet? */
		if(VIDEO_DGM_LEN >= size + RTP_HEADER_LEN) {
			/* Single unit NAL case */
      video_header_size = RTP_HEADER_LEN+1; /* note that header byte is also first byte of packet */
      lastsize = dgm_payload_size = first_dgm_payload_size = size;
			dgms = 1;
		} else {
      /* Fragmentation unit case */			
			video_header_size = RTP_HEADER_LEN+2; /* 2 bytes for FU indicator and FU header */
			dgm_payload_size = VIDEO_DGM_LEN - video_header_size;
			first_dgm_payload_size = dgm_payload_size+1; /* because NAL unit octet is in FU header */
			rest = size - first_dgm_payload_size;
			if(rest<0) rest = 0;
			dgms = 1;
			lastsize = rest%dgm_payload_size;
			dgms += (rest/dgm_payload_size) + 
				( (lastsize>0) ? 1 : 0 );
		}
		
		nal_unit_octet = *h264_data;

    for(i=0; i<dgms; i++) {
			/* 
			 * Set pointer to this dgm's data, minus header space needed
			 */ 
			if(i==0)
				/* The +1 below is to overwrite the NAL unit octet with the header, because the
				 * NAL unit octet is transmitted as part of the header */
				video_dgm_ptr = h264_data - video_header_size +1; /* Yes, there is room before h264_data */
			else
			  video_dgm_ptr = h264_data + (i-1)*dgm_payload_size + first_dgm_payload_size - video_header_size;

			/*
			 * Make RTP header and set cnt to number of bytes in dgm (excluding header)
			 */
      if(i==dgms-1) {
				/* Last packet (which could also be the first...) */
					/* Last packet, no padding */
          cnt = lastsize;
          if(cnt==0) cnt = dgm_payload_size;
          make_rtp_header(video_dgm_ptr, HDVIPER_RTP_VER, 0, 0, 0, last_nal_unit_of_frame, HDVIPER_RTP_PT_H264, tick_sequence_nr(&v->rtp_seq), timecode, v->ssrc, 0);
      } else {
        /* Not last packet (could be the first) */
				cnt = dgm_payload_size;
        make_rtp_header(video_dgm_ptr, HDVIPER_RTP_VER, 0, 0, 0, 0, HDVIPER_RTP_PT_H264,
					tick_sequence_nr(&v->rtp_seq), timecode, v->ssrc, 0);
      }

			/*
			 * Make profile specific header
			 */

      if(dgms==1) {

				/* Single NAL unit packet */
        make_h264_header(video_dgm_ptr+RTP_HEADER_LEN, H264_SINGLE_NAL_UNIT_PACKET, nal_unit_octet, 0, 0);
				send_rtp_packet((void *)video_dgm_ptr, cnt+video_header_size-1);
      } else {
				/* Fragmentation units */
        if(i==dgms-1 && i!=0) {
					/* Last packet of fragmentation unit */
            make_h264_header(video_dgm_ptr+RTP_HEADER_LEN, H264_FRAGMENTATION_UNIT, nal_unit_octet, 0, 1);
				} else {
					
					if(i==0) {
						/* First packet of fragmentation unit */
            make_h264_header(video_dgm_ptr+RTP_HEADER_LEN, H264_FRAGMENTATION_UNIT, nal_unit_octet, 1, 0);
          } else {
						/* Neither first nor last packet of a fragmentation unit */
						make_h264_header(video_dgm_ptr+RTP_HEADER_LEN, H264_FRAGMENTATION_UNIT, nal_unit_octet, 0, 0);
					}
				}
				send_rtp_packet((void *)video_dgm_ptr, cnt+video_header_size);
      }
    }
}

/*As implemented now, this function assumes byte aligned. 
 *Apart from that, is it doing the right thing (returns NAL
 *type in header after three zeros...)?
 * -- Yes, this is the way to do it. /MJ
 */
int h264_start_code(unsigned char* p){
        if (p[0]==0 && p[1]==0 && p[2]==1){
                return p[3]&0x1F;
        }
        return 0;
}

void hdviper_h264_packetize(Video *v, int timecode) {
	unsigned char *p, *nal_unit_ptrs[500];
  int s, n=0, i, size, leftsize;

	if(v->size_out==0)
		return;

	p = v->compressed;
  //printf("h264_packetize, size = %d\n", v->size_out);
	while( p < (unsigned char *)(v->compressed + v->size_out) && n<500) {
		s = h264_start_code(p);
		if(s!=0) { 
			/* NAL start code found */
			//printf("h264_packetize: found H.264 start code = %d\n", s);
			p+=3;
			nal_unit_ptrs[n++] = p;
		}
		p++;
	}
	if(n==0) return;
	for(i=0; i<n-1; i++) {
		size = nal_unit_ptrs[i+1]-nal_unit_ptrs[i]-3; // -3 because of start code
		hdviper_h264_packetize_nal_unit(v, nal_unit_ptrs[i], size, timecode, 0);
	}
	leftsize=v->size_out - ((char*)nal_unit_ptrs[n-1] - v->compressed);
  hdviper_h264_packetize_nal_unit(v, nal_unit_ptrs[n-1], leftsize, timecode, 1);

}

