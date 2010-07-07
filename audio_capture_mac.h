#ifndef AUDIO_CAPTURE_MAC_H
#define AUDIO_CAPTURE_MAC_H

#include "audio.h"

int hdviper_setup_audio_capture_mac(Audio *a); 
int hdviper_capture_audio_frame_mac(Audio *a);
int hdviper_register_audio_callback_mac(Audio *a, void(*f)(Audio *));
int hdviper_deregister_audio_callback_mac(Audio *a);
void hdviper_destroy_audio_capture_mac(Audio *a);

#endif
