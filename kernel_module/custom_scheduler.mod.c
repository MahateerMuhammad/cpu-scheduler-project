#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x3a907ee7, "module_layout" },
	{ 0xb212b734, "param_ops_int" },
	{ 0x489d4c0d, "single_release" },
	{ 0x9526f10e, "seq_lseek" },
	{ 0xed9b9516, "seq_read" },
	{ 0x37a0cba, "kfree" },
	{ 0xf2d9f835, "kthread_stop" },
	{ 0x25f2949a, "proc_remove" },
	{ 0xe93c407c, "wake_up_process" },
	{ 0x93fa68a0, "kthread_create_on_node" },
	{ 0x6bb01345, "proc_create" },
	{ 0xf9a482f9, "msleep" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x92997ed8, "_printk" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x80827451, "seq_printf" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xc866e771, "single_open" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "41C00DDB0BA5F24FFCBDF8C");
