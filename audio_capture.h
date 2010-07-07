#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "audio.h"

int hdviper_setup_audio_capture(Audio *);
int hdviper_capture_audio_frame(Audio *);
int hdviper_register_audio_callback(Audio *, void(*f)(Audio *));
int hdviper_deregister_audio_callback(Audio *);
void hdviper_destroy_audio_capture(Audio *);

#endif
