#ifndef VCCONTROL_H
#define VCCONTROL_H

#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include "vcmod_api.h"

struct control_device {
	int major;
	dev_t dev_number;
	struct class * dev_class;
	struct device * device;	
	struct cdev cdev;
	struct vc_device ** vc_devices;
	size_t vc_device_count;
	struct semaphore reg_semaphore;
};

int create_ctrldev(const char * dev_name);
void destroy_ctrldev(void);

int create_new_vcdevice( struct vcmod_device_spec * dev_spec );

extern struct control_device * ctrldev;

#endif