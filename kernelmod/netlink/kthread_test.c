#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

DEFINE_SPINLOCK(msg_lock);
static int msg_ready = 0;
static struct task_struct *test_task;

int threadfunc(void *data){
	while(1){
		set_current_state(TASK_INTERRUPTIBLE);
		if(kthread_should_stop()) break;
		spin_lock(msg_lock);
		if(msg_ready == 0){
			spin_unlock(msg_lock);
			schedule_timeout(20*HZ);
		}
		else {
			msg_ready--;
			spin_unlock(msg_lock);
			
		}
	}

	return 0;	
}

static int __init test_init(void){
	int err;
	test_task = kthread_create(test_thread, NULL, "test_task");
	if(IS_ERR(test_task)){
		printk("Unable to start kernel thread.\n");
		err = PTR_ERR(test_task);
		test_task = NULL;
		return err;
	}
	wake_up_process(test_task);
	return 0;
}

static void __exit test_exit(void){
	
}

module_init(test_init);
module_exit(test_exit);

