#include <linux/module.h>
#include <linux/spinlock.h>
#include <media/videobuf2-core.h>
#include "vcdevice.h"
#include "vcioctl.h"
#include "vcvideobuf.h"
#include "vcfb.h"
#include "debug.h"

extern const char * vc_dev_name;

static const struct v4l2_file_operations vcdev_fops = {
    .owner             = THIS_MODULE,
    .open              = v4l2_fh_open,
    .unlocked_ioctl    = video_ioctl2,
    .mmap              = vb2_fop_mmap,
};

static const struct v4l2_ioctl_ops vcdev_ioctl_ops = {
    .vidioc_querycap         = vcdev_querycap,
    .vidioc_enum_input       = vcdev_enum_input,
    .vidioc_g_input          = vcdev_g_input,
    .vidioc_s_input          = vcdev_s_input,
    .vidioc_enum_fmt_vid_cap = vcdev_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap    = vcdev_g_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap    = vcdev_s_fmt_vid_cap,
    .vidioc_reqbufs          = vb2_ioctl_reqbufs,
    .vidioc_create_bufs      = vb2_ioctl_create_bufs,
    .vidioc_prepare_buf      = vb2_ioctl_prepare_buf,
    .vidioc_querybuf         = vb2_ioctl_querybuf,
    .vidioc_qbuf             = vb2_ioctl_qbuf,
    .vidioc_dqbuf            = vb2_ioctl_dqbuf,
    .vidioc_streamon         = vb2_ioctl_streamon,
    .vidioc_streamoff        = vb2_ioctl_streamoff
};

static const struct video_device vc_video_device_template = {
    .fops      = &vcdev_fops,
    .ioctl_ops = &vcdev_ioctl_ops,
    .release   = video_device_release_empty,
};

static void fill_v4l2pixfmt( struct v4l2_pix_format * fmt, 
	struct vcmod_device_spec * dev_spec )
{
	if( !fmt || !dev_spec ){
		return;
	}

	memset( fmt, 0x00, sizeof( struct v4l2_pix_format ) );
	fmt->width = dev_spec->width;
	fmt->height = dev_spec->height;
	PRINT_DEBUG("Filling %dx%d\n",dev_spec->width,dev_spec->height);

	switch( dev_spec->pix_fmt ){
		case VCMOD_PIXFMT_RGB24:
			fmt->pixelformat = V4L2_PIX_FMT_RGB24;
			fmt->bytesperline = (fmt->width*3);
			break;
		case VCMOD_PIXFMT_YUV:
			fmt->pixelformat = V4L2_PIX_FMT_YUYV;
			fmt->bytesperline = (fmt->width) << 1;
			break;
		default:
			fmt->pixelformat = V4L2_PIX_FMT_RGB24;
			fmt->bytesperline = (fmt->width*3);
			break;
	}

	fmt->field  = V4L2_FIELD_INTERLACED;
    fmt->sizeimage = fmt->height * fmt->bytesperline;
    fmt->colorspace = V4L2_COLORSPACE_SRGB;
}

struct vc_device * create_vcdevice(size_t idx, struct vcmod_device_spec * dev_spec )
{
	struct vc_device * vcdev;
	struct video_device * vdev;
	struct proc_dir_entry * pde;
	struct v4l2_pix_format *fmt;
	int ret = 0;

	PRINT_DEBUG("creating device\n");

	vcdev = (struct vc_device *) 
		kmalloc( sizeof( struct vc_device), GFP_KERNEL );
	PRINT_DEBUG("Allocated at %0X\n",(unsigned int) vcdev);
	if( !vcdev ){
		goto vcdev_alloc_failure;
	}

	memset( vcdev, 0x00, sizeof(struct vc_device) );

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

	spin_lock_init( &vcdev->out_q_slock );
	INIT_LIST_HEAD( &vcdev->vc_out_vidq.active );

	vdev = &vcdev->vdev;
	*vdev = vc_video_device_template;
	vdev->v4l2_dev = &vcdev->v4l2_dev; 
    vdev->queue = &vcdev->vb_out_vidq;
    vdev->lock = &vcdev->vc_mutex;
	snprintf(vdev->name, sizeof(vdev->name),
		"%s-%d",vc_dev_name,(int)idx);
	video_set_drvdata(vdev, vcdev);

	ret = video_register_device( vdev, VFL_TYPE_GRABBER, -1 );
	if(ret < 0){
		PRINT_ERROR("video_register_device failure\n");
		goto video_regdev_failure;
	}
	PRINT_DEBUG("video_register_device successfull\n");

	//Initialize framebuffer device
	snprintf(vcdev->vc_fb_fname, sizeof(vcdev->vc_fb_fname),
		"%s_%d_fb",vc_dev_name, (int)idx );

	pde = init_framebuffer( (const char *) vcdev->vc_fb_fname , vcdev );
	if ( !pde ){
		goto framebuffer_failure;
	}
	vcdev->vc_fb_procf = pde;

	PRINT_DEBUG("Creating %dx%d\n",dev_spec->width,dev_spec->height);

	//Alloc and set initial format
	vcdev->v4l2_fmt[0] = (struct v4l2_pix_format *) 
		kmalloc( sizeof(struct v4l2_pix_format) , GFP_KERNEL );
	if( !vcdev->v4l2_fmt[0] ){
		PRINT_ERROR( "Failed to allocate pixel format descriptor\n");
		goto fmt_alloc_failure;
	}
	fmt = vcdev->v4l2_fmt[0];
	
	fill_v4l2pixfmt( fmt, dev_spec );

	vcdev->nr_fmts = 1;

	return vcdev;
	fmt_alloc_failure:
	framebuffer_failure:
		//Probably nothing to do in here
		destroy_framebuffer( vcdev->vc_fb_fname );
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
	int i;
	PRINT_DEBUG("destroying vcdevice\n");
	if( !vcdev ){
		return;
	}
	destroy_framebuffer( vcdev->vc_fb_fname );
	mutex_destroy( &vcdev->vc_mutex );
	video_unregister_device( &vcdev->vdev );
	v4l2_device_unregister( &vcdev->v4l2_dev );

	for( i = 0; i < vcdev->nr_fmts; i++ ){
		kfree( vcdev->v4l2_fmt[i] );
	}

	kfree( vcdev );
}