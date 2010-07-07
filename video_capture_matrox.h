#ifndef VIDEO_CAPTURE_MATROX_H
#define VIDEO_CAPTURE_MATROX_H

#include <mil.h>
#include "video.h"

int hdviper_setup_video_capture_matrox(Video *);
int hdviper_capture_video_frame_matrox(Video *);
int hdviper_register_video_callback_matrox(Video *, void(*f)(Video *));
int hdviper_deregister_video_callback_matrox(Video *);
void hdviper_destroy_video_capture_matrox(Video *);


#ifndef M_SYSTEM_VIO
#define M_SYSTEM_VIO MIL_TEXT("\\\\.\\M_SYSTEM_VIO")
#endif

#define MIL_JPEG_BUF_LEN 3
#define MIL_GRAB_BUF_LEN 3

struct mil_info {
  MIL_ID   MilApplication;
  MIL_ID   MilSystem;
  MIL_ID   MilDigitizer;
  MIL_ID   MilDigitizer2; /* used by Orion boards */
  MIL_ID   MilDisplay;
  MIL_ID   MilImage[MIL_GRAB_BUF_LEN]; /* image buffer array */
  MIL_ID   MilImage2; /* used by Orion boards */
  MIL_ID   MilJPEGImage;
  MIL_ID   MilImageDisp;
  MIL_ID   MilQtableLum;
  MIL_ID   MilQtableCr;

  int	   grabbed; /* number of frames currently in grab buffer */
  int	   grab_index; /* index in the grab buffer array of the next frame to be processed  */
	int	   grab_start_index; /* index in the grab buffer array where the next grab will be initiated  */
	int	   grab_end_index; /*  index in the grab buffer array where the next grab buffer will be finished */
	int    buf_reference[MIL_GRAB_BUF_LEN]; /* ref count for frames in grab buffer */
	int		 destroy_pending; /* set when grabbing should stop, because we're destoying the video compressor */

  MIL_ID waitevent;
	int defer_grab;
	int grab_error_pending;
};

#endif

