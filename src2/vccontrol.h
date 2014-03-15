#ifndef VCCONTROL_H
#define VCCONTROL_H

#include <linux/cdev.h>

struct control_device {
	int major;
	dev_t dev_number;
	struct class * dev_class;
	struct device * device;	
	struct cdev cdev;
};

int create_ctrldev(const char * dev_name);
void destroy_ctrldev(void);

extern struct control_device * ctrldev;

#endif