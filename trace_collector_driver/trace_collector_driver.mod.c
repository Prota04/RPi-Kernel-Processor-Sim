#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x5df7df87, "device_destroy" },
	{ 0xc467ddce, "class_unregister" },
	{ 0x7057799f, "class_destroy" },
	{ 0x4e54674e, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x92997ed8, "_printk" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xf6e1d11f, "cdev_init" },
	{ 0x42041a93, "cdev_add" },
	{ 0xf795a77a, "class_create" },
	{ 0xe5393233, "device_create" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0xe3e78877, "kmalloc_caches" },
	{ 0x604324bd, "__kmalloc_cache_noprof" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xf1ce2f51, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "62FF81CB746695D6B3A09B9");
