#ifndef VCDEBUG_H
#define VCDEBUG_H

#include <linux/kernel.h>

#ifdef DEBUG
	#define PRINT_DEBUG(arg,...) printk( KERN_INFO "VC,%s(%s):%d -> " arg, __FILE__, __FUNCTION__ , __LINE__ , ##__VA_ARGS__ )
#else
	#define PRINT_DEBUG(arg,...)
#endif
	
	#define PRINT_ERROR(arg,...) printk( KERN_ERR "VC,%s(%s):%d -> " arg, __FILE__, __FUNCTION__ , __LINE__ , ##__VA_ARGS__ )

#endif