#include <linux/module.h>
#include "vcfb.h"
#include "debug.h"

static ssize_t vcfb_write( struct file * file, const char __user * buffer, size_t length,
	loff_t * offset );

struct file_operations vcfb_fops = {
	.owner   = THIS_MODULE,
	.write   = vcfb_write,
};

struct proc_dir_entry* init_framebuffer( const char * proc_fname )
{
	//TODO
	int ret;
	struct proc_dir_entry * procf;
	ret = 0;

	PRINT_DEBUG( "Creating framebuffer for /dev/%s\n" ,proc_fname );
	procf = proc_create_data( proc_fname, 0644, NULL , &vcfb_fops, NULL);
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

static ssize_t vcfb_write( struct file * file, const char __user * buffer, size_t length,
	loff_t * offset )
{

	PRINT_DEBUG("Write %ld Bytes req\n",length);

	return length;
}