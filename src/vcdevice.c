#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <media/videobuf2-core.h>
#include "vcdevice.h"
#include "vcioctl.h"
#include "vcvideobuf.h"
#include "vcfb.h"
#include "debug.h"

extern const char * vc_dev_name;
extern unsigned char allow_pix_conversion;
extern unsigned char allow_scaling;

struct __attribute__ ((__packed__)) rgb_struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

static const struct vc_device_format vcdev_supported_fmts[] = {
	{
		.name      = "RGB24 (LE)",
		.fourcc    = V4L2_PIX_FMT_RGB24,
		.bit_depth = 24,
	},
	{
		.name      = "YUV 4:2:2 (YUYV)",
		.fourcc    = V4L2_PIX_FMT_YUYV,
		.bit_depth = 16,
	},
};

static const struct v4l2_file_operations vcdev_fops = {
    .owner             = THIS_MODULE,
    .open              = v4l2_fh_open,
    .release           = vb2_fop_release,
    .read              = vb2_fop_read,
    .poll              = vb2_fop_poll,
    .unlocked_ioctl    = video_ioctl2,
    .mmap              = vb2_fop_mmap,
};

static const struct v4l2_ioctl_ops vcdev_ioctl_ops = {
    .vidioc_querycap            = vcdev_querycap,
    .vidioc_enum_input          = vcdev_enum_input,
    .vidioc_g_input             = vcdev_g_input,
    .vidioc_s_input             = vcdev_s_input,
    .vidioc_enum_fmt_vid_cap    = vcdev_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap       = vcdev_g_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap     = vcdev_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap       = vcdev_s_fmt_vid_cap,
    .vidioc_s_parm				= vcdev_g_parm,
    .vidioc_g_parm              = vcdev_s_parm,
    .vidioc_enum_frameintervals = vcdev_enum_frameintervals,
    .vidioc_enum_framesizes     = vcdev_enum_framesizes,
    .vidioc_reqbufs             = vb2_ioctl_reqbufs,
    .vidioc_create_bufs         = vb2_ioctl_create_bufs,
    .vidioc_prepare_buf         = vb2_ioctl_prepare_buf,
    .vidioc_querybuf            = vb2_ioctl_querybuf,
    .vidioc_qbuf                = vb2_ioctl_qbuf,
    .vidioc_dqbuf               = vb2_ioctl_dqbuf,
    .vidioc_streamon            = vb2_ioctl_streamon,
    .vidioc_streamoff           = vb2_ioctl_streamoff
};

static const struct video_device vc_video_device_template = {
    .fops      = &vcdev_fops,
    .ioctl_ops = &vcdev_ioctl_ops,
    .release   = video_device_release_empty,
};

static inline void rgb24_to_yuyv( void *dst, void *src )
{
	unsigned char * rgb  = ( unsigned char * ) src;
	unsigned char * yuyv = ( unsigned char * ) dst;
	yuyv[0] = (( 66 * rgb[0] + 129 * rgb[1] +  25 * rgb[2]) >> 8) + 16;
	yuyv[1] = ((-38 * rgb[0] -  74 * rgb[1] + 112 * rgb[2]) >> 8) + 128;
	yuyv[2] = (( 66 * rgb[3] + 129 * rgb[4] +  25 * rgb[5]) >> 8) + 16;
	yuyv[3] = ((112 * rgb[0] -  94 * rgb[1] -  18 * rgb[2]) >> 8) + 128;
	yuyv[0] = yuyv[0] > 240 ? 240 : yuyv[0]; yuyv[0] = yuyv[0] < 16 ? 16 : yuyv[0];
	yuyv[1] = yuyv[1] > 235 ? 235 : yuyv[1]; yuyv[1] = yuyv[1] < 16 ? 16 : yuyv[1]; 
	yuyv[2] = yuyv[2] > 240 ? 240 : yuyv[2]; yuyv[2] = yuyv[2] < 16 ? 16 : yuyv[2]; 
	yuyv[3] = yuyv[3] > 235 ? 235 : yuyv[3]; yuyv[3] = yuyv[3] < 16 ? 16 : yuyv[3]; 
}

static inline void yuyv_to_rgb24( void *dst, void *src )
{
	unsigned char * rgb  = ( unsigned char * ) dst;
	unsigned char * yuyv = ( unsigned char * ) src;
	int16_t r,g,b,c1,c2,d,e;
	c2 = yuyv[0] - 16;
	 d = yuyv[1] - 128;
	c1 = yuyv[2] - 16;
     e = yuyv[3] - 128;

	r = (298 * c1 + 409 * e ) >> 8;
	g = (298 * c1 - 100 * d - 208 * e ) >> 8;
	b = (298 * c1 + 516 * d ) >> 8;
	r = r > 255 ? 255 : r; r = r < 0 ? 0 : r;
	g = g > 255 ? 255 : g; g = g < 0 ? 0 : g;
	b = b > 255 ? 255 : b; b = b < 0 ? 0 : b;
	rgb[0] = (unsigned char) r;
	rgb[1] = (unsigned char) g;
	rgb[2] = (unsigned char) b;

	r = (298 * c2 + 409 * e ) >> 8;
	g = (298 * c2 - 100 * d - 208 * e ) >> 8;
	b = (298 * c2 + 516 * d ) >> 8;
	r = r > 255 ? 255 : r; r = r < 0 ? 0 : r;
	g = g > 255 ? 255 : g; g = g < 0 ? 0 : g;
	b = b > 255 ? 255 : b; b = b < 0 ? 0 : b;
	rgb[3] = (unsigned char) r;
	rgb[4] = (unsigned char) g;
	rgb[5] = (unsigned char) b;
}

static inline void yuyv_to_rgb24_one_pix( void *dst, void *src, unsigned char even)
{
	unsigned char * rgb  = ( unsigned char * ) dst;
	unsigned char * yuyv = ( unsigned char * ) src;
	int16_t r,g,b,c,d,e;
	 c = even ? yuyv[0] - 16 : yuyv[2] - 16;
	 d = yuyv[1] - 128;
     e = yuyv[3] - 128;

	r = (298 * c + 409 * e + 128) >> 8;
	g = (298 * c - 100 * d - 208 * e + 128) >> 8;
	b = (298 * c + 516 * d + 128) >> 8;
	r = r > 255 ? 255 : r; r = r < 0 ? 0 : r;
	g = g > 255 ? 255 : g; g = g < 0 ? 0 : g;
	b = b > 255 ? 255 : b; b = b < 0 ? 0 : b;
	rgb[0] = (unsigned char) r;
	rgb[1] = (unsigned char) g;
	rgb[2] = (unsigned char) b;
}

void submit_noinput_buffer(struct vc_out_buffer * buf, 
	struct vc_device * dev)
{
	void * vbuf_ptr;
	size_t size;
	size_t rowsize;
	size_t rows;
	int i,j,stripe_size;
	struct timeval ts;
	int32_t *yuyv_ptr;
	int32_t yuyv_tmp;
	unsigned char * yuyv_helper = (unsigned char *) &yuyv_tmp;

	vbuf_ptr = vb2_plane_vaddr(&buf->vb, 0);
	yuyv_ptr = vbuf_ptr;
	size = dev->output_format.sizeimage;
	rowsize = dev->output_format.bytesperline;
	rows = dev->output_format.height;

	stripe_size = ( rows / 255 );
	if( dev->output_format.pixelformat == V4L2_PIX_FMT_YUYV){

		
		yuyv_tmp = 0x80808080;

		for( i = 0; i < 255; i++ ){
			yuyv_helper[0] = (unsigned char)i;
			yuyv_helper[2] = (unsigned char)i;
			for( j = 0; j < ((rowsize*stripe_size) >> 2); j++){
				*yuyv_ptr = yuyv_tmp;
				yuyv_ptr++;
			}
		}	

		yuyv_tmp = 0x80ff80ff;
		while( (void *)yuyv_ptr < (void *)((void *)vbuf_ptr + size) ){
			*yuyv_ptr = yuyv_tmp;
			yuyv_ptr++;
		}

	}else{

		for( i = 0; i < 255; i++ ){
			memset( vbuf_ptr, i, rowsize * stripe_size );
			vbuf_ptr += rowsize *  stripe_size; 
		}

		if( rows % 255 ){
			memset( vbuf_ptr, 0xff , rowsize * ( rows % 255 ) );
		}
	}

	//memset( vbuf_ptr, 0x00, size );

	do_gettimeofday( &ts );
	buf->vb.v4l2_buf.timestamp = ts;
	vb2_buffer_done( &buf->vb, VB2_BUF_STATE_DONE );
	//PRINT_DEBUG("Skipped buffer submitted\n");
}

void copy_scale( unsigned char * dst, unsigned char * src, struct vc_device * dev)
{
	uint32_t dst_height = dev->output_format.height;
	uint32_t dst_width  = dev->output_format.width;
	uint32_t src_height = dev->input_format.height;
	uint32_t src_width  = dev->input_format.width;
	uint32_t ratio_height = ( (src_height << 16) / dst_height) + 1;
	uint32_t ratio_width;
	int i,j,tmp1,tmp2;
	
	if( dev->output_format.pixelformat == V4L2_PIX_FMT_YUYV ){
		
		uint32_t * yuyv_dst = (uint32_t *)dst;
		uint32_t * yuyv_src = (uint32_t *)src;
		dst_width >>= 1;
		src_width >>= 1;
		ratio_width  = ( (src_width  << 16) / dst_width ) + 1;
		for( i = 0; i < dst_height; i++ ){
			tmp1 = ((i*ratio_height) >> 16);
			for( j = 0; j < dst_width; j++ ){
				tmp2 = ((j*ratio_width) >> 16);
				yuyv_dst[ (i*dst_width) + j] = yuyv_src[ (tmp1*src_width) + tmp2];
			}
		}

	}else if(dev->output_format.pixelformat == V4L2_PIX_FMT_RGB24){

		struct rgb_struct * yuyv_dst = (struct rgb_struct *)dst;
		struct rgb_struct * yuyv_src = (struct rgb_struct *)src;
		ratio_width  = ( (src_width  << 16) / dst_width ) + 1;
		for( i = 0; i < dst_height; i++ ){
			tmp1 = ((i*ratio_height) >> 16);
			for( j = 0; j < dst_width; j++ ){
				tmp2 = ((j*ratio_width) >> 16);
				yuyv_dst[ (i*dst_width) + j] = yuyv_src[ (tmp1*src_width) + tmp2];
			}
		}

	}
}

void copy_scale_rgb24_to_yuyv( unsigned char * dst, unsigned char * src, struct vc_device * dev )
{
	uint32_t dst_height = dev->output_format.height;
	uint32_t dst_width  = dev->output_format.width;
	uint32_t src_height = dev->input_format.height;
	uint32_t src_width  = dev->input_format.width;
	uint32_t ratio_height = ( (src_height << 16) / dst_height) + 1;
	uint32_t ratio_width;
	int i,j,tmp1,tmp2;

	uint32_t * yuyv_dst = (uint32_t *)dst;
	struct rgb_struct * rgb_src = (struct rgb_struct *)src;
	dst_width >>= 1;
	src_width >>= 1;
	ratio_width  = ( (src_width  << 16) / dst_width ) + 1;
	for( i = 0; i < dst_height; i++ ){
		tmp1 = ((i*ratio_height) >> 16);
		for( j = 0; j < dst_width; j++ ){
			tmp2 = ((j*ratio_width) >> 16);
			rgb24_to_yuyv(yuyv_dst, &rgb_src[tmp1*(src_width << 1) + (tmp2 << 1)]);
			yuyv_dst++;
		}
	}
}

void copy_scale_yuyv_to_rgb24( unsigned char * dst, unsigned char * src, struct vc_device * dev)
{
	uint32_t dst_height = dev->output_format.height;
	uint32_t dst_width  = dev->output_format.width;
	uint32_t src_height = dev->input_format.height;
	uint32_t src_width  = dev->input_format.width;
	uint32_t ratio_height = ( (src_height << 16) / dst_height) + 1;
	uint32_t ratio_width  = ( (src_width  << 16) / dst_width ) + 1;
	int i,j,tmp1,tmp2;

	struct rgb_struct * rgb_dst = (struct rgb_struct *)dst;
	int32_t * yuyv_src = (int32_t *) src;
	for( i = 0; i < dst_height; i++ ){
		tmp1 = ((i*ratio_height) >> 16);
		for( j = 0; j < dst_width; j++ ){
			tmp2 = ((j*ratio_width) >> 16);
			yuyv_to_rgb24_one_pix(rgb_dst, 
				&yuyv_src[tmp1*(src_width >> 1) + (tmp2 >> 1)], tmp2 & 0x01);
			rgb_dst++;
		}
	}
}

static void convert_rgb24_buf_to_yuyv( unsigned char * dst, unsigned char * src, size_t pixel_count )
{
	int i;
	pixel_count >>= 1;
	for( i = 0; i < pixel_count; i++ ){
		rgb24_to_yuyv(dst,src);
		dst += 4;
		src += 6;
	}
}

static void convert_yuyv_buf_to_rgb24( unsigned char * dst, unsigned char * src, size_t pixel_count )
{
	int i;
	pixel_count >>= 1;
	for( i = 0; i < pixel_count; i++ ){
		yuyv_to_rgb24(dst,src);
		dst+=6;
		src+=4;
	}
}

void submit_copy_buffer( struct vc_out_buffer * out_buf,
	struct vc_in_buffer * in_buf, struct vc_device * dev )
{
	void * in_vbuf_ptr;
	void * out_vbuf_ptr;
	struct timeval ts;

	in_vbuf_ptr = in_buf->data;
	if(!in_vbuf_ptr){
		PRINT_ERROR("Input buffer is NULL in ready state\n");
		return;
	}
	out_vbuf_ptr = vb2_plane_vaddr(&out_buf->vb, 0);
	if(!out_vbuf_ptr){
		PRINT_ERROR("Output buffer is NULL\n");
		return;
	}

	if( dev->output_format.pixelformat == dev->input_format.pixelformat ){
		PRINT_DEBUG("Same pixel format\n");
		PRINT_DEBUG("%d,%d -> %d,%d\n",dev->output_format.width,dev->output_format.height,
			dev->input_format.width, dev->input_format.height);
	   	if(dev->output_format.width == dev->input_format.width &&
			dev->output_format.height == dev->input_format.height ){
	   			PRINT_DEBUG("No scaling\n");
				memcpy( out_vbuf_ptr, in_vbuf_ptr, in_buf->filled );

			}else{
				PRINT_DEBUG("Scaling\n");
				copy_scale( out_vbuf_ptr, in_vbuf_ptr, dev );

			}	

	}else {
		if(dev->output_format.width == dev->input_format.width &&
			dev->output_format.height == dev->input_format.height ){

				int pixel_count = dev->input_format.height * dev->input_format.width;
				if( dev->input_format.pixelformat == V4L2_PIX_FMT_YUYV ){
					PRINT_DEBUG("YUYV->RGB24 no scale\n");
					convert_yuyv_buf_to_rgb24( out_vbuf_ptr, in_vbuf_ptr, pixel_count );
				}else{
					PRINT_DEBUG("RGB24->YUYV no scale\n");
					convert_rgb24_buf_to_yuyv( out_vbuf_ptr, in_vbuf_ptr, pixel_count );
				}

		}else{

				if( dev->output_format.pixelformat == V4L2_PIX_FMT_YUYV ){
					PRINT_DEBUG("RGB24->YUYV scale\n");
					copy_scale_rgb24_to_yuyv( out_vbuf_ptr, in_vbuf_ptr, dev );
				} else if( dev->output_format.pixelformat == V4L2_PIX_FMT_RGB24 ){
					PRINT_DEBUG("RGB24->YUYV scale\n");
					copy_scale_yuyv_to_rgb24( out_vbuf_ptr, in_vbuf_ptr, dev );
				}

		}	
	}
	
	do_gettimeofday( &ts );
	out_buf->vb.v4l2_buf.timestamp = ts;
	vb2_buffer_done( &out_buf->vb, VB2_BUF_STATE_DONE );
	//PRINT_DEBUG("Copy buffer submitted, %dB\n", (int)in_buf->filled);
}

int submitter_thread( void * data )
{
	struct vc_device * dev;
	struct vc_out_queue * q;
	struct vc_in_queue * in_q;
	struct vc_out_buffer * buf;
	struct vc_in_buffer * in_buf;
	unsigned long flags;
	int computation_time_jiff,computation_time_ms;
	int timeout_ms;
	int timeout;
	int ret = 0;
	flags = 0;
	dev = ( struct vc_device * )  data;
	q = &dev->vc_out_vidq;
	in_q = &dev->in_queue;

	PRINT_DEBUG("Submitter thread started");

	while(!kthread_should_stop()){
		//Do something and sleep
		computation_time_jiff = jiffies;
		spin_lock_irqsave( &dev->out_q_slock, flags );
		if ( list_empty( &q->active ) ){
			PRINT_DEBUG("Buffer queue is empty\n");
			spin_unlock_irqrestore( &dev->out_q_slock, flags );
			goto have_a_nap;
		}
		buf = list_entry( q->active.next, struct vc_out_buffer, list );
		list_del( &buf->list );
		spin_unlock_irqrestore( &dev->out_q_slock, flags );

		if( !dev->fb_isopen ){
			submit_noinput_buffer( buf, dev );
		}else{
			spin_lock_irqsave( &dev->in_q_slock, flags );
			in_buf = in_q->ready;
			if(!in_buf){
				PRINT_ERROR("Ready buffer in input queue has NULL pointer\n");
				goto unlock_and_continue;
			}
			submit_copy_buffer( buf, in_buf, dev );
			unlock_and_continue:
			spin_unlock_irqrestore( &dev->in_q_slock, flags);
		}

		have_a_nap:
			if( !dev->output_fps.denominator ){
				dev->output_fps.numerator = 1001;
				dev->output_fps.denominator = 30000;
			}
			timeout_ms = dev->output_fps.denominator / dev->output_fps.numerator;
			if( !timeout_ms ){
				dev->output_fps.numerator = 1001;
				dev->output_fps.denominator = 60000;
				timeout_ms = dev->output_fps.denominator / dev->output_fps.numerator;
			}
			
			//Compute timeout, modify FPS
			computation_time_jiff = jiffies - computation_time_jiff;
			timeout = msecs_to_jiffies( timeout_ms );
			if(computation_time_jiff > timeout ){
				computation_time_ms = msecs_to_jiffies( computation_time_jiff );
				dev->output_fps.numerator   = 1001;
				dev->output_fps.denominator = 1000 * computation_time_ms;
			}else if(timeout > computation_time_jiff){
				schedule_timeout_interruptible(timeout - computation_time_jiff);
			}
	}

	PRINT_DEBUG("Submitter thread finished");
	return ret;
}

int check_supported_pixfmt( struct vc_device * dev, unsigned int fourcc )
{
    int i;
    for( i = 0; i < dev->nr_fmts; i++ ){
        if( dev->out_fmts[i].fourcc == fourcc)
            break;
    }

    if( i == dev->nr_fmts ){
        return 0;
    }
    return 1;
}

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
			fmt->colorspace = V4L2_COLORSPACE_SRGB;
			break;
		case VCMOD_PIXFMT_YUYV:
			fmt->pixelformat = V4L2_PIX_FMT_YUYV;
			fmt->bytesperline = (fmt->width) << 1;
			fmt->colorspace = V4L2_COLORSPACE_SMPTE170M;
			break;
		default:
			fmt->pixelformat = V4L2_PIX_FMT_RGB24;
			fmt->bytesperline = (fmt->width*3);
			fmt->colorspace = V4L2_COLORSPACE_SRGB;
			break;
	}

	fmt->field  = V4L2_FIELD_INTERLACED;
    fmt->sizeimage = fmt->height * fmt->bytesperline;
}

struct vc_device * create_vcdevice(size_t idx, struct vcmod_device_spec * dev_spec )
{
	struct vc_device * vcdev;
	struct video_device * vdev;
	struct proc_dir_entry * pde;
	int i,ret = 0;

	PRINT_DEBUG("creating device\n");

	vcdev = (struct vc_device *) 
		kmalloc( sizeof( struct vc_device), GFP_KERNEL );
	//PRINT_DEBUG("Allocated at %0X\n",(unsigned int) vcdev);
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
	spin_lock_init( &vcdev->in_q_slock );
	spin_lock_init( &vcdev->in_fh_slock );

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
		"vcfb%d", MINOR(vcdev->vdev.dev.devt) );

	pde = init_framebuffer( (const char *) vcdev->vc_fb_fname , vcdev );
	if ( !pde ){
		goto framebuffer_failure;
	}
	vcdev->vc_fb_procf = pde;
	vcdev->fb_isopen   = 0;

	PRINT_DEBUG("Creating %dx%d\n",dev_spec->width,dev_spec->height);

	//Setup conversion capabilities

	vcdev->conv_res_on = allow_scaling;	
	vcdev->conv_pixfmt_on = allow_pix_conversion;
	PRINT_DEBUG("%d %d\n",vcdev->conv_res_on,vcdev->conv_pixfmt_on);

	//Alloc and set initial format
	if( vcdev->conv_pixfmt_on ){
		for( i = 0; i < ARRAY_SIZE( vcdev_supported_fmts ); i++ ){
			vcdev->out_fmts[i] = vcdev_supported_fmts[i];
		}
		vcdev->nr_fmts = i;
	}else{
		if( dev_spec && dev_spec->pix_fmt == VCMOD_PIXFMT_YUYV ){
			vcdev->out_fmts[0] = vcdev_supported_fmts[1];
		}else{
			vcdev->out_fmts[0] = vcdev_supported_fmts[0];
		}
		vcdev->nr_fmts = 1;
	}
	//vcdev->out_fmts[0] = vcdev_supported_fmts[0];
	
	fill_v4l2pixfmt( &vcdev->output_format, dev_spec );
	fill_v4l2pixfmt( &vcdev->input_format, dev_spec );

	vcdev->sub_thr_id = NULL;

	//Init input 
	ret = vc_in_queue_setup( &vcdev->in_queue ,
		vcdev->input_format.sizeimage );
	if( ret ){
		PRINT_ERROR("Failed to initialize input buffer\n");
		goto input_buffer_failure;
	}

	vcdev->output_fps.numerator = 1001;
	vcdev->output_fps.denominator = 30000;
	
	return vcdev;
	input_buffer_failure:
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
	PRINT_DEBUG("destroying vcdevice\n");
	if( !vcdev ){
		return;
	}
	if( vcdev->sub_thr_id ){
		kthread_stop(vcdev->sub_thr_id);
	}
	vc_in_queue_destroy( &vcdev->in_queue );
	destroy_framebuffer( vcdev->vc_fb_fname );
	mutex_destroy( &vcdev->vc_mutex );
	video_unregister_device( &vcdev->vdev );
	v4l2_device_unregister( &vcdev->v4l2_dev );

	kfree( vcdev );
}