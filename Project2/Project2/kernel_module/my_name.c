#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bhavya Patel");

static int intParameter = 2024;
static char *charParameter = "Fall";

module_param(intParameter, int, 0);
module_param(charParameter, charp, 0);

static int __init my_name_init(void) {
	printk(KERN_INFO "Hello, I am Bhavya Patel, a student of CSE330 %s %d.\n", charParameter, intParameter);
	return 0; 
}
static void __exit my_name_exit(void) {
	printk(KERN_INFO "Goodbye from Bhavya.\n");
}

module_init(my_name_init);
module_exit(my_name_exit);
