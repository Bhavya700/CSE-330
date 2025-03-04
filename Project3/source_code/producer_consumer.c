#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>

static int prod = 0;
static int cons = 0;
static int size = 0;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bhavya Patel");

module_param(prod, int, 0);
module_param(cons, int, 0);
module_param(size, int, 0);

struct semaphore empty, full;
struct task_struct **pthreads;
struct task_struct **cthreads;

static int pt(void *data) {
    int id = *(int *)data;
    while (!kthread_should_stop()) {
        down_interruptible(&empty);
        printk("An item has been produced by Producer-%d\n", id);
        up(&full);
    }
    return 0;
}

static int ct(void *data) {
    int id = *(int *)data;
    while (!kthread_should_stop()) {
        down_interruptible(&full);
        printk("An item has been consumed by Consumer-%d\n", id);
        up(&empty);
    }
    return 0;
}


static int __init producer_consumer_init(void) {
    int i;
    sema_init(&empty, size);
    sema_init(&full, 0);

    pthreads = kmalloc(prod * sizeof(struct task_struct *), GFP_KERNEL);
    cthreads = kmalloc(cons * sizeof(struct task_struct *), GFP_KERNEL);

    int *pids = kmalloc(prod * sizeof(int), GFP_KERNEL);
    int *cids = kmalloc(cons * sizeof(int), GFP_KERNEL);

    for (i = 0; i < prod; i++) {
        pids[i] = i;
        pthreads[i] = kthread_run(pt, &pids[i], "Producer-%d", i);
    }
    for (i = 0; i < cons; i++) {
        cids[i] = i;
        cthreads[i] = kthread_run(ct, &cids[i], "Consumer-%d", i);
    }
    return 0;
}

static void __exit producer_consumer_exit(void) {
    int i;

    for (i = 0; i < prod; i++) {
        if (pthreads[i]) {
            kthread_stop(pthreads[i]);
        }
    }

    for (i = 0; i < cons; i++) {
        if (cthreads[i]) {
            kthread_stop(cthreads[i]);
        }
    }

    kfree(pthreads);
    kfree(cthreads);
}


module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
