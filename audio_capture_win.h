#ifndef AUDIO_CAPTURE_WIN_H
#define AUDIO_CAPTURE_WIN_H

#include "audio.h"

int hdviper_setup_audio_capture_win(Audio *a); 
int hdviper_capture_audio_frame_win(Audio *a);
int hdviper_register_audio_callback_win(Audio *a, void(*f)(Audio *));
int hdviper_deregister_audio_callback_win(Audio *a);
void hdviper_destroy_audio_capture_win(Audio *a);

#endif
