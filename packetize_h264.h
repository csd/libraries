#ifndef PACKETIZE_H264_H
#define PACKETIZE_H264_H


#define H264_SINGLE_NAL_UNIT_PACKET 0
#define H264_AGGREGATION_PACKET 1
#define H264_FRAGMENTATION_UNIT 2

void hdviper_h264_packetize(Video *v, int timecode);
#endif
