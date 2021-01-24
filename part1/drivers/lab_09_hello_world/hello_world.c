#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init hello_wolrd_init(void)
{
	printk(KERN_INFO "Hello, World!\n");
	return 0;
}

static void __exit hello_wolrd_exit(void)
{
	printk(KERN_INFO "Goodbye, World!\n");
}

module_init(hello_wolrd_init);
module_exit(hello_wolrd_exit);

MODULE_AUTHOR("JSC Angstrem-Telecom");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Laboratory work #9");
MODULE_SUPPORTED_DEVICE("Centillium Atlanta 100 WDT");
