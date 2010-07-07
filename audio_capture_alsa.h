#ifndef AUDIO_CAPTURE_ALSA_H
#define AUDIO_CAPTURE_ALSA_H

#include "audio.h"

int hdviper_setup_audio_capture_alsa(Audio *a); 
int hdviper_capture_audio_frame_alsa(Audio *a);
int hdviper_register_audio_callback_alsa(Audio *a, void(*f)(Audio *));
int hdviper_deregister_audio_callback_alsa(Audio *a);
void hdviper_destroy_audio_capture_alsa(Audio *a);

#endif
