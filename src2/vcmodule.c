#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#define DEBUG
#include "debug.h"
#include "vccontrol.h"

MODULE_AUTHOR("Jan Sebechlebsky");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual V4L2 Device driver module");

#define CTRL_DEV_NAME "vcdev"

static int __init vcdev_init(void)
{
    int ret;
    PRINT_DEBUG( "init\n" );
    ret = create_ctrldev(CTRL_DEV_NAME);
    return 0;
}

static void __exit vcdev_exit(void)
{
    destroy_ctrldev();
    PRINT_DEBUG( "exit\n" );
}

module_init(vcdev_init)
module_exit(vcdev_exit)