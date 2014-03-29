#ifndef VCIOCTL_H
#define VCIOCTL_H
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>

int vcdev_querycap( struct file * file, void * priv,
	struct v4l2_capability * cap );

int vcdev_enum_input( struct file * file, void * priv,
                              struct v4l2_input * inp);

int vcdev_g_input( struct file * file, void * priv,
                           unsigned int * i );

int vcdev_s_input( struct file * file, void * priv,
                           unsigned int i );

#endif