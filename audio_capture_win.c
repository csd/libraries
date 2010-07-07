/*
 *  This is the Windows version of the audio capture for the Media Services 
 *  of the HDVIPER project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <winsock.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <sys/types.h>
#include "audio.h"
#include "audio_capture.h"
#include "audio_capture_win.h"

HWAVEIN  audio_in_fd;

#define WAVE_BUFS 7

WAVEHDR input_header[WAVE_BUFS];

char *wave_inbuf[WAVE_BUFS];
int slicesize[WAVE_BUFS];
int wave_bufsize;
int wave_in_size;
int wave_in_available=0;
int inbuf_index = 0;
int tailp = 0;
int buffered_bytes = 0;

WAVEFORMATEX wavefmt = {
    WAVE_FORMAT_PCM,
    1,
    16000,
    2 * 16000,
    2,
    16,
    0
};

struct audMux {
	MIXERCONTROLDETAILS select_[50];
	MIXERCONTROLDETAILS vol_[50];
	MIXERCONTROL mc[50];
	u_char mcnt_;
	u_char vcnt_;
	unsigned char isOut_;
};

static struct audMux mux;
static int input_port = 0;
char *audio_input_list[50];
int num_audio_inputs = 0;
static int audio_dev = 0;
//HMIXEROBJ audio_dev_hmixerobj;
char *audio_device_list[50];
int num_audio_devices = 0;

void(*win_audio_in_callback)(Audio *) = NULL;

/*
 * Callback from audio recording
 */
void CALLBACK wave_in_callback(HWAVEOUT waveIn, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {

  if (uMsg != MM_WIM_DATA) {
    return;
  }

  wave_in_available += wave_in_size;
  if(win_audio_in_callback)
    win_audio_in_callback((Audio *)dwParam1);
}

int get_input_index_of(char *name) {
	int i;

	for (i = 0; i < num_audio_inputs; i++) {
		if (strncmp(name, audio_input_list[i], strlen(name)) == 0) {
			return i;
		}
	}

	return 0;
}

void get_mixer_volume_details(MIXERLINE *ml, MIXERCONTROL *mc)
{
	MIXERCONTROLDETAILS mcd;
	int i = 0;
	MIXERCONTROLDETAILS_UNSIGNED *mcdu;

	if (num_audio_inputs == 0) {
		num_audio_inputs = 1;
		audio_input_list[0] = strdup(ml->szName);
	}

	mcd.cbStruct = sizeof(mcd);
	mcd.dwControlID = mc->dwControlID;
	mcd.cChannels = ml->cChannels;

	mcdu =(MIXERCONTROLDETAILS_UNSIGNED *)malloc(sizeof(MIXERCONTROLDETAILS_UNSIGNED)*mcd.cChannels);
	memset(mcdu, 0, mcd.cChannels * sizeof(*mcdu));

	mux.vol_[get_input_index_of((char*)ml->szName)] = mcd;
	if (mc->fdwControl | MIXERCONTROL_CONTROLF_UNIFORM) {
		mux.vol_[i].cChannels = 1;
	} else {
		mux.vol_[i].cChannels = 2;
	}
	mux.vol_[i].cbDetails = sizeof(*mcdu);
	mux.vol_[i].paDetails = mcdu;

}

void get_mixer_details(MIXERLINE *ml, MIXERCONTROL *mc)
{
	MIXERCONTROLDETAILS mcd;
	unsigned int i;
	unsigned int sts, n, j;
	MIXERCONTROLDETAILS_BOOLEAN *mcdb;
	MIXERCONTROLDETAILS_UNSIGNED *mcdu;
  MIXERCONTROLDETAILS_LISTTEXT mcdt[100];	
	int index;

	mcd.cbStruct = sizeof(mcd);
	mcd.dwControlID = mc->dwControlID;
	mcd.cChannels = ml->cChannels;
	mcd.cMultipleItems = mc->cMultipleItems;
	if (mcd.cMultipleItems) {
		mcd.cbDetails = sizeof(mcdt[0]);
		mcd.paDetails = mcdt;
		sts = mixerGetControlDetails((HMIXEROBJ)audio_dev, &mcd, MIXER_GETCONTROLDETAILSF_LISTTEXT);
		if (sts == MMSYSERR_NOERROR) {
			for (i = 0; i < mc->cMultipleItems; ++i) {
				n = mcd.cMultipleItems * mcd.cChannels;
				mcdb = (MIXERCONTROLDETAILS_BOOLEAN *)
					malloc(sizeof(MIXERCONTROLDETAILS_BOOLEAN)*n);
				memset(mcdb, 0, n * sizeof(*mcdb));
				for (j = 0; j < mcd.cChannels; ++j)
					mcdb[j*mc->cMultipleItems+i].fValue = 1;
				mux.select_[i] = mcd;
				mux.select_[i].cbDetails = sizeof(*mcdb);
				mux.select_[i].paDetails = mcdb;
				audio_input_list[i] = strdup(mcdt[i].szName);
				num_audio_inputs++;
			}
			mux.mcnt_ = (u_char)num_audio_inputs;
		}
	} else {
		if (num_audio_inputs == 0) {
			num_audio_inputs = 1;
			audio_input_list[0] = strdup(ml->szName);
		}
		mux.vcnt_ = num_audio_inputs;

		mcdu =(MIXERCONTROLDETAILS_UNSIGNED *)malloc(sizeof(MIXERCONTROLDETAILS_UNSIGNED)*mcd.cChannels);
		memset(mcdu, 0, mcd.cChannels * sizeof(*mcdu));
		index = get_input_index_of(ml->szName);
		mux.vol_[index] = mcd;
		if (mc->fdwControl | MIXERCONTROL_CONTROLF_UNIFORM) {
			mux.vol_[index].cChannels = 1;
		} else {
			mux.vol_[index].cChannels = 2;
		}
		mux.vol_[index].cbDetails = sizeof(*mcdu);
		mux.vol_[index].paDetails = mcdu;
		if (mc->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) {
			mcdb = (MIXERCONTROLDETAILS_BOOLEAN *)
					malloc(sizeof(MIXERCONTROLDETAILS_BOOLEAN));
			memset(mcdb, 0, sizeof(*mcdb));
			mux.select_[index] = mcd;
			mux.select_[index].cbDetails = sizeof(*mcdb);
			mux.select_[index].paDetails = mcdb;
			mcdb->fValue = 0;
		}
	}
}

void get_mixer_ctrls(MIXERLINE *ml)
{
	MIXERLINECONTROLS mlc;
	MIXERCONTROL mc[16];
	unsigned int i;
	MIXERLINE src;
	MIXERCONTROLDETAILS mcd = {0};
	MIXERCONTROLDETAILS_BOOLEAN mcb = {0};
	MMRESULT result;
	MIXERCONTROLDETAILS_LISTTEXT mcdt[100];

	memset(&mlc, 0, sizeof(mlc));
	memset(mc, 0, sizeof(mc));
	mlc.cbStruct = sizeof(mlc);
	mlc.cbmxctrl = sizeof(mc[0]);
	mlc.pamxctrl = &mc[0];
	mlc.dwLineID = ml->dwLineID;
	mlc.cControls = ml->cControls;
	mixerGetLineControls((HMIXEROBJ)audio_dev, &mlc, MIXER_GETLINECONTROLSF_ALL);
	for (i = 0; i < mlc.cControls; ++i) {
		switch (mc[i].dwControlType) {

		case MIXERCONTROL_CONTROLTYPE_MUX:
		case MIXERCONTROL_CONTROLTYPE_MIXER:
		case MIXERCONTROL_CONTROLTYPE_VOLUME:
		case MIXERCONTROL_CONTROLTYPE_MUTE:
			get_mixer_details(ml, &mc[i]);
			break;
		/*case MIXERCONTROL_CONTROLTYPE_MUTE:
			mcd.dwControlID = mc[i].dwControlID;
			mcd.cChannels = ml->cChannels;
			mcd.cbDetails = sizeof(mcdt[0]);
			mcd.paDetails = mcdt;
			if (mc[i].cMultipleItems > 0) {
				result = mixerGetControlDetails((HMIXEROBJ)audio_dev, &mcd, MIXER_GETCONTROLDETAILSF_LISTTEXT);
			}
			mcb.fValue = 1;
			mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
			mcd.paDetails = &mcb;
			mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
			result = mixerSetControlDetails((HMIXEROBJ) audio_dev, &mcd, MIXER_SETCONTROLDETAILSF_VALUE);
			if (result != MMSYSERR_NOERROR) {
				printf("Hello");
			}
			break;*/
		}
	}
	/*
	 * if there are multiple source lines for this line,
	 * get their volume controls
	 */
	for (i = 0; i < ml->cConnections; ++i) {
		memset(&src, 0, sizeof(src));
		src.cbStruct = sizeof(src);
		src.dwSource = i;
		src.dwDestination = ml->dwDestination;
		if (mixerGetLineInfo((HMIXEROBJ)audio_dev, &src, MIXER_GETLINEINFOF_SOURCE) == MMSYSERR_NOERROR) {
			get_mixer_ctrls(&src);
		}
	}
}

int hdviper_setup_audio_capture_win(Audio *a) {
	int i, n, n_wave;
	MIXERCAPS mc;
	int s;
  	MIXERLINE ml;
		int error = 0;

	  fprintf(stderr, "hdviper_setup_audio_capture_win called\n");

	n_wave = waveInGetNumDevs();
	
	n = mixerGetNumDevs();
	audio_dev = n-1;
	if(n_wave != n) {
	  fprintf(stderr, "Warning: Number of WAVE audio input devices does not equal number of mixing devices\n");
	}
	fprintf(stderr, "Number of audio devices found: %d\n", n_wave);
	fprintf(stderr, "Number of mixer devices found: %d\n", n);
	
	for (i = 0; i < n_wave; i++) {
		WAVEINCAPS cap;
		waveInGetDevCaps(i, &cap, sizeof(cap));
    mixerGetDevCaps(i, &mc, sizeof(MIXERCAPS));
		audio_device_list[i] = strdup(cap.szPname);
		fprintf(stderr, "Audio device #%d: %s\n", i, cap.szPname);
	}

	num_audio_devices = n_wave;
	audio_dev = 0;

  	num_audio_inputs = 0;
	memset(&mux, 0, sizeof(mux));

	memset(&ml, 0, sizeof(ml));
	ml.cbStruct = sizeof(ml);
	ml.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
	s = mixerGetLineInfo((HMIXEROBJ)audio_dev, &ml, MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (s != 0)
		return -1;

	get_mixer_ctrls(&ml);	

  	wavefmt.wFormatTag = WAVE_FORMAT_PCM;
  	wavefmt.nChannels = a->channels;
  	wavefmt.wBitsPerSample =  a->bits_per_sample;
  	wavefmt.nSamplesPerSec = a->sample_rate;
  	wavefmt.nBlockAlign = wavefmt.nChannels*wavefmt.wBitsPerSample/8;
  	wavefmt.nAvgBytesPerSec = wavefmt.nSamplesPerSec*wavefmt.nBlockAlign;
  	wavefmt.cbSize = 0;

  	error = waveInOpen(&audio_in_fd, audio_dev, &wavefmt,  
    	(DWORD)wave_in_callback, (DWORD_PTR)a, CALLBACK_FUNCTION);
  	if(error) {
			fprintf(stderr, "Could not set recording parameters.\n");
    		return 0;
  	}

  	/* Set input and output buffer sizes to 1 s */
  	wave_bufsize = wavefmt.nSamplesPerSec*wavefmt.nBlockAlign;

  	/* record 20 ms chunks at a time */
  	wave_in_size = wavefmt.nSamplesPerSec*wavefmt.nBlockAlign/50;

  	/* Allocate input and output audio buffers */
  	for(i=0; i<WAVE_BUFS; i++) {
    		wave_inbuf[i] = (char *)malloc(wave_in_size);
    		memset(wave_inbuf[i], 0, wave_in_size);
    		input_header[i].dwFlags = 0;
    		input_header[i].dwLoops = 0;
    		input_header[i].lpData = wave_inbuf[i];
    		input_header[i].dwBufferLength = wave_in_size;
    		error = waveInPrepareHeader(audio_in_fd, &input_header[i], 
      		sizeof(WAVEHDR));
    		if(error) {
      			fprintf(stderr, "error in waveInPrepareHeader\n");
      			return 0;
    		}
    		error = waveInAddBuffer(audio_in_fd, &input_header[i], sizeof(WAVEHDR));
    		if(error) {
      			fprintf(stderr, "error in waveInAddBuffer\n");
      			return 0;
    		}
  	}

  	/* start recording */
  	waveInStart(audio_in_fd);
		return 1;
}

int hdviper_capture_audio_frame_win(Audio *a) {
  int size, error;
  char *audio_ptr;
  static int waveinaddbuf_error_seen = 1; /* Set to 1 to disable strange error message */

  if(a->unencoded_n>wave_in_available)
    a->unencoded_n = wave_in_available;
  if(a->unencoded_n==0) {
    return 0;
  }
  size = a->unencoded_n;
  audio_ptr = a->unencoded_buf;

  while(size>=wave_in_size) {
    waveInUnprepareHeader(audio_in_fd, &input_header[inbuf_index], 
      sizeof(WAVEHDR));
    memcpy(audio_ptr, wave_inbuf[inbuf_index], 
      input_header[inbuf_index].dwBufferLength);
    audio_ptr += input_header[inbuf_index].dwBufferLength;
    size -= input_header[inbuf_index].dwBufferLength;
    error = waveInPrepareHeader(audio_in_fd, &input_header[inbuf_index], 
      sizeof(WAVEHDR));
    if(error) {
#ifdef DEBUG
      fprintf(stderr, "Error in waveInPrepareHeader\n");
#endif
	    return 0;
    }
    error = waveInAddBuffer(audio_in_fd, &input_header[inbuf_index], 
      sizeof(WAVEHDR));
    if(error != MMSYSERR_NOERROR ) {
		/* Alert user first time waveInaddBuffer fails, then ignore error */
		if(!waveinaddbuf_error_seen) {
		  switch(error) {
		  case MMSYSERR_INVALHANDLE:
            fprintf(stderr, "Error in waveInAddBuffer: Specified device handle is invalid.\n");
		  break;
		  case MMSYSERR_NODRIVER:
            fprintf(stderr, "Error in waveInAddBuffer: No device driver is present.\n");
		  break;
		  case MMSYSERR_NOMEM:
		    fprintf(stderr,"Error in waveInAddBuffer: Unable to allocate or lock memory.");
		  break;
		  case WAVERR_UNPREPARED:
		    fprintf(stderr,"Error in waveInAddBuffer: The buffer pointed to by the pwh parameter hasn't been prepared.");
		  break;
		  default:
		    fprintf(stderr,"Error in waveInAddBuffer: Unknown error.");
		  }
          waveinaddbuf_error_seen = 1;
		  return 1;
		}
    }
    inbuf_index = (inbuf_index+1)%WAVE_BUFS;
  }
  wave_in_available -= a->unencoded_n;
  return a->unencoded_n;
}


int hdviper_register_audio_callback_win(Audio *a, void(*f)(Audio *)) {
  win_audio_in_callback = f;
}

int hdviper_deregister_audio_callback_win(Audio *a) {
  win_audio_in_callback = NULL;
}

void hdviper_destroy_audio_capture_win(Audio *a) {
  int i;

  waveInStop(audio_in_fd);
  waveInReset(audio_in_fd);
  waveInClose(audio_in_fd);
  for(i = 0; i < num_audio_inputs; i++)
	free(audio_input_list[i]);
  num_audio_inputs = 0;
}


