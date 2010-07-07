
/*
 *  This is the BlackMagic Decklink HD video capture for the Media Services 
 *  of the HDVIPER project.
 */

#include <stdio.h>
#include "video_capture_blackmagic.h"

#if !defined(HDVIPER_VIDEO_CAPTURE_BLACKMAGIC) || !defined(WIN32) 
#error "This grabber is for BlackMagic/DirectShow/win32 use only"
#endif

#include <windows.h>
#include <tchar.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <streams.h>
#include <xprtdefs.h> 
#include <dbt.h>
#include <msacm.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <commdlg.h>
#include <Dvdmedia.h>
#include <Errors.h>
#include <sys/timeb.h>

#include "dshow.h"

static int COM_initialized = 0;
static int decklink_dev_index=-1;
static int default_video_input_num=0;

//static int input_selected = 0;
//static int low_framerate_selected = 0;

struct ds_capture {
	IMoniker *device; /* Video capture device */
    ICaptureGraphBuilder2 *graph_builder;
    IGraphBuilder *filter_graph_manager;
	IBaseFilter *source_filter; /* Filter created from video capture device */
	IAMStreamConfig *DV_source_interface;
	IBaseFilter *RGB_sample_grabber;	/* RGB sample grabber filter */
	ISampleGrabber *RGB_sample_grabber_interface;
	IBaseFilter *video_preview;	/* Video renderer filter */
	IMediaControl *graph_control;	/* filter graph control interface */
    IVideoWindow *preview_window;
	int running;		/* is graph running? */
	int compression_algorithm; 
	int drop_count;	/* Drop this number of DV frames for every transmitted */
	int d_cnt;	/* actual drop counter variable */
	char node_names[1000];
};

void init_ds_capture_structure(struct ds_capture*);

extern "C" {
  int ds_init_video_hardware(Video *v);
  int ds_setup_video_grabber(Video *v);
  void ds_reconfigure_video_grabber(Video *v);
  int ds_select_video_input(Video *v);
  void ds_set_video_grabber_qfactor(Video *v);
  void ds_set_video_grabber_fps(Video *v);
  void ds_grab_video(Video *v);
  void ds_dispose_grabbed_frame(Video *v);
  void ds_destroy_video_grabber(Video *v);
  void ds_enable_live_video(Video *v, int live);
  int ds_grabber_installed(Video *v);
  int ds_get_video_nodes(char *str);
  int ds_get_default_input();
  int ds_get_default_standard(int nodenum);
  int ds_grabber_is_ready(Video *v);
  int setup_video_compressor(Video *v, int type);
  void destroy_video_compressor(Video *v, int type);
  void compress_video(Video *v);
	int read_video_image(Video *v, int type, char *image_filename, char *img_buf, int not32bit);
	void debug_msg(int level , char *fmt, ...);
	void update_motion_detection_pixels_rgb(Video *v, int comps, int ri, int gi, int bi);
	int time_is_after(struct timeval now, struct timeval mark);
  int gettimeofday(struct timeval *, struct timezone *);
	void setup_video_preprocessing(Video *v);
  void destroy_video_preprocessing(Video *v);
  void swap_preproc_buffers(Video *v);
  void preprocess_video(Video *v);
	int gettimeofday_multicore(struct timeval *curTimeP, struct timezone *tz);
	void init_gettimeofday_multicore();
	int read_video_image(Video *v, int type, char *image_filename, char *img_buf, int not32bit);
	extern struct _timeb startTOD;
	void local_render(Video *v);
}


void init_ds_capture_structure(struct ds_capture *cap) {
	cap->compression_algorithm = 0;
	cap->device = NULL;
	cap->DV_source_interface = NULL;
	cap->filter_graph_manager = NULL;
	cap->graph_builder = NULL;
	cap->graph_control = NULL;
	cap->preview_window = NULL;
	cap->RGB_sample_grabber = NULL;
	cap->RGB_sample_grabber_interface = NULL;
	cap->source_filter = NULL;
	cap->video_preview = NULL;
	cap->running = 0;
}


/** Check for grabber hardware **/

int ds_grabber_installed(Video *v)  
{
	HRESULT hr;

	if (v->ds_cap == NULL) return 0;
	ds_get_video_nodes((char*)&(v->ds_cap->node_names));	
	/* might need to initialize COM */
	if(!COM_initialized)
	  CoInitialize(NULL);
  COM_initialized = 1;

    /* enumerate all video capture devices */
    ICreateDevEnum *pCreateDevEnum;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if(hr != S_OK) {
        WishDebug(3, "Error Creating Device Enumerator");
        return 0;
    }

	/* find video input devices */
    IEnumMoniker *pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
    if(hr != S_OK) {
		 /*pEm->Release();*/
        return 0;
    }
    pEm->Release();

	return 1;
}

int hdviper_setup_video_capture_blackmagic(Video *v) {
	// Create ds_cap structure 
	if (v->ds_cap == NULL) {
		v->ds_cap = (struct ds_capture*) malloc(sizeof(struct ds_capture));
		init_ds_capture_structure(v->ds_cap);
	}

	/* enumerate all video capture devices */
	ICreateDevEnum *pCreateDevEnum;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
												IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if(hr != S_OK) {
		fprintf(stderr, "Error Creating Device Enumerator\n");
		return 0;
	}

	/* enumerate video input devices */
	IEnumMoniker *pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if(hr != S_OK) {
		fprintf(stderr, "Sorry, you have no video capture hardware");
		return 0;
	}

	pEm->Reset();

	ULONG cFetched;
	IMoniker *pM;
	UINT uIndex = 0;

	/* find the selected device */
	hr = pEm->Next(1, &pM, &cFetched);
	if(hr != S_OK) return 0;
	while(uIndex++ < default_video_input_num) {
		hr = pEm->Next(1, &pM, &cFetched);
		if(hr!=S_OK) return 0;
	}
	if((v->ds_cap->device) != pM ) {
		if((v->ds_cap->device) != NULL) v->ds_cap->device->Release();
		pM->AddRef();
		v->ds_cap->device = pM;
	}
	if(!COM_initialized)
	  CoInitialize(NULL);
  COM_initialized = 1;

    /* enumerate all video capture devices */
    ICreateDevEnum *pCreateDevEnum;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if(hr != S_OK) {
        fprintf(stderr, "Error Creating Device Enumerator");
        return 0;
    }

	/* find video input devices */
    IEnumMoniker *pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
    if(hr != S_OK) {
		 /*pEm->Release();*/
        return 0;
    }
    pEm->Release();


  return 1;
}

int hdviper_capture_video_frame_blackmagic(Video *v) {
	HRESULT hr, hr2;
	long bufsize;
	struct timeval now;
	int i, j;

      /* Get size of RGB data buffer */
	  hr = v->ds_cap->RGB_sample_grabber_interface->GetCurrentBuffer(&bufsize, NULL);
	  /* Grab the actual image. */
	  hr2 = v->ds_cap->RGB_sample_grabber_interface->GetCurrentBuffer(&bufsize, reinterpret_cast<long*>(v->yuv));
      if(hr != S_OK || hr2 != S_OK) {
	    /* fprintf(stderr, "GetCurrentBuffer failed"); */
	    return 0;
	  }

  return bufsize;
}

int hdviper_register_video_callback_blackmagic(Video *v, void(*f)(Video *)) {
}

int hdviper_deregister_video_callback_blackmagic(Video *v) {
}

/* Tear down everything downstream of a given filter */
void destroy_ds_filter_graph(IBaseFilter *pf, Video *v)
{
    IPin *pP, *pTo;
    ULONG u;
    IEnumPins *pins = NULL;
    PIN_INFO pininfo;
    
	HRESULT hr = pf->EnumPins(&pins);
    pins->Reset();
    
	while(hr == S_OK) {
        hr = pins->Next(1, &pP, &u);
        if(hr == S_OK && pP) {
            pP->ConnectedTo(&pTo);
            if(pTo) {
                hr = pTo->QueryPinInfo(&pininfo);
                if(hr == S_OK) {
                    if(pininfo.dir == PINDIR_INPUT) {
                        destroy_ds_filter_graph(pininfo.pFilter, v);
                        v->ds_cap->filter_graph_manager->Disconnect(pTo);
                        v->ds_cap->filter_graph_manager->Disconnect(pP);
                        v->ds_cap->filter_graph_manager->RemoveFilter(pininfo.pFilter);
                    }
                    pininfo.pFilter->Release();
                }
                pTo->Release();
            }
            pP->Release();
        }
    }
    if(pins)
        pins->Release();
}

void hdviper_destroy_video_capture_blackmagic(Video *v) {
	if (v->ds_cap->running == 1) {
	  hr = v->ds_cap->graph_control->Stop();
	  if(hr != S_OK) {
	    fprintf(stderr, "Couldn't stop filter graph");
	    return;
	  }
      v->ds_cap->running = 0;
	}

    if(v->ds_cap->source_filter)
        destroy_ds_filter_graph(v->ds_cap->source_filter, v);

	/* Delete COM objects */
	if (v->ds_cap->graph_builder)
		v->ds_cap->graph_builder->Release();
	if (v->ds_cap->filter_graph_manager)
		v->ds_cap->filter_graph_manager->Release();
	if (v->ds_cap->source_filter)
		v->ds_cap->source_filter->Release();
	if (v->ds_cap->video_preview)
		v->ds_cap->video_preview->Release();
	if (v->ds_cap->graph_control)
		v->ds_cap->graph_control->Release();

}


