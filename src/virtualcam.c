#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-vmalloc.h>

#define VIRTUAL_CAMERA_MODULE_NAME "virtualcam"

MODULE_AUTHOR("Jan Sebechlebsky");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual V4L2 device driver");

static int video_nr = -1;

/*
 * V4L2 file operations pointer structure
 */
static const struct v4l2_file_operations vircam_fops = {
    .owner             = THIS_MODULE,
    .open              = v4l2_fh_open,
    .unlocked_ioctl    = video_ioctl2,
    .mmap              = vb2_fop_mmap,
};

/*
 * V4L2 Query capabilities ioctl command callback
 */
static int vidioc_querycap( struct file * file, void * priv,
                            struct v4l2_capability * cap)
{
    printk( KERN_INFO "Virtualcam: querycap called\n");
    strcpy(cap->driver,"virtualcam");
    strcpy(cap->card,"virtualcam");
    cap->device_caps  = V4L2_CAP_VIDEO_CAPTURE |
                        V4L2_CAP_STREAMING     |
                        V4L2_CAP_READWRITE;
    cap->capabilities = cap->device_caps |
                        V4L2_CAP_DEVICE_CAPS;
    return 0;
}

/*
 * V4L2 Enum input sources ioctl command callback
 */
static int vidioc_enum_input( struct file * file, void * priv,
                              struct v4l2_input * inp)
{
    printk( KERN_INFO "Virtualcam: enum_input with id=%d called\n",inp->index);
    
    if(inp->index >= 1 )
        return -EINVAL;
    
    inp->type = V4L2_INPUT_TYPE_CAMERA;
    sprintf(inp->name,"VirtualCamera %u", inp->index );
    return 0;
}


/*
 * V4L2 Get active input index ioctl callback
 */ 
static int vidioc_g_input( struct file * file, void * priv,
                           unsigned int * i )
{
   printk( KERN_INFO "Virtualcam: get_input ioctl called\n");

   *i = 0;
   return 0; 
}

/*
 * V4L2 Set active input ioctl callback
 */ 
static int vidioc_s_input( struct file * file, void * priv,
                           unsigned int i )
{
    if ( i>= 1 )
        return -EINVAL;
    return 0;
}

/*
 * V4L2 Enum format ioctl command callback
 */ 
static int vidioc_enum_fmt_vid_cap( struct file * file, void * priv,
                                    struct v4l2_fmtdesc * f)
{
    printk( KERN_INFO "Virtualcam: enum_fmt with id=%d called\n",f->index);
    
    if( f->index >= 1 )
        return -EINVAL;

    strcpy(f->description,"RGB24 (LE)");
    f->pixelformat = V4L2_PIX_FMT_RGB24;
    return 0;
}

/*
 * V4L2 Get format ioctl command callback
 */ 
static int vidioc_g_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    printk( KERN_INFO "Virtualcam: g_fmt called\n");

    f->fmt.pix.width  = 640;
    f->fmt.pix.height = 480;
    f->fmt.pix.field  = V4L2_FIELD_INTERLACED;
    f->fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    f->fmt.pix.bytesperline = (640*24) >> 3;
    f->fmt.pix.sizeimage = 480*f->fmt.pix.bytesperline;
    f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    
    return 0;
}

/*
 * V4L2 Set format ioctl command callback
 */ 
static int vidioc_s_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    int ret;
    printk( KERN_INFO "Virtualcam: s_fmt called\n");

    ret = vidioc_g_fmt_vid_cap( file, priv, f);
    if ( ret < 0 )
         return ret;
    return 0;
}

/* 
 * V4L2 ioctl operations pointer structure
 */
static const struct v4l2_ioctl_ops vircam_ioctl_ops = {
    .vidioc_querycap         = vidioc_querycap,
    .vidioc_enum_input       = vidioc_enum_input,
    .vidioc_g_input          = vidioc_g_input,
    .vidioc_s_input          = vidioc_s_input,
    .vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap    = vidioc_g_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap    = vidioc_s_fmt_vid_cap,
    .vidioc_reqbufs          = vb2_ioctl_reqbufs,
    .vidioc_create_bufs      = vb2_ioctl_create_bufs,
    .vidioc_prepare_buf      = vb2_ioctl_prepare_buf,
    .vidioc_querybuf         = vb2_ioctl_querybuf,
    .vidioc_qbuf             = vb2_ioctl_qbuf,
    .vidioc_dqbuf            = vb2_ioctl_dqbuf,
    .vidioc_streamon         = vb2_ioctl_streamon,
    .vidioc_streamoff        = vb2_ioctl_streamoff
};

/*  
 * V4L2 device descriptor structure
 */
static const struct video_device virtualcam_video_device_template = {
    .name = "virtualcam",
    .fops      = &vircam_fops,
    .ioctl_ops = &vircam_ioctl_ops,
    .release   = video_device_release_empty,
};


/*
 * Virtual camera device structure
 */
struct virtualcam_device {
    struct v4l2_device     v4l2_dev;
    struct video_device    vdev;
    struct mutex           mutex;
    struct vb2_queue       vb_vidq;
    spinlock_t             slock;
};


/*----------------------------------------------------
 * Video buffer 
 *---------------------------------------------------*/

struct virtualcam_buffer {
    struct vb2_buffer vb;
    struct list_head  list;
};

static int queue_setup( struct vb2_queue * vq,
                         const struct v4l2_format * fmt,
                         unsigned int *nbuffers, unsigned int *nplanes,
                         unsigned int sizes[], void * alloc_ctxs[])
{
    unsigned long size;    
    //struct virtualcam_device * dev = vb2_get_drv_priv(vq);
    printk( KERN_INFO "queue_setup called\n" );

    if( fmt )
        size = fmt->fmt.pix.sizeimage;
    else
        size = 640*480*3;
   
    if( 0 == *nbuffers )
        *nbuffers = 32;
 
    *nplanes = 1;
    
    sizes[0] = size;
    printk( KERN_INFO "queue_setup completed\n" ); 
    return 0;
}

static int buffer_prepare( struct vb2_buffer * vb )
{
    unsigned long size;
    printk( KERN_INFO "buffer_prepare called\n");
    
    size = 480*640*3;
    if( vb2_plane_size(vb,0) < size ){
        printk( KERN_ERR "data will not fit into buffer");
        return -EINVAL;
    }

    vb2_set_plane_payload(vb,0,size);
    
    return 0;
}

static void buffer_queue( struct vb2_buffer * vb )
{
    //TODO
    //struct virtualcam_device * dev;
    

    printk( KERN_INFO "buffer_queue called\n");
    
    //v4l2_get_timestamp(&vb->v4l2_buf.timestamp);
    vb2_buffer_done(vb, VB2_BUF_STATE_DONE );
}

static int start_streaming( struct vb2_queue * q, unsigned int count )
{
    printk( KERN_INFO "start streaming vb called\n");
    //TODO
    return 0;
}

static int stop_streaming( struct vb2_queue * q )
{
    printk( KERN_INFO "stop streaming vb called\n");
    //TODO
    return 0;
}

static void virtualcam_lock( struct vb2_queue * vq )
{
    struct virtualcam_device * dev = vb2_get_drv_priv(vq);
    mutex_lock(&dev->mutex);
}

static void virtualcam_unlock( struct vb2_queue * vq )
{
    struct virtualcam_device * dev = vb2_get_drv_priv(vq);
    mutex_unlock(&dev->mutex);
}

static const struct vb2_ops virtualcam_vb_ops = {
    .queue_setup     = queue_setup,
    .buf_prepare     = buffer_prepare,
    .buf_queue       = buffer_queue,
    .start_streaming = start_streaming,
    .stop_streaming  = stop_streaming,
    .wait_prepare    = virtualcam_unlock,
    .wait_finish     = virtualcam_lock,
};

static struct virtualcam_device * device = NULL;

static int __init create_instance( int inst )
{
    struct virtualcam_device *dev;
    struct video_device      *vfd;
    struct vb2_queue         *q;
    int ret;
    
    printk(KERN_INFO "create_instance(%d) called\n",inst);
    
    dev = kzalloc( sizeof(*dev), GFP_KERNEL );
    if(!dev)
        return -ENOMEM;

    snprintf( dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name),
              "%s-%03d", VIRTUAL_CAMERA_MODULE_NAME, inst);
    ret = v4l2_device_register( NULL, &dev->v4l2_dev );
    if (ret)
        goto free_dev;

    spin_lock_init(&dev->slock);   
 
    //Initialize vb2 buffer queue
    q = &dev->vb_vidq;
    q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
    q->drv_priv = dev;
    q->buf_struct_size = sizeof(struct virtualcam_buffer);
    q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    q->ops = &virtualcam_vb_ops;
    q->mem_ops = &vb2_vmalloc_memops;

    ret = vb2_queue_init(q);
    if (ret)
        goto unreg_dev;

    mutex_init(&dev->mutex);

    vfd = &dev->vdev;
    *vfd = virtualcam_video_device_template;
    vfd->v4l2_dev = &dev->v4l2_dev; 
    vfd->queue = q;
    vfd->lock = &dev->mutex;
    video_set_drvdata( vfd, dev);
    
    ret = video_register_device(vfd, VFL_TYPE_GRABBER, video_nr );
    if (ret < 0)
    	goto unreg_dev;
    
    printk(KERN_INFO "virtualcamera registered as %s\n",
                     video_device_node_name(vfd));
  
    device = dev;
    return 0;
    
  unreg_dev:
        v4l2_device_unregister(&dev->v4l2_dev);
  free_dev:
        kfree(dev);
        return ret;
    return ret;
}

/*
 * Module initialization callback
 */ 
static int __init virtualcam_init( void )
{
    int ret;
    printk(KERN_INFO "virtualcam module loaded\n");
    ret = create_instance(0);
    printk(KERN_INFO "create_instance returned %d\n",ret);
    return ret;
}

/*
 * Module cleanup callback
 */ 
static void __exit virtualcam_exit( void )
{
    if(device){
        printk(KERN_INFO "Unregistering %s\n",
               video_device_node_name(&device->vdev));
        video_unregister_device(&device->vdev);
        v4l2_device_unregister(&device->v4l2_dev);
        kfree(device);
        device = NULL;
    }
    printk(KERN_INFO "virtualcam module unloaded\n");
}

module_init( virtualcam_init );
module_exit( virtualcam_exit);

