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
};

/*  
 * V4L2 device descriptor structure
 */
static const struct video_device virtualcam_video_device_template = {
    .name = "virtualcam",
    .fops = &vircam_fops,
    .ioctl_ops = &vircam_ioctl_ops,
    .release = video_device_release_empty,
};


/*
 * Virtual camera device structure
 */
struct virtualcam_device {
    struct v4l2_device     v4l2_dev;
    struct video_device    vdev;
    struct mutex           mutex;
};

static struct virtualcam_device * device = NULL;

static int __init create_instance( int inst )
{
    struct virtualcam_device *dev;
    struct video_device      *vfd;
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
    
    mutex_init(&dev->mutex);

    vfd = &dev->vdev;
    *vfd = virtualcam_video_device_template;
    vfd->v4l2_dev = &dev->v4l2_dev; 
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

