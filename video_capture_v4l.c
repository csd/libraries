
/*
 *  This is the Video for Linux version of the video capture module of the 
 *  Media Services of the HDVIPER project.
 */

#ifndef HDVIPER_VIDEO_CAPTURE_VIDEO4LINUX
#error "This file requires Video for Linux support"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <asm/types.h> /* for videodev2.h */
#include <linux/videodev2.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>

#include "video_capture_v4l.h"
#include "video_conversion.h"

#define TRACE_FUNCTIONS(fmt, args...)	\
	printf("%s " fmt "\n", __func__ , ## args);
#define INT_TO_FOURCC(x) \
	(x & 0x000000FF), \
	((x & 0x0000FF00) >> 8), \
	((x & 0x00FF0000) >> 16), \
	((x & 0xFF000000) >> 24)

static char dev_name[] = "/dev/video0";

v4l2_data_t *v4l2_data;

void v4l2_convert_to_BGRA(Video *v, int pixel_type);
char *v4l2_buf_type_to_string(int buf_type);
char *v4l2_capture_capability_to_string(int capability);
char *v4l2_colorspace_to_string(int colorspace);
char *v4l2_pix_field_to_string(int field);

int my_ioctl(int fd, int request, void *arg) {
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while (-1 == ret && EINTR == errno);

	return ret;
}

void v4l2_init_data() {
	v4l2_data = (v4l2_data_t *) calloc(1, sizeof(v4l2_data_t));
	v4l2_data->fd = -1;
}

void v4l2_list_pixel_formats(v4l2_data_t *v4l2_data) {
	struct v4l2_fmtdesc fmt;
	int index = 0;
	struct v4l2_fmtdesc fmts[10];
	int num_formats = 0;
	int i;

	TRACE_FUNCTIONS("BEGIN")
	while(1) {
		memset(&fmt, 0, sizeof(fmt));
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.index = index;
		if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_ENUM_FMT, &fmt)) {
			goto OUT;
		}
		fmts[index] = fmt;
		index++;
		num_formats++;
		/* test */
		if (fmt.flags == 0) {
			v4l2_data->pixel_format = fmt.pixelformat;
		}
	}
OUT:
	if (num_formats > 0) {
		printf("Supported pixel formats are:\n");
		for (i=0; i<num_formats; i++) {
			printf("    index %d\n"
				"\tflags\t\t%x\n"
				"\tdescription\t%s\n"
				"\tpixelformat\t%c%c%c%c\n",
				i,
				fmts[i].flags,
				fmts[i].description,
				INT_TO_FOURCC(fmts[i].pixelformat));
		}
	} else {
		printf("Could not get list of supported pixel formats\n");
	}
	printf("Selected pixelformat %c%c%c%c\n", INT_TO_FOURCC(fmts[i].pixelformat));
	TRACE_FUNCTIONS("END")
}

void v4l2_list_frame_intervals(v4l2_data_t *v4l2_data, int width, int height) {
	struct v4l2_frmivalenum fi;
	int index = 0;
	struct frameinterval {
		int n;
		int d;
	};
	struct frameinterval intervals[10];
	int num_intervals = 0;
	int i;

	TRACE_FUNCTIONS("BEGIN");
	while (1) {
		memset(&fi, 0, sizeof(fi));
                fi.index = index;
                fi.pixel_format = v4l2_data->pixel_format;
                fi.width = width;
                fi.height = height;
		fi.type = V4L2_FRMIVAL_TYPE_DISCRETE;
		index++;
		if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_ENUM_FRAMEINTERVALS, &fi)) {
			goto OUT;
		}
		switch(fi.type) {
		case V4L2_FRMIVAL_TYPE_DISCRETE:
			intervals[num_intervals].n = fi.discrete.numerator;
			intervals[num_intervals].d = fi.discrete.denominator;
			num_intervals++;
			break;
		case V4L2_FRMIVAL_TYPE_CONTINUOUS:
			printf("Type is continuous\n");
			break;
		case V4L2_FRMIVAL_TYPE_STEPWISE:
			printf("Type is stepwise - min=(%d|%d) max=(%d|%d) step=(%d|%d)\n",
			fi.stepwise.min.numerator,fi.stepwise.min.denominator,
			fi.stepwise.max.numerator,fi.stepwise.max.denominator,
			fi.stepwise.step.numerator,fi.stepwise.step.denominator);
			break;
		}
	}
OUT:
	if (num_intervals > 0) {
		printf("Supported frame intervals for size %dx%d are:\n", width, height);
		for (i=0; i<num_intervals; i++) {
			printf("\tnumerator %3d, denominator %3d\n", intervals[i].n, intervals[i].d);
		}
	}

	TRACE_FUNCTIONS("END");
}

void v4l2_list_frame_sizes(v4l2_data_t *v4l2_data) {
	struct v4l2_frmsizeenum fs;
	int index = 0;
	struct framesize {
		int w;
		int h;
	};
	struct framesize sizes[10];
	int num_sizes = 0;
	int i;

	TRACE_FUNCTIONS("BEGIN");
	while (1) {
		memset(&fs, 0, sizeof(fs));
		fs.index = index;
		fs.pixel_format = v4l2_data->pixel_format;
		index++;
		if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_ENUM_FRAMESIZES, &fs)) {
			goto OUT;
		}
		switch(fs.type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			sizes[num_sizes].w = fs.discrete.width;
			sizes[num_sizes].h = fs.discrete.height;
			num_sizes++;
			v4l2_list_frame_intervals(v4l2_data, fs.discrete.width, fs.discrete.height);
			break;
		case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			printf("Frame size type is continuous\n");
			break;
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			printf("Frame size type is stepwise\n");
			break;
		}
	}
OUT:
	if (num_sizes > 0) {
		printf("Supported sizes are:\n");
		for (i=0; i<num_sizes; i++) {
			printf("width %3d, height %3d\n", sizes[i].w, sizes[i].h);
		}
	}
	TRACE_FUNCTIONS("END");
}

void v4l2_print_current_parm(v4l2_data_t *v4l2_data) {
	struct v4l2_streamparm sp;

	TRACE_FUNCTIONS("BEGIN")
	memset(&sp, 0, sizeof(sp));
	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_G_PARM, &sp)) {
		printf("VIDIOC_G_PARM failed (%s)\n", strerror(errno));
		goto OUT;
	}
	printf("Current capture parameters are: \n"
		"\ttype\t\t%d (%s)\n"
		"\tcapability\t0x%x (%s)\n"
		"\tcapturemode\t%x\n"
		"\ttimeperframe\t%u / %u\n"
		"\textendedmode\t%d\n",
		sp.type, v4l2_buf_type_to_string(sp.type),
		sp.parm.capture.capability,
		v4l2_capture_capability_to_string(sp.parm.capture.capability),
		sp.parm.capture.capturemode,
		sp.parm.capture.timeperframe.numerator,
		sp.parm.capture.timeperframe.denominator,
		sp.parm.capture.extendedmode);

OUT:
	TRACE_FUNCTIONS("END");
}

void v4l2_get_current_format(v4l2_data_t *v4l2_data, struct v4l2_format *fmt) {
	memset(fmt, 0, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_G_FMT, fmt)) {
                printf("VIDIOC_G_FMT failed.\n");
                return;
	}
	printf("Current format:\n"
		"\twidth\t\t%d\n"
		"\theight\t\t%d\n"
		"\tformat\t\t%c%c%c%c\n"
		"\tfield\t\t%d (%s)\n"
		"\tbytesperline\t%d\n"
		"\tsizeimage\t%d\n"
		"\tcolorspace\t%d (%s)\n",
		fmt->fmt.pix.width,
		fmt->fmt.pix.height,
		INT_TO_FOURCC(fmt->fmt.pix.pixelformat),
		fmt->fmt.pix.field,
		v4l2_pix_field_to_string(fmt->fmt.pix.field),
		fmt->fmt.pix.bytesperline,
		fmt->fmt.pix.sizeimage,
		fmt->fmt.pix.colorspace,
		v4l2_colorspace_to_string(fmt->fmt.pix.colorspace));
}

void init_mmap(v4l2_data_t *v4l2_data) {
	struct v4l2_requestbuffers req;
	unsigned int i;

	TRACE_FUNCTIONS("BEGIN")
	memset(&req, 0, sizeof(req));
	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			printf("%s does not support memory mapping\n", dev_name);
			return;
		} else {
			printf("Error trying to allocate memory mapped buffers (%s)\n", strerror(errno));
			return;
		}
	}
	if (req.count < 2) {
		printf("Insufficient buffer memory on %s\n", dev_name);
		return;
	}
	v4l2_data->mmap_buffers = calloc(req.count, sizeof(struct mmap_buffer_t));
	for (v4l2_data->n_buffers = 0; v4l2_data->n_buffers < req.count; ++v4l2_data->n_buffers) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = v4l2_data->n_buffers;
		if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_QUERYBUF, &buf)) {
			printf("VIDIOC_QUERYBUF failed (%s)\n", strerror(errno));
			return;
		}
		v4l2_data->mmap_buffers[v4l2_data->n_buffers].length = buf.length;
		v4l2_data->mmap_buffers[v4l2_data->n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, v4l2_data->fd, buf.m.offset);
		if (MAP_FAILED == v4l2_data->mmap_buffers[v4l2_data->n_buffers].start) {
			printf("MMAP failed.\n");
			return;
		}
	}
	for (i = 0; i < v4l2_data->n_buffers; ++i) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;
		if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_QBUF, &buf)) {
			printf("VIDIOC_QBUF failed (%s)\n", strerror(errno));
			return;
		}
	}
	TRACE_FUNCTIONS("END")
}

void release_mmap(v4l2_data_t *v4l2_data) {
	int i;
	TRACE_FUNCTIONS("BEGIN")
	for (i = 0; i < v4l2_data->n_buffers; ++i) {
		if (-1 == munmap(v4l2_data->mmap_buffers[i].start, v4l2_data->mmap_buffers[i].length)) {
			printf("munmap failed (%s)", strerror(errno));
		}
	}
	free(v4l2_data->mmap_buffers);
	TRACE_FUNCTIONS("END")
}

int v4l2_open_device(v4l2_data_t *v4l2_data) {
        struct stat st;

	TRACE_FUNCTIONS("BEGIN")
	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
			dev_name, errno, strerror (errno));
		return 0;
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		return 0;
	}

	v4l2_data->fd = open(dev_name, O_RDWR, 0);
	if (-1 == v4l2_data->fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
			dev_name, errno, strerror(errno));
		return 0;
	}

	TRACE_FUNCTIONS("END")
	return 1;
}

/*
 * Set up the video capture board
 */
int hdviper_setup_video_capture_v4l(Video *v) {
	int ret = 0;
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	enum v4l2_buf_type type;

	TRACE_FUNCTIONS("BEGIN")
	v4l2_init_data();
	ret = v4l2_open_device(v4l2_data);
	if (0 == ret) {
		printf("Error opening device. Can't use grabber.\n");
		return 0;
	}

	// Device is opened, query it for capabilities
	ret = my_ioctl(v4l2_data->fd, VIDIOC_QUERYCAP, &cap);
	if (ret == -1) {
		if (errno == EINVAL) {
			printf("%s is not a V4L2 device. Exiting.\n", dev_name);
			exit(-1);
		} else {
			printf("VIDIOC_QUERYCAP failed. Can't use device.\n");
			return 0;
		}
	}
	printf("Reported capabilities:\n"
		"\tDriver\t\t%s\n"
		"\tCard\t\t%s\n"
		"\tbus_info\t%s\n"
		"\tVersion\t\t%d\n"
		"\tcapabilities\t%x\n",
		cap.driver, cap.card, cap.bus_info, cap.version,
		cap.capabilities);
#define PRINT_CAP(x, y) \
	if (cap.capabilities & x) printf(y);

	PRINT_CAP(V4L2_CAP_VIDEO_CAPTURE, "VIDEO_CAPTURE, ");
	PRINT_CAP(V4L2_CAP_VIDEO_OUTPUT, "VIDEO_OUTPUT, ");
	PRINT_CAP(V4L2_CAP_VIDEO_OVERLAY, "VIDEO_OVERLAY, ");
	PRINT_CAP(V4L2_CAP_VBI_CAPTURE, "VBI_CAPTURE, ");
	PRINT_CAP(V4L2_CAP_VBI_OUTPUT, "VBI_OUTPUT, ");
	PRINT_CAP(V4L2_CAP_SLICED_VBI_CAPTURE, "SLICED_VBI_CAPTURE, ");
	PRINT_CAP(V4L2_CAP_SLICED_VBI_OUTPUT, "SLICED_VBI_OUTPUT, ");
	PRINT_CAP(V4L2_CAP_RDS_CAPTURE, "RDS_CAPTURE, ");
	PRINT_CAP(V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "VIDEO_OUTPUT_OVERLAY, ");
	PRINT_CAP(V4L2_CAP_TUNER, "TUNER, ");
	PRINT_CAP(V4L2_CAP_AUDIO, "AUDIO, ");
	PRINT_CAP(V4L2_CAP_RADIO, "RADIO, ");
	PRINT_CAP(V4L2_CAP_READWRITE, "READWRITE, ");
	PRINT_CAP(V4L2_CAP_ASYNCIO, "ASYNCIO, ");
	PRINT_CAP(V4L2_CAP_STREAMING, "STREAMING");
	printf("\n");

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("%s is not a video capture device\n", dev_name);
		return 0;
	}
	/* Select video input, video standard and tune here. */
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == my_ioctl(v4l2_data->fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */
		if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_S_CROP, &crop)) {
			if (EINVAL == errno) {
				printf("Cropping not supported.\n");
			} else {
				printf("Error related to cropping (%s)\n", strerror(errno));
			}
		}
	} else {
		/* Errors ignored. */
	}

	v4l2_list_pixel_formats(v4l2_data);
	v4l2_list_frame_sizes(v4l2_data);
	v4l2_print_current_parm(v4l2_data);
	v4l2_get_current_format(v4l2_data, &fmt);

	fmt.fmt.pix.width       = 320;
	fmt.fmt.pix.height      = 240;
	fmt.fmt.pix.pixelformat = v4l2_data->pixel_format;
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_S_FMT, &fmt)) {
		TRACE_FUNCTIONS("VIDIOC_S_FMT failed.")
		return 0;
	}
	printf("Format after setting:\n");
	v4l2_get_current_format(v4l2_data, &fmt);

	// Set up mmap
	init_mmap(v4l2_data);
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_STREAMON, &type)) {
		printf("VIDIOC_STREAMON failed (%s)\n", strerror(errno));
		return 0;
	}


	TRACE_FUNCTIONS("END")
	return ret;
}

/*
 * Get one frame of video from the video device
 */
int hdviper_capture_video_frame_v4l(Video *v) {
	struct v4l2_buffer buf;
	unsigned int i;

	//TRACE_FUNCTIONS("BEGIN")

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
		case EAGAIN:
			//printf("eagain\n");
			return;
		case EIO:
			/* Could ignore EIO, see spec. */
			/* fall through */
		default:
			printf("VIDIOC_DQBUF failed");
			return;
		}
	}
	//printf("Efter VIDIOC_DQBUF -- %s %d\n", __FILE__, __LINE__);
	assert(buf.index < v4l2_data->n_buffers);
	memcpy(v->yuv, v4l2_data->mmap_buffers[buf.index].start, v4l2_data->mmap_buffers[buf.index].length);
	if (-1 == my_ioctl(v4l2_data->fd, VIDIOC_QBUF, &buf)) {
		printf("VIDIOC_QBUF failed.\n");
	}

	//TRACE_FUNCTIONS("END")
}

int hdviper_register_video_callback_v4l(Video *v, void(*f)(Video *)) {
}

int hdviper_deregister_video_callback_v4l(Video *v) {
}

void hdviper_destroy_video_capture_v4l(Video *v) {
	int res;
	TRACE_FUNCTIONS("BEGIN")
	res = close(v4l2_data->fd);
	if (res == -1) {
		printf("closing device failed (%s)\n", strerror(errno));
	}
	release_mmap(v4l2_data);
	free(v4l2_data);
	TRACE_FUNCTIONS("END")
}


/* 
 * Converts camera image to 32-bit rgb.
 */
void v4l2_convert_to_BGRA(Video *v, int pixel_type) {
	switch(pixel_type) {
	case V4L2_PIX_FMT_YUV420:
		yuv420p_to_bgra(v->width, v->height, v->rgb, v->yuv);
		break;
	case V4L2_PIX_FMT_YUYV:
		yuyv_to_bgra(v->width, v->height, v->rgb, v->yuv);
		break;
	case V4L2_PIX_FMT_MJPEG:
		break;
	default:
		printf("Unknown pixel type - can't convert image.\n");
	}
}

char *v4l2_buf_type_to_string(int buf_type) {
	char *str;
	switch(buf_type) {
        case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        	str = "V4L2_BUF_TYPE_VIDEO_CAPTURE";
		break;
        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        	str = "V4L2_BUF_TYPE_VIDEO_OUTPUT";
		break;
        case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        	str = "V4L2_BUF_TYPE_VIDEO_OVERLAY";
		break;
        case V4L2_BUF_TYPE_VBI_CAPTURE:
        	str = "V4L2_BUF_TYPE_VBI_CAPTURE";
		break;
        case V4L2_BUF_TYPE_VBI_OUTPUT:
        	str = "V4L2_BUF_TYPE_VBI_OUTPUT";
		break;
        case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
        	str = "V4L2_BUF_TYPE_SLICED_VBI_CAPTURE";
		break;
        case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
        	str = "V4L2_BUF_TYPE_SLICED_VBI_OUTPUT";
		break;
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
        	str = "V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY";
		break;
	default:
		printf("Unknow V4L2 buffer type.\n");
		str = "Unknown";
	}
	return str;
}

char *v4l2_capture_capability_to_string(int capability) {
	static char str[60];
	int known=0;
	memset(str, 0, 60);
	if (capability & V4L2_MODE_HIGHQUALITY) {
		sprintf(str, "V4L2_MODE_HIGHQUALITY");
		known = 1;
		capability &= ~V4L2_MODE_HIGHQUALITY;
		if (capability != 0)
			sprintf(str, ", ");
	}
	if (capability & V4L2_CAP_TIMEPERFRAME) {
		sprintf(str, "V4L2_CAP_TIMEPERFRAME");
		known = 1;
		capability &= ~V4L2_CAP_TIMEPERFRAME;
		if (capability != 0)
			sprintf(str, ", ");
	}
	if (!known) {
		sprintf(str, "Unknown");
	}
	return str;
}

char *v4l2_colorspace_to_string(int colorspace) {
	char *str;
	switch(colorspace) {
        case V4L2_COLORSPACE_SMPTE170M:
		str = "SMPTE170M";
		break;
        case V4L2_COLORSPACE_SMPTE240M:
		str = "SMPTE240M";
		break;
        case V4L2_COLORSPACE_REC709:
		str = "REC709";
		break;
        case V4L2_COLORSPACE_BT878:
		str = "BT878";
		break;
        case V4L2_COLORSPACE_470_SYSTEM_M:
		str = "470_SYSTEM_M";
		break;
        case V4L2_COLORSPACE_470_SYSTEM_BG:
		str = "470_SYSTEM_BG";
		break;
        case V4L2_COLORSPACE_JPEG:
		str = "JPEG";
		break;
        case V4L2_COLORSPACE_SRGB:
		str = "SRGB";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

char *v4l2_pix_field_to_string(int field) {
	char *str;
	switch(field) {
        case V4L2_FIELD_ANY:
		str = "V4L2_FIELD_ANY";
		break;
        case V4L2_FIELD_NONE:
		str = "V4L2_FIELD_NONE";
		break;
        case V4L2_FIELD_TOP:
		str = "V4L2_FIELD_TOP";
		break;
        case V4L2_FIELD_BOTTOM:
		str = "V4L2_FIELD_BOTTOM";
		break;
        case V4L2_FIELD_INTERLACED:
		str = "V4L2_FIELD_INTERLACED";
		break;
        case V4L2_FIELD_SEQ_TB:
		str = "V4L2_FIELD_SEQ_TB";
		break;
        case V4L2_FIELD_SEQ_BT:
		str = "V4L2_FIELD_SEQ_BT";
		break;
        case V4L2_FIELD_ALTERNATE:
		str = "V4L2_FIELD_ALTERNATE";
		break;
        case V4L2_FIELD_INTERLACED_TB:
		str = "V4L2_FIELD_INTERLACED_TB";
		break;
        case V4L2_FIELD_INTERLACED_BT:
		str = "V4L2_FIELD_INTERLACED_BT";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

