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
};

/*
 * V4L2 Query capabilities ioctl command callback
 */
static int vidioc_querycap( struct file * file, void * priv,
                            struct v4l2_capability * cap)
{
    strcpy(cap->driver,"virtualcam");
    strcpy(cap->card,"virtualcam");
    cap->capabilities = V4L2_CAP_VIDEO_CAPTURE;

    return 0;
}

/* 
 * V4L2 ioctl operations pointer structure
 */
static const struct v4l2_ioctl_ops vircam_ioctl_ops = {
    .vidioc_querycap = vidioc_querycap,
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

