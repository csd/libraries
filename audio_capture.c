/*
 *  This is the audio capture API for the Media Services of the HDVIPER 
 *  project. The functions in this file typically calls hardware-specific
 *  functions located in other files, e.g. audio_capture_alse.c for the
 *  ALSA-specific versions, etc.
 */

#include <stdlib.h>
#include "audio_capture.h"
#ifdef HDVIPER_AUDIO_CAPTURE_ALSA
#include "audio_capture_alsa.h"
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_WIN
#include "audio_capture_win.h"
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_MAC
#include "audio_capture_mac.h"
#endif

/*
 * Initialize the audio capture board and set up everything needed to 
 * start capturing audio.
 */
int hdviper_setup_audio_capture(Audio *a) {
  int i=1;

  /* these are currently hardcoded */
  a->sample_rate = 16000;
  a->bits_per_sample = 16;
  a->channels = 1;
  a->unencoded_buf = (short *)malloc(16000*2); /* 1 sek */
  a->encoded_buf = (char *)malloc(16000*2);
  a->rtp_pkt = (char *)malloc(1500);
  a->rtp_seq = 0;

#ifdef HDVIPER_AUDIO_CAPTURE_ALSA
  i = hdviper_setup_audio_capture_alsa(a); 
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_WIN
  i = hdviper_setup_audio_capture_win(a); 
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_MAC
  i = hdviper_setup_audio_capture_mac(a); 
#endif

  return i;
}

/*
 * Capture one 'chunk' of audio data.
 */
int hdviper_capture_audio_frame(Audio *a) {
  int i, j;


#ifdef HDVIPER_AUDIO_CAPTURE_TEST
  /* generate 320 Hz tone */
  a->unencoded_n = 16000;
  for(i=0; i<a->unencoded_n / 50; i++) {
    for(j=0; j<25; j++)
      a->unencoded_buf[j+i*50] = 10000;
    for(j=25; j<50; j++)
      a->unencoded_buf[j+i*50] = -10000;
  }
#endif

#ifdef HDVIPER_AUDIO_CAPTURE_ALSA
  i = hdviper_capture_audio_frame_alsa(a);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_WIN
  i = hdviper_capture_audio_frame_win(a);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_MAC
  i = hdviper_capture_audio_frame_mac(a);
#endif

  return i;
}


/*
 * Register a callback function to be called whenever audio data is
 * available.
 */
int hdviper_register_audio_callback(Audio *a, void(*f)(Audio *)) {
#ifdef HDVIPER_AUDIO_CAPTURE_ALSA
  return hdviper_register_audio_callback_alsa(a, f);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_WIN
  return hdviper_register_audio_callback_win(a, f);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_MAC
  return hdviper_register_audio_callback_mac(a, f);
#endif
}

/*
 * De-register a callback function 
 */
int hdviper_deregister_audio_callback(Audio *a) {
#ifdef HDVIPER_AUDIO_CAPTURE_ALSA
  return hdviper_deregister_audio_callback_alsa(a);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_WIN
  return hdviper_deregister_audio_callback_win(a);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_MAC
  return hdviper_deregister_audio_callback_mac(a);
#endif
}

/*
 * Free all memory allocated in the setup function, close audio devices etc.
 */
void hdviper_destroy_audio_capture(Audio *a) {
  free(a->unencoded_buf);
  free(a->encoded_buf);
  free(a->rtp_pkt);
#ifdef HDVIPER_AUDIO_CAPTURE_ALSA
  hdviper_destroy_audio_capture_alsa(a);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_WIN
  hdviper_destroy_audio_capture_win(a);
#endif
#ifdef HDVIPER_AUDIO_CAPTURE_MAC
  hdviper_destroy_audio_capture_mac(a);
#endif
}


