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

unsigned short devices_max = 8;
unsigned short create_devices = 1;
unsigned char  allow_pix_conversion = 1;
unsigned char  allow_scaling = 1;


module_param( devices_max, ushort, 0 );
MODULE_PARM_DESC( devices_max, "Maximal number of devices\n");

module_param( create_devices, ushort, 0);
MODULE_PARM_DESC( create_devices, "Number of devices to be created during initialization\n");

module_param( allow_pix_conversion, byte, 0);
MODULE_PARM_DESC( allow_pix_conversion, "Allow pixel format conversion by default\n");

module_param( allow_scaling, byte, 0);
MODULE_PARM_DESC( allow_scaling, "Allow image scaling by default\n");



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

    for( i = 0; i < create_devices; i++ ){
        create_new_vcdevice( NULL );
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