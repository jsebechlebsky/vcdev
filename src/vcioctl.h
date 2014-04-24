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

int vcdev_enum_fmt_vid_cap( struct file * file, void * priv,
                                    struct v4l2_fmtdesc * f);

int vcdev_g_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f);

int vcdev_try_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f);

int vcdev_s_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f);

int vcdev_enum_frameintervals(struct file *filp, void *priv,
                                     struct v4l2_frmivalenum *fival);

int vcdev_g_parm(struct file *fil, void * priv,
                struct v4l2_streamparm *a);

int vcdev_s_parm(struct file *fil, void * priv,
                struct v4l2_streamparm *a);

#endif