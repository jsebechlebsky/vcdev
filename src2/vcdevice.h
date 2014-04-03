#ifndef VCDEVICE_H
#define VCDEVICE_H

#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-core.h>

struct vc_device {
	dev_t dev_number;
	struct v4l2_device      v4l2_dev;
    struct video_device     vdev;
    struct vb2_queue        vb_out_vidq;
    char                    vc_fb_fname[16];
    struct proc_dir_entry*  vc_fb_procf;
    struct mutex            vc_mutex;
};

struct vc_device * create_vcdevice( size_t idx );
void destroy_vcdevice( struct vc_device * vcdev );

#endif