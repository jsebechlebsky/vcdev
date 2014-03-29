#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include "vcdevice.h"
#include "vccontrol.h"
#include "debug.h"

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

static long ctrl_ioctl( struct file * file, unsigned int ioctl_cmd,
							 unsigned long ioctl_param )
{
	PRINT_DEBUG( "ioctl" );
	return 0;
}

int create_new_vcdevice( void )
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

	vcdev = create_vcdevice(ctrldev->vc_device_count);
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