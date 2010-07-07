
/*
 *  This is the Matrox video capture for the Media Services of the 
 *  HDVIPER project.
 */

#include <stdio.h>
#include <mil.h>
#include <milproto.h>
#include "video_capture_matrox.h"

struct mil_info mil;

/* Hook function called when a frame grab ends  */
long MFTYPE grab_end(long HookType, MIL_ID EventId, void MPTYPE *UserPtr)
{
  Video *v=(Video*)UserPtr;

	/* OK, we have a buffer ready to be processed */
	mil.grabbed++;
	
	mil.grab_end_index = (mil.grab_end_index + 1) % MIL_GRAB_BUF_LEN;

  return 0;
}

/* Hook function called when a frame grab starts  */
long MFTYPE grab_start(long HookType, MIL_ID EventId, void MPTYPE *UserPtr)
{
  Video *v=(Video*)UserPtr;
	int signal=1;
	int cnt=0;
	int i;

  /* Set up next frame grab */
 
	/* Is the next buffer already occupied? */
	while (mil.buf_reference[mil.grab_start_index] > 0) {
		/* Yes: wait for buffer to be freed (by grab_video) */
		mil.defer_grab=1;
		i = MthrWait(mil.waitevent, M_EVENT_WAIT, M_NULL);
    if(i==M_TIMEOUT)
		//return 0;
	}

	if(mil.destroy_pending)
		return 0;

	mil.buf_reference[mil.grab_start_index]=1;

	/* Grab video frame into buffer */
  MdigGrab(mil.MilDigitizer, mil.MilImage[mil.grab_start_index]);
	//MdigGrabWait(mil.MilDigitizer, M_GRAB_NEXT_FRAME);

	/* Advance next-buffer pointer */
   mil.grab_start_index = (mil.grab_start_index + 1) % MIL_GRAB_BUF_LEN;
  //WishDebug("leaving grab_start");
  return 0;
}

int hdviper_setup_video_capture_matrox(Video *v) {
  MIL_ID status;
  int i;
  long dummy;

  printf("hdviper_setup_video_capture_matrox called\n");
  v->format = HDVIPER_VIDEO_FORMAT_BGRA;

  /* Allocate a MIL application. */
  MappAlloc(M_DEFAULT, mil.MilApplication);

  mil.destroy_pending = 0;

  /* Disable MIL errors to be displayed on screen */
	MappControl(M_ERROR, M_PRINT_DISABLE);

  /* Allocate MIL VIO System */
  MsysAlloc(M_SYSTEM_VIO, M_DEV0, M_DEFAULT, &mil.MilSystem);
  if(mil.MilSystem == M_NULL) {
			fprintf(stderr, "The Matrox Vio video capture board was not found.\n");
		  return 0;
	}

  MdispAlloc(mil.MilSystem, M_DEFAULT, "M_DEFAULT", M_WINDOWED, (MIL_ID *)&mil.MilDisplay);
  
	/* Allocate a MIL digitizer depending on the resolution */
	switch(v->width) {
	  case 768: /* PAL */
		  MdigAlloc(mil.MilSystem, M_DEV0, MIL_TEXT("Pal.dcf"), M_DEFAULT, &mil.MilDigitizer);
		  break;
	  case 640: /* NTSC */
		  MdigAlloc(mil.MilSystem, M_DEV0, MIL_TEXT("Ntsc.dcf"), M_DEFAULT, &mil.MilDigitizer);
		  break;
	  case 1920: /* 1080i */
		  MdigAlloc(mil.MilSystem, M_DEV0, MIL_TEXT("hd_yprpb_1920x1080i_25Hz.dcf"), M_DEFAULT, &mil.MilDigitizer);
			break;
    case 1280: /* 720p */
		  MdigAlloc(mil.MilSystem, M_DEV0, MIL_TEXT("hd_yprpb_1280x720p_50Hz.dcf"), M_DEFAULT, &mil.MilDigitizer);
      break;
	  default:
		  fprintf(stderr, "Unknown video resolution\n");
		  return 0;
	}

  /* Allocate an array of MIL buffers */
  for(i=0; i<MIL_GRAB_BUF_LEN; i++) {
	  status = MbufAllocColor(mil.MilSystem, 3, v->width, v->height, 8+M_UNSIGNED, M_IMAGE+M_GRAB+M_DISP+M_PACKED+M_BGR32+M_HOST_MEMORY, &mil.MilImage[i]);

    if(status==0 || mil.MilImage[i]==0) {
	    fprintf(stderr, "MIL buffer allocation failed\n");
	    return 0;
		}
  }

	/* Grab start hook function */
	MdigHookFunction(mil.MilDigitizer, M_GRAB_START, grab_start, (void *)v);
	MdigHookFunction(mil.MilDigitizer, M_GRAB_END, grab_end, (void *)v);

  MappControl(M_ERROR, M_PRINT_DISABLE);

	v->buffer = (char *)MbufInquire(mil.MilImage[0], M_HOST_ADDRESS, &dummy);
  if(v->buffer==M_NULL) {
	  fprintf(stderr, "MIL buffer inquire failed\n");
	  return 0;
  } 
	

  /* Set asynchronous grab */
  MdigControl(mil.MilDigitizer, M_GRAB_MODE, M_ASYNCHRONOUS);
	MdigControl(mil.MilDigitizer, M_CAMERA_LOCK, M_DISABLE);

  /* Clear the grab buffers */
  /*for(i=0; i<MIL_GRAB_BUF_LEN; i++)
    MbufClear(mil.MilImage[i], 0);*/

  mil.grabbed = 0;
  mil.grab_index = 0;
	mil.grab_start_index = 1;
	mil.grab_end_index = 0;
	for(i=0; i<MIL_GRAB_BUF_LEN; i++) {
    MbufClear(mil.MilImage[i], 0);
	  mil.buf_reference[i] = 0;
	}

  mil.buf_reference[0] = 1;
	mil.defer_grab=0;
  mil.reallocate_digitizer = 0;

  MthrAlloc(mil.MilSystem, M_EVENT, M_DEFAULT, M_NULL, M_NULL, (MIL_ID *)&(mil.waitevent));

	gettimeofday(&mil.watchdog_timer, NULL);
}

int hdviper_capture_video_frame_matrox(Video *v) {
  int signal=0;
  int i;
	long dummy;
	struct timeval now;

  /* Get image data from the grabber board */

  /* Is a new frame available? (shouldn't get here if grabbed==0) */
  if(mil.grabbed==0) {
	fprintf(stderr, "Error: hdviper_capture_video_frame_matrox called with grabbed=0.\n");
	  return;
  }

  if(mil.buf_reference[mil.grab_index]==0) {
	  fprintf(stderr, "Error: hdviper_capture_video_frame_matrox called with buf_ref=0.\n");
	  return;
  }

  v->buffer = (char *)MbufInquire(mil.MilImage[mil.grab_index], M_HOST_ADDRESS, &dummy);
  if(v->buffer==M_NULL) {
		fprintf(stderr, "MbufInquire returns M_NULL\n");
	    /* This could possibly happen if there's not enough paged memory available for MIL */
	    mil.buf_reference[mil.grab_index]=0;
	    mil.grab_index = (mil.grab_index+1)%MIL_GRAB_BUF_LEN;
	    mil.grabbed--;
	    return;
  }

  mil.grabbed--;
	mil.buf_reference[mil.grab_index]=0;
	mil.grab_index = (mil.grab_index+1)%MIL_GRAB_BUF_LEN;

	if(mil.defer_grab) {
		if (mil.grabbed <= 1) { /* Is this necessary? */
		  	mil.defer_grab=0;
				MthrControl(mil.waitevent, M_EVENT_SET, M_SIGNALED);
		}
  }
}

int hdviper_register_video_callback_matrox(Video *v, void(*f)(Video *)) {
}

int hdviper_deregister_video_callback_matrox(Video *v) {
}

void hdviper_destroy_video_capture_matrox(Video *v) {
	mil.destroy_pending = 1;

  fprintf(stderr, "hdviper_destroy_video_capture_matrox called\n");

	/* Unhook grabbing funtions */
	MdigHookFunction(mil.MilDigitizer, M_GRAB_END+M_UNHOOK, grab_end, (void *)v);

	/* Send a few wakeup signals and wait for sleeping threads to finish */
	for(i=0; i<MIL_GRAB_BUF_LEN; i++)
	  mil.buf_reference[i] = 0;

	for(i=0; i<MIL_GRAB_BUF_LEN; i++) {
		MthrControl(mil.waitevent, M_EVENT_SET, M_SIGNALED);
	}
	/* for some reason, this should be done after the wake-up of sleeping threads */
	MdigHookFunction(mil.MilDigitizer, M_GRAB_START+M_UNHOOK, grab_start, (void *)v);

	MthrFree(mil.waitevent);

  /* Free allocated objects. */
  for(i=0; i<MIL_GRAB_BUF_LEN; i++)
    MbufFreee(mil.MilImage[i]);

  MdispFree(mil.MilDisplay);
  MdigFree(mil.MilDigitizer);
  MsysFree(mil.MilSystem);
  MappFree(mil.MilApplication);

}


