#ifndef VCMODAPI_H
#define VCMODAPI_H

#define VCMOD_IOCTL_CREATE_DEVICE        0x1
#define VCMOD_IOCTL_DESTROY_DEVICE       0x2
#define VCMOD_IOCTL_GET_DEVICE           0x3
#define VCMOD_IOCTL_ENUM_DEVICES         0x4
#define VCMOD_IOCTL_MODIFY_SETTING       0x5

#define VCMOD_PIXFMT_RGB24         0x01
#define VCMOD_PIXFMT_YUYV          0x02

struct vcmod_device_spec {
	unsigned int   idx;
	unsigned short width;
	unsigned short height;
	unsigned char  pix_fmt;
	char  video_node[64];
	char  fb_node[64];
};

#endif