#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x4c46c04, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xc64f8cc7, __VMLINUX_SYMBOL_STR(vb2_ioctl_reqbufs) },
	{ 0x2691f9d9, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xbb323702, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xe62505cc, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xd8369dd, __VMLINUX_SYMBOL_STR(video_device_release_empty) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x9b1126bd, __VMLINUX_SYMBOL_STR(dev_set_drvdata) },
	{ 0x41096b97, __VMLINUX_SYMBOL_STR(v4l2_device_unregister) },
	{ 0x2e19d8f5, __VMLINUX_SYMBOL_STR(vb2_fop_poll) },
	{ 0x31d561a4, __VMLINUX_SYMBOL_STR(vb2_ioctl_streamon) },
	{ 0x48f9a0c5, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x98a923ff, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0xe2bb627d, __VMLINUX_SYMBOL_STR(__video_register_device) },
	{ 0x2c3615b1, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xe4e8a2d5, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0x7d11c268, __VMLINUX_SYMBOL_STR(jiffies) },
	{ 0x1ebfc36e, __VMLINUX_SYMBOL_STR(v4l2_device_register) },
	{ 0xc17a5790, __VMLINUX_SYMBOL_STR(vb2_fop_read) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x11b397ce, __VMLINUX_SYMBOL_STR(PDE_DATA) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x8e61d64b, __VMLINUX_SYMBOL_STR(vb2_vmalloc_memops) },
	{ 0x8f64aa4, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x71f82f03, __VMLINUX_SYMBOL_STR(vb2_fop_mmap) },
	{ 0xc02d0164, __VMLINUX_SYMBOL_STR(vb2_ioctl_qbuf) },
	{ 0x72728632, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x1f3147c7, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0xf6689573, __VMLINUX_SYMBOL_STR(video_unregister_device) },
	{ 0xb8aba8e4, __VMLINUX_SYMBOL_STR(vb2_plane_vaddr) },
	{ 0xd13f0886, __VMLINUX_SYMBOL_STR(vb2_buffer_done) },
	{ 0x9166fada, __VMLINUX_SYMBOL_STR(strncpy) },
	{ 0x976a9df5, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x99df166d, __VMLINUX_SYMBOL_STR(vb2_ioctl_prepare_buf) },
	{ 0x255e4a5f, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xe2a738a6, __VMLINUX_SYMBOL_STR(vb2_ioctl_create_bufs) },
	{ 0x340698fb, __VMLINUX_SYMBOL_STR(vb2_ioctl_dqbuf) },
	{ 0xd1c71a37, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x270673ce, __VMLINUX_SYMBOL_STR(vb2_fop_release) },
	{ 0xc3fe50a2, __VMLINUX_SYMBOL_STR(video_devdata) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x3bd1b1f6, __VMLINUX_SYMBOL_STR(msecs_to_jiffies) },
	{ 0x79bcd2f2, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0x7149fc2a, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x9327f5ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0xadb5559d, __VMLINUX_SYMBOL_STR(param_ops_byte) },
	{ 0x6b6b3bb6, __VMLINUX_SYMBOL_STR(v4l2_fh_open) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0x4f68e5c9, __VMLINUX_SYMBOL_STR(do_gettimeofday) },
	{ 0xed798b1, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0xe9b09935, __VMLINUX_SYMBOL_STR(vb2_ioctl_querybuf) },
	{ 0x9c55cec, __VMLINUX_SYMBOL_STR(schedule_timeout_interruptible) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xa8eda5f0, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x487d9343, __VMLINUX_SYMBOL_STR(param_ops_ushort) },
	{ 0xba76e385, __VMLINUX_SYMBOL_STR(vb2_ioctl_streamoff) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xac517418, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x21f89d23, __VMLINUX_SYMBOL_STR(dev_get_drvdata) },
	{ 0xd0b53cdb, __VMLINUX_SYMBOL_STR(video_ioctl2) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0x4abfbb43, __VMLINUX_SYMBOL_STR(vb2_queue_init) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=videobuf2-core,videodev,videobuf2-vmalloc";


MODULE_INFO(srcversion, "1E112365B878A3F35C5FF4C");
