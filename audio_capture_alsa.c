/*
 *  This is the ALSA audio capture for the Media Services of the HDVIPER 
 *  project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <alloca.h>
#include <alsa/asoundlib.h>
#include "audio_capture.h"
#include "audio_capture_alsa.h"

static snd_pcm_t *alsa_capture_handle;
static char *alsa_device = "default";      /* capture device */
static int alsa_audio_is_initialized = 0;

#define CHECK_RET_OR_MESSAGE_AND_RETURN(x) 		\
	if (ret < 0) { 					\
		fprintf(stderr, x, snd_strerror(ret));	\
		return 0;				\
	}

#define TRACE_X(x) printf("%s(): %s\n", __func__, x);


int hdviper_setup_audio_capture_alsa(Audio *a) {
	int ret;
	snd_pcm_hw_params_t *hw_params;

	TRACE_X("snd_pcm_hw_params_malloc")
	ret = snd_pcm_hw_params_malloc(&hw_params);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot allocate hardware parameter structure (%s)");

	TRACE_X("snd_pcm_open")
	ret = snd_pcm_open(&alsa_capture_handle, alsa_device, SND_PCM_STREAM_CAPTURE, 0);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot open audio capture device (%s)");
	TRACE_X("snd_pcm_hw_params_any")
	ret = snd_pcm_hw_params_any(alsa_capture_handle, hw_params);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot initialize hardware parameter structure (%s)");

	TRACE_X("snd_pcm_hw_params_set_access")
	ret = snd_pcm_hw_params_set_access(alsa_capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot set access type (%s)");

	TRACE_X("snd_pcm_hw_params_set_format")
	ret = snd_pcm_hw_params_set_format(alsa_capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot set sample format (%s)");

	TRACE_X("snd_pcm_hw_params_set_rate_near")
	ret = snd_pcm_hw_params_set_rate_near(alsa_capture_handle, hw_params, &a->sample_rate, 0);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot set sample rate (%s)");
	printf("%s(): sample rate is %d\n", __func__, a->sample_rate);

	TRACE_X("snd_pcm_hw_params_set_channels")
	ret = snd_pcm_hw_params_set_channels(alsa_capture_handle, hw_params, 1);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot set channel count (%s)");

	TRACE_X("snd_pcm_hw_params")
	ret = snd_pcm_hw_params(alsa_capture_handle, hw_params);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot set parameters (%s)");

	TRACE_X("snd_pcm_hw_params_free")
	snd_pcm_hw_params_free(hw_params);
	
	TRACE_X("snd_pcm_prepare")
	ret = snd_pcm_prepare(alsa_capture_handle);
	CHECK_RET_OR_MESSAGE_AND_RETURN("cannot prepare audio interface for use (%s)");

	ret = snd_pcm_start(alsa_capture_handle);
	CHECK_RET_OR_MESSAGE_AND_RETURN("snd_pcm_start failed (%s)");

	alsa_audio_is_initialized = 1;
	return 1;
}

int hdviper_capture_audio_frame_alsa(Audio *a) {
	int ret;
	assert(alsa_audio_is_initialized);
	//TRACE_X("")
	ret = snd_pcm_readi(alsa_capture_handle, a->unencoded_buf, a->unencoded_n);
	if (ret < 0) {
		printf("%s(): read error - %s\n", __func__, snd_strerror(ret));
		return 0;
	}
	ret *= 2;
	//printf("%s(): read %d bytes.\n", __func__, ret);
	return ret;
}


int hdviper_register_audio_callback_alsa(Audio *a, void(*f)(Audio *)) {
}

int hdviper_deregister_audio_callback_alsa(Audio *a) {
}

void hdviper_destroy_audio_capture_alsa(Audio *a) {
	TRACE_X("BEGIN")
	snd_pcm_close(alsa_capture_handle);
	alsa_audio_is_initialized = 0;
	TRACE_X("END")
}


