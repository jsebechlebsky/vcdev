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
#include "vcmod_api.h"

#define PIXFMTS_MAX 16
#define FB_NAME_MAXLENGTH 16

struct vc_in_buffer {
	void * data;
	size_t filled;
	int state;
};

struct vc_in_queue {
	struct vc_in_buffer buffers[2];
	struct vc_in_buffer dummy;
	struct vc_in_buffer * pending;
	struct vc_in_buffer * ready;
};

struct vc_out_buffer {
	struct vb2_buffer vb;
	struct list_head list;
	size_t filled;
};

struct vc_out_queue {
	struct list_head active;
	int frame;
	//TO be completed later
};

struct vc_device_format{
	struct list_head active;
	struct v4l2_pix_format v4l2_fmt;
};

struct vc_device {
	dev_t dev_number;
	struct v4l2_device       v4l2_dev;
    struct video_device      vdev;
    
    //input buffer
    struct vc_in_queue       in_queue;
    spinlock_t               in_q_slock;

    //output buffer
    struct vb2_queue         vb_out_vidq;
    struct vc_out_queue      vc_out_vidq;
    spinlock_t               out_q_slock;

    char                     vc_fb_fname[FB_NAME_MAXLENGTH];
    struct proc_dir_entry*   vc_fb_procf;
    struct mutex             vc_mutex;

    //Submitter thread
    struct task_struct *     sub_thr_id;

    //Format descriptor
    size_t                   nr_fmts;
    struct v4l2_pix_format * v4l2_fmt[PIXFMTS_MAX];       
};

struct vc_device * create_vcdevice( size_t idx, struct vcmod_device_spec * dev_spec );
void destroy_vcdevice( struct vc_device * vcdev );

int submitter_thread( void * data );

#endif