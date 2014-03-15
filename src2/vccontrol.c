#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include "vccontrol.h"
#define DEBUG
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

int __init create_ctrldev(const char * dev_name)
{
	int ret = 0;
	PRINT_DEBUG("registering control device\n");
	
	ctrldev = (struct control_device *) kmalloc(sizeof(*ctrldev),GFP_KERNEL);
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
		kfree(ctrldev);
		ctrldev = NULL;
	kmalloc_failure:
		return ret;
}

void __exit destroy_ctrldev(void)
{
	PRINT_DEBUG("destroying control device\n");
	if(ctrldev){
		device_destroy( ctrldev->dev_class, ctrldev->dev_number );
		class_destroy( ctrldev->dev_class );
		cdev_del( &ctrldev->cdev );
		unregister_chrdev_region(ctrldev->dev_number, 1);
		kfree(ctrldev);
		ctrldev = NULL;
	}
}