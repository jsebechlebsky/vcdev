#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include "vcdevice.h"
#include "vccontrol.h"
#include "vcvideobuf.h"
#include "debug.h"
#include "vcmod_api.h"

static int ctrl_open(struct inode * inode, struct file * file);
static int ctrl_release(struct inode * inode, struct file * file);
static ssize_t ctrl_read( struct file * file, char __user * buffer, size_t length, loff_t * offset );
static ssize_t ctrl_write( struct file * file, const char __user * buffer, size_t length, loff_t * offset );
static long ctrl_ioctl( struct file * file, unsigned int ioctl_cmd, unsigned long ioctl_param );


struct file_operations ctrl_fops = {
	.owner   = THIS_MODULE,
	.read    = ctrl_read,
	.write   = ctrl_write,
	.open    = ctrl_open,
	.release = ctrl_release,
	.unlocked_ioctl   = ctrl_ioctl,
};

struct vcmod_device_spec default_vcdev_spec = {
	.width   = 640,
	.height  = 480,
	.pix_fmt = VCMOD_PIXFMT_RGB24,
};

extern int devices_max;

struct control_device * ctrldev = NULL;

static int ctrl_open( struct inode * inode, struct file * file)
{
	PRINT_DEBUG("open\n");
	return 0;
}

static int ctrl_release( struct inode * inode, struct file * file)
{
	PRINT_DEBUG("release\n");
	return 0;
}

static ssize_t ctrl_read( struct file *file, char __user * buffer,
							 size_t length,	loff_t * offset )
{
	int len;
	static const char * str = "Virtual V4L2 device driver\n";
	PRINT_DEBUG("read %ld %dB\n", (long) buffer, (int) length );
	len = strlen(str);
	if( len < length )
		len = length;
	copy_to_user(buffer,str,len);
	return len;
}

static ssize_t ctrl_write( struct file * file, const char __user * buffer, 
							size_t length, loff_t * offset )
{
	PRINT_DEBUG( "write %ld %dB\n", (long) buffer, (int) length );
	return length;
}

static int ctrl_ioctl_get_device( struct vcmod_device_spec * dev_spec )
{
	struct vc_device * dev;
	if( ctrldev->vc_device_count <= dev_spec->idx ){
		return -EINVAL;
	}

	dev = ctrldev->vc_devices[dev_spec->idx];
	dev_spec->width = dev->input_format.width;
	dev_spec->height = dev->input_format.height;
	if( dev->input_format.pixelformat == V4L2_PIX_FMT_YUYV ){
		dev_spec->pix_fmt = VCMOD_PIXFMT_YUYV;
	}else {
		dev_spec->pix_fmt = VCMOD_PIXFMT_RGB24;
	}
	strncpy( (char *)&dev_spec->fb_node, (const char *)&dev->vc_fb_fname, sizeof(dev_spec->fb_node) );
	snprintf( (char *)&dev_spec->video_node, sizeof(dev_spec->video_node),
		"/dev/video%d",dev->vdev.minor);
	return 0;
}

static int ctrl_ioctl_modify_input_setting( struct vcmod_device_spec * dev_spec )
{
	struct vc_device * dev;
	unsigned long flags = 0;

	if( ctrldev->vc_device_count <= dev_spec->idx ){
		return -EINVAL;
	}

	dev = ctrldev->vc_devices[dev_spec->idx];

	spin_lock_irqsave( &dev->in_fh_slock , flags );
	if( dev->fb_isopen ){
		spin_unlock_irqrestore( &dev->in_fh_slock , flags );
		return -EBUSY;
	}
	dev->fb_isopen = 1;
	spin_unlock_irqrestore( &dev->in_fh_slock , flags);

	if( dev_spec->width && dev_spec->height ){
		dev->input_format.width  = dev_spec->width;
		dev->input_format.height = dev_spec->height;
	}

	if( dev_spec->pix_fmt == VCMOD_PIXFMT_YUYV ){
		dev->input_format.pixelformat = V4L2_PIX_FMT_YUYV;
	}else if(dev_spec->pix_fmt == VCMOD_PIXFMT_RGB24){
		dev->input_format.pixelformat = V4L2_PIX_FMT_RGB24;
	}

	if( dev->input_format.pixelformat == V4L2_PIX_FMT_YUYV ){
		dev->input_format.colorspace  = V4L2_COLORSPACE_SMPTE170M;
		dev->input_format.bytesperline = dev_spec->width << 1;
	}else{
		dev->input_format.colorspace  = V4L2_COLORSPACE_SRGB;
		dev->input_format.bytesperline = dev_spec->width * 3;
	}
	dev->input_format.sizeimage = 
		dev->input_format.bytesperline * dev_spec->height;

	spin_lock_irqsave( &dev->in_q_slock, flags );
	vc_in_queue_destroy( &dev->in_queue );
	vc_in_queue_setup(  &dev->in_queue , dev->input_format.sizeimage );

	PRINT_DEBUG("Input format set (%dx%d)(%dx%d)\n",dev_spec->width,dev_spec->height,
		dev->input_format.width,dev->input_format.height);
	spin_unlock_irqrestore( &dev->in_q_slock, flags );

	spin_lock_irqsave( &dev->in_fh_slock , flags );
	dev->fb_isopen = 0;
	spin_unlock_irqrestore( &dev->in_fh_slock , flags);
	return 0;

}

static long ctrl_ioctl( struct file * file, unsigned int ioctl_cmd,
							 unsigned long ioctl_param )
{
	long ret;
	struct vcmod_device_spec dev_spec;

	ret = 0;
	PRINT_DEBUG( "ioctl\n" );

	copy_from_user( &dev_spec, ( void __user * ) ioctl_param,
	 	sizeof(struct vcmod_device_spec) );

	switch( ioctl_cmd ){
		case VCMOD_IOCTL_CREATE_DEVICE:
			PRINT_DEBUG("Requesing new device\n");
			create_new_vcdevice( &dev_spec );
			break;
		case VCMOD_IOCTL_DESTROY_DEVICE:
			PRINT_DEBUG("Requesting destroy of device\n");
			break;
		case VCMOD_IOCTL_GET_DEVICE:
			PRINT_DEBUG("Get device(%d)\n",dev_spec.idx);
			ret = ctrl_ioctl_get_device( &dev_spec );
			if(!ret){
				copy_to_user( (void * __user *) ioctl_param, &dev_spec,
					sizeof(struct vcmod_device_spec) );
			}
			break;
		case VCMOD_IOCTL_MODIFY_SETTING:
			PRINT_DEBUG("Modify setting(%d)\n",dev_spec.idx);
			ctrl_ioctl_modify_input_setting( &dev_spec );
			break;
		default:
			ret = -1;
	}
	return ret;
}

int create_new_vcdevice( struct vcmod_device_spec * dev_spec )
{
	struct vc_device * vcdev;
	int idx;
	PRINT_DEBUG( "creating vcdevice\n" );
	if(! ctrldev ){
		return -ENODEV;
	}

	if( ctrldev->vc_device_count > devices_max ){
		return -ENOMEM;
	}

	if( !dev_spec ){
		vcdev = create_vcdevice(ctrldev->vc_device_count, &default_vcdev_spec );
	}else{
		vcdev = create_vcdevice(ctrldev->vc_device_count, dev_spec );
	}

	if(!vcdev){
		return -ENODEV;
	}

	idx = ctrldev->vc_device_count++;
	ctrldev->vc_devices[idx] = vcdev;
	return idx;
}

inline struct control_device * alloc_control_device(void)
{
	struct control_device * res;
	res  = (struct control_device *) kmalloc( sizeof(*res), GFP_KERNEL);
	if(!res){
		goto return_res;
	}

	res->vc_devices = (struct vc_device **) 
			kmalloc(sizeof(struct vc_device *) * devices_max, GFP_KERNEL);
	if(!(res->vc_devices)){
		goto vcdev_alloc_failure;
	}
	memset(res->vc_devices, 0x00, sizeof(struct vc_devices *) * devices_max );
	res->vc_device_count = 0;

	sema_init(&res->reg_semaphore,1);
	return res;

	vcdev_alloc_failure:
		kfree(res);
		res = NULL;
	return_res:
		return res;
}

inline void destroy_control_device( struct control_device * dev )
{
	size_t i;
	PRINT_DEBUG("destroy control device, %d active_devices\n",
		(int) dev->vc_device_count );
	for( i = 0; i < dev->vc_device_count; i++ ){
		destroy_vcdevice( dev->vc_devices[i] );
	}
	kfree(dev->vc_devices);
	device_destroy( dev->dev_class, dev->dev_number );
	class_destroy( dev->dev_class );
	cdev_del( &dev->cdev );
	unregister_chrdev_region( dev->dev_number, 1);
	kfree(dev);
}

int __init create_ctrldev(const char * dev_name)
{
	int ret = 0;
	PRINT_DEBUG("registering control device\n");
	
	ctrldev = alloc_control_device();
	if(!ctrldev){
		PRINT_ERROR("kmalloc_failed\n");
		ret = -ENOMEM;
		goto kmalloc_failure;
	}

	ctrldev->dev_class = class_create( THIS_MODULE, dev_name );
	if(!(ctrldev->dev_class)){
		PRINT_ERROR("Error creating device class for control device\n");
		ret = -ENODEV;
		goto class_create_failure;
	}

	cdev_init( &ctrldev->cdev, &ctrl_fops );
	ctrldev->cdev.owner = THIS_MODULE;

	ret = alloc_chrdev_region( &ctrldev->dev_number, 0, 1, dev_name );
	if(ret){
		PRINT_ERROR("Error allocating device number\n");
		goto alloc_chrdev_error;
	}

	ret = cdev_add( &ctrldev->cdev, ctrldev->dev_number, 1 );
	PRINT_DEBUG("cdev_add returned %d",ret);
	if( ret < 0 ){
		PRINT_ERROR("device registration failure\n");
		goto registration_failure;
	}

	ctrldev->device = device_create(ctrldev->dev_class, NULL, 
					ctrldev->dev_number, NULL, dev_name, MINOR(ctrldev->dev_number) );
	if(!ctrldev->device){
		PRINT_ERROR("device_create failed\n");
		ret = -ENODEV;
		goto device_create_failure;
	}

	return 0;
	device_create_failure:
		cdev_del( &ctrldev->cdev );
	registration_failure:
		unregister_chrdev_region( ctrldev->dev_number, 1 );
		class_destroy( ctrldev->dev_class );
	alloc_chrdev_error:
	class_create_failure:
		destroy_control_device(ctrldev);
		ctrldev = NULL;
	kmalloc_failure:
		return ret;
}

void __exit destroy_ctrldev(void)
{
	PRINT_DEBUG("module exit called\n");
	if(ctrldev){
		destroy_control_device( ctrldev );
		ctrldev = NULL;
	}
}