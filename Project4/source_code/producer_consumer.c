#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>

#define EXIT_ZOMBIE 0x00000020

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bhavya Patel");

static int prod = 1;
static int cons = 0;
static int size = 0;
static int uid = 0;
module_param(prod, int, 0);
module_param(cons, int, 0);
module_param(size, int, 0);
module_param(uid, int, 0);

struct task_struct *pthread;
struct task_struct **cthreads;
struct task_struct **buffer;
int bcount = 0;
int bsize;
struct semaphore empty;
struct semaphore full;
spinlock_t block;

static int producer_function(void *data) {
    struct task_struct *task;
    while (!kthread_should_stop()) {
        for_each_process(task) {
            if (task->exit_state & EXIT_ZOMBIE && task->cred->uid.val == uid) {
                if (down_interruptible(&empty)) {
                    continue;
                }
                spin_lock(&block);
                buffer[bcount++] = task;
                printk(KERN_INFO "[Producer-1] has produced a zombie process with pid %d and parent pid %d\n", task->pid, task->parent->pid);
                spin_unlock(&block);
                up(&full);
            }
        }
        msleep(250);
    }
    return 0;
}

static int consumer_function(void *data) {
    int id = *(int *)data;
    struct task_struct *task;
    while (!kthread_should_stop()) {
        if (down_interruptible(&full)) {
            continue;
        }
        spin_lock(&block);
        task = buffer[--bcount];
        printk(KERN_INFO "[Consumer-%d] has consumed a zombie process with pid %d and parent pid %d\n", id, task->pid, task->parent->pid);
        spin_unlock(&block);
        up(&empty);
        kill_pid(task_pid(task->parent), SIGKILL, 0);
    }
    return 0;
}

static int __init producer_consumer_init(void) {
    int i;
    bsize = size;
    buffer = kmalloc_array(bsize, sizeof(struct task_struct *), GFP_KERNEL);
    sema_init(&empty, bsize);
    sema_init(&full, 0);
    spin_lock_init(&block);
    pthread = kthread_run(producer_function, NULL, "Producer-1");
    cthreads = kmalloc_array(cons, sizeof(struct task_struct *), GFP_KERNEL);
    for (i = 0; i < cons; i++) {
        int *id = kmalloc(sizeof(int), GFP_KERNEL);
        *id = i;
        cthreads[i] = kthread_run(consumer_function, id, "Consumer-%d", i);
    }
    return 0;
}

static void __exit producer_consumer_exit(void) {
    int i;
    kthread_stop(pthread);
    for (i = 0; i < cons; i++) {
        kthread_stop(cthreads[i]);
    }
    kfree(cthreads);
    kfree(buffer);
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
