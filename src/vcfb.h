#ifndef VCFB_H
#define VCFB_H

#include "vcdevice.h"
#include <linux/proc_fs.h>

struct proc_dir_entry* init_framebuffer( const char * proc_fname, struct vc_device * dev );

void destroy_framebuffer( const char * proc_fname );

#endif