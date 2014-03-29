#include <linux/module.h>
#include "vcdevice.h"
#include "vcioctl.h"
#include "vcvideobuf.h"
#include "debug.h"

extern const char * vc_dev_name;

static const struct v4l2_file_operations vcdev_fops = {
    .owner             = THIS_MODULE,
    .open              = v4l2_fh_open,
    .unlocked_ioctl    = video_ioctl2,
    //.mmap              = vb2_fop_mmap,
};

static const struct v4l2_ioctl_ops vcdev_ioctl_ops = {
    .vidioc_querycap         = vcdev_querycap,
    .vidioc_enum_input       = vcdev_enum_input,
    .vidioc_g_input          = vcdev_g_input,
    .vidioc_s_input          = vcdev_s_input,
    .vidioc_enum_fmt_vid_cap = vcdev_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap    = vcdev_g_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap    = vcdev_s_fmt_vid_cap,
//    .vidioc_reqbufs          = vb2_ioctl_reqbufs,
//    .vidioc_create_bufs      = vb2_ioctl_create_bufs,
//    .vidioc_prepare_buf      = vb2_ioctl_prepare_buf,
//    .vidioc_querybuf         = vb2_ioctl_querybuf,
//    .vidioc_qbuf             = vb2_ioctl_qbuf,
//    .vidioc_dqbuf            = vb2_ioctl_dqbuf,
//    .vidioc_streamon         = vb2_ioctl_streamon,
//    .vidioc_streamoff        = vb2_ioctl_streamoff
};

static const struct video_device vc_video_device_template = {
    .fops      = &vcdev_fops,
    .ioctl_ops = &vcdev_ioctl_ops,
    .release   = video_device_release_empty,
};

struct vc_device * create_vcdevice(size_t idx)
{
	struct vc_device * vcdev;
	struct video_device * vdev;
	int ret = 0;

	PRINT_DEBUG("creating device\n");

	vcdev = (struct vc_device *) 
		kmalloc( sizeof( struct vc_device), GFP_KERNEL );
	if( !vcdev ){
		goto vcdev_alloc_failure;
	}

	//Assign name of v4l2 device 
	snprintf(vcdev->v4l2_dev.name, sizeof(vcdev->v4l2_dev.name),
		"%s-%d",vc_dev_name,(int)idx);
	//Try to register
	ret = v4l2_device_register(NULL,&vcdev->v4l2_dev);
	if(ret){
		PRINT_ERROR("v4l2 registration failure\n");
		goto v4l2_registration_failure;
	}
	PRINT_DEBUG("v4l2 device \"%s\" registration successful\n",
		vcdev->v4l2_dev.name);

	//Initialize buffer queue and device structures
	mutex_init( &vcdev->vc_mutex );

	//Try to initialize output buffer
	ret = vc_out_videobuf2_setup( vcdev );
	if( ret ){
		PRINT_ERROR(" failed to initialize output videobuffer\n");
		goto vb2_out_init_failed;
	}

	vdev = &vcdev->vdev;
	*vdev = vc_video_device_template;
	snprintf(vdev->name, sizeof(vdev->name),
		"%s-%d",vc_dev_name,(int)idx);
	video_set_drvdata(vdev, vcdev);

	ret = video_register_device( vdev, VFL_TYPE_GRABBER, -1 );
	if(ret < 0){
		PRINT_ERROR("video_register_device failure\n");
		goto video_regdev_failure;
	}
	PRINT_DEBUG("video_register_device successfull\n");

	return vcdev;
	video_regdev_failure:
		//TODO vb2 deinit
	vb2_out_init_failed:
		v4l2_device_unregister( &vcdev->v4l2_dev );
	v4l2_registration_failure:
		kfree(vcdev);
	vcdev_alloc_failure:
		return NULL;
}

void destroy_vcdevice( struct vc_device * vcdev )
{
	PRINT_DEBUG("destroying vcdevice\n");
	if( !vcdev ){
		return;
	}
	mutex_destroy( &vcdev->vc_mutex );
	video_unregister_device( &vcdev->vdev );
	v4l2_device_unregister( &vcdev->v4l2_dev );
	kfree( vcdev );
}