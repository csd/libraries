/*
 *  This is the Mac version of the  audio capture for the Media Services 
 *  of the HDVIPER project.
 */

#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "audio_capture.h"
#include "audio_capture_mac.h"
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <math.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#define Cursor QTCursor
#include <QuickTimeComponents.h>
#include <FixMath.h>
#include <pthread.h>

pthread_t mac_audio_thread;
pthread_attr_t  pthread_custom_attr;
pthread_mutex_t mac_audio_mutex;

SeqGrabComponent AudioSeqGrabber;
SGChannel soundChannel;
#define MAX_AUDIO_IN_SAMPLES 8000
#define MAX_AUDIO_IN_BYTES (MAX_AUDIO_IN_SAMPLES)
short audio_in_buf[MAX_AUDIO_IN_SAMPLES];
int audio_in_bytes=0;
int audio_in_ptr=0;
static int mac_audio_initialized=0;
Str255 deviceNameStr = "\pBuilt-in Microphone";

void(*mac_audio_in_callback)(Audio *) = NULL;

void *mac_audio_idle_thread(void *arg) {
  //printf("mac_audio_idle_thread called\n");
  while(1) {
    //printf("mac_audio_idle_thread running\n");
    //pthread_mutex_lock(&mac_audio_mutex);
    SGIdle(AudioSeqGrabber);
    //pthread_mutex_unlock(&mac_audio_mutex);
    usleep(11000);
  }
}


OSErr capture_audio_callback(SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType, long refCon ) {
  short sample;
  int i, t;

  if (writeType == seqGrabWriteReserve)
    return 0;

  //printf("capture_audio_callback: Got %d bytes of data, time = %d\n", len, t);
  if(c!=soundChannel) {
    printf("wrong channel\n");
    return;
  }

  pthread_mutex_lock(&mac_audio_mutex);
  if(len > MAX_AUDIO_IN_BYTES - audio_in_bytes) {
    //printf("overflow in capture_audio_callback, dropping %d bytes\n", len - (MAX_AUDIO_IN_BYTES - audio_in_bytes));
    len = MAX_AUDIO_IN_BYTES - audio_in_bytes;
  }

  memcpy(((char *)audio_in_buf)+audio_in_bytes, p, len);
  // swap endian
#if 0
  for (i=audio_in_bytes/2; i<audio_in_bytes/2+len/2; i++) {
    sample = myhtons(audio_in_buf[i]);
    audio_in_buf[i] = sample;
  }
#endif
  audio_in_bytes += len;

  if(mac_audio_in_callback)
    mac_audio_in_callback(NULL);

  return 0;
} 


int hdviper_setup_audio_capture_mac(Audio *a) {
  OSStatus err = noErr;
  ComponentResult cerr = noErr;

  if(mac_audio_initialized)
    return 1;

  AudioSeqGrabber = OpenDefaultComponent(SeqGrabComponentType, 0 );
  cerr = SGInitialize(AudioSeqGrabber);
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGInitialize failed: %d\n", cerr);
    return 0;
  }
  cerr = SGSetDataRef(AudioSeqGrabber, NULL, 0, seqGrabToMemory);
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetDataRef failed: %d\n", cerr);
    return 0;
  }
  cerr = SGNewChannel(AudioSeqGrabber, SoundMediaType /*SGAudioMediaType*/, &soundChannel );
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGNewChannel failed: %d\n", cerr);
    return 0;
  }
  SGSetChannelUsage(soundChannel, seqGrabRecord);
  /*cerr = SGSetChannelDevice( soundChannel, deviceNameStr );
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetChannelDevice failed: %d\n", cerr);
    return 0;
  }*/
  cerr = SGSetChannelPlayFlags( soundChannel, channelPlayAllData );
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetChannelPlayFlags failed: %d\n", cerr);
    return 0;
  }
  cerr = SGSetSoundInputParameters( soundChannel, 16, 1, kSoundNotCompressed );
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetSoundInputParameters failed: %d\n", cerr);
    return 0;
  }
  cerr = SGSetSoundInputRate( soundChannel, rate16khz );
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetSoundInputRate failed: %d\n", cerr);
    return 0;
  }
  // We want this to be small, about 10-20 ms worth of data
  //cerr = SGSetSoundRecordChunkSize(soundChannel, X2Fix(-0.001));
  cerr = SGSetSoundRecordChunkSize(soundChannel, X2Fix(-0.001));
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetSoundRecordChunkSize failed: %d\n", cerr);
    return 0;
  }

  cerr = SGSetDataProc (AudioSeqGrabber, capture_audio_callback, (long)(void *)a);
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGSetDataProc failed: %d\n", cerr);
    return 0;
  }
  cerr = SGStartRecord(AudioSeqGrabber);
  if(cerr!=noErr) {
    fprintf(stderr, "hdviper_setup_audio_capture_mac: SGStartRecord failed: %d\n", cerr);
    return 0;
  }

  // Start thread to call SGIdle()
  pthread_mutex_init(&mac_audio_mutex, NULL);
  pthread_attr_init(&pthread_custom_attr);
  pthread_create(&mac_audio_thread, &pthread_custom_attr, mac_audio_idle_thread, (void *)AudioSeqGrabber);

  mac_audio_initialized = 1;  
  return 1;
}

int hdviper_capture_audio_frame_mac(Audio *a) {
  
   //printf("audio_read: %d bytes available\n", audio_in_bytes);
  if(audio_in_bytes < a->unencoded_n) {
    //printf("underflow in audio_read: %d bytes \n", a->unencoded_n - audio_in_bytes);
    a->unencoded_n = audio_in_bytes;
  }

  memcpy(a->unencoded_buf, ((char *)audio_in_buf), a->unencoded_n);
  if(audio_in_bytes - a->unencoded_n>0)
    memmove((char *)audio_in_buf, ((char *)audio_in_buf)+a->unencoded_n, audio_in_bytes-a->unencoded_n);
  audio_in_bytes -= a->unencoded_n;
  return a->unencoded_n;
}


int hdviper_register_audio_callback_mac(Audio *a, void(*f)(Audio *)) {
  mac_audio_in_callback = f;
}

int hdviper_deregister_audio_callback_mac(Audio *a) {
  mac_audio_in_callback = NULL;
}

void hdviper_destroy_audio_capture_mac(Audio *a) {
  mac_audio_initialized = 0;  
}


