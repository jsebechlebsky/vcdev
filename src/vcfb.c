#include <linux/module.h>
#include <linux/spinlock.h>
#include "vcfb.h"
#include "vcdevice.h"
#include "vcvideobuf.h"
#include "debug.h"

static ssize_t vcfb_write( struct file * file, const char __user * buffer, size_t length,
	loff_t * offset );

static int vcfb_open( struct inode *ind, struct file *file );
static int vcfb_release( struct inode * ind, struct file * file );

struct file_operations vcfb_fops = {
	.owner   = THIS_MODULE,
	.open    = vcfb_open,
	.release = vcfb_release,
	.write   = vcfb_write,
};

struct proc_dir_entry* init_framebuffer( const char * proc_fname, struct vc_device * dev )
{
	//TODO
	int ret;
	struct proc_dir_entry * procf;
	ret = 0;

	PRINT_DEBUG( "Creating framebuffer for /dev/%s\n" ,proc_fname );
	procf = proc_create_data( proc_fname, 0644, NULL , &vcfb_fops, dev );
	if( !procf ){
		PRINT_ERROR("Failed to create procfs entry\n");
		ret = -ENODEV;
		goto failure;
	}

	return procf;
	failure:
		return NULL;
}

void destroy_framebuffer( const char * proc_fname )
{
	if(!proc_fname)
		return;
	PRINT_DEBUG( "Destroying framebuffer %s\n", proc_fname );
	remove_proc_entry( proc_fname, NULL );
}

static int vcfb_open( struct inode *ind, struct file *file )
{
	struct vc_device * dev;
	unsigned long flags = 0;

	PRINT_DEBUG("Open framebuffer proc file\n");

	dev = PDE_DATA(ind);
	if(!dev){
		PRINT_ERROR("Private data field of PDE not initilized.\n");
		return -ENODEV;
	}

	spin_lock_irqsave( &dev->in_fh_slock , flags );
	if( dev->fb_isopen ){
		spin_unlock_irqrestore( &dev->in_fh_slock , flags );
		return -EBUSY;
	}
	dev->fb_isopen = 1;
	spin_unlock_irqrestore( &dev->in_fh_slock , flags);

	file->private_data = dev;

	return 0;
}

static int vcfb_release( struct inode * ind, struct file * file )
{
	struct vc_device * dev;
    unsigned long flags = 0;
	dev = PDE_DATA(ind);
	spin_lock_irqsave( &dev->in_fh_slock , flags );
	dev->fb_isopen = 0;
	spin_unlock_irqrestore( &dev->in_fh_slock , flags );
	return 0;
}

static ssize_t vcfb_write( struct file * file, const char __user * buffer, size_t length,
	loff_t * offset )
{
	struct vc_device * dev;
	struct vc_in_queue * in_q;
	struct vc_in_buffer * buf;
	size_t waiting_bytes;
	size_t to_be_copyied;
	unsigned long flags = 0;
	void * data;

	//PRINT_DEBUG("Write %ld Bytes req\n", (long)length);

	dev = file->private_data;
	if( !dev ){
		PRINT_ERROR("Private data field of file not initialized yet.\n");
		return 0;
	}

	waiting_bytes = dev->input_format.sizeimage;

	in_q = &dev->in_queue;

	buf = in_q->pending;
	if( !buf ){
		PRINT_ERROR("Pending pointer set to NULL\n");
		return 0;
	}
	//Fill the buffer
	//TODO real buffer handling
	to_be_copyied = length;
	if( (buf->filled + to_be_copyied) > waiting_bytes ){
		to_be_copyied = waiting_bytes - buf->filled;
	}

	data = buf->data;
	if( !data ){
		PRINT_ERROR("NULL pointer to framebuffer");
		return 0;
	}

	if( !buf->filled ){
		do_gettimeofday( &buf->ts );
	}

	copy_from_user( data + buf->filled, (void *) buffer, to_be_copyied );
	buf->filled += to_be_copyied;
	//PRINT_DEBUG("Received %d/%d B\n", (int)buf->filled, (int)waiting_bytes);

	if ( buf->filled == waiting_bytes ){
		spin_lock_irqsave( &dev->in_q_slock, flags );
		swap_in_queue_buffers( in_q );
		spin_unlock_irqrestore( &dev->in_q_slock, flags );
		PRINT_DEBUG("Swapping buffers\n");
	}

	return length;
}