#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "debug.h"
#include "vccontrol.h"

MODULE_AUTHOR("Jan Sebechlebsky");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual V4L2 Device driver module");

#define CTRL_DEV_NAME "vcdev"
#define VC_DEV_NAME "virtualcam"

int devices_max = 8;
const char * vc_dev_name = VC_DEV_NAME;
const char * vc_driver_name = VC_DEV_NAME;

static int __init vcdev_init(void)
{
    int ret;
    int i;
    PRINT_DEBUG( "init\n" );
    ret = create_ctrldev(CTRL_DEV_NAME);
    if( ret ){
    	goto error_creating_ctrldev;
    }

    for( i = 0; i < 3; i++ ){
        create_new_vcdevice();
    }

    error_creating_ctrldev:
    return ret;
}

static void __exit vcdev_exit(void)
{
    destroy_ctrldev();
    PRINT_DEBUG( "exit\n" );
}

module_init(vcdev_init)
module_exit(vcdev_exit)