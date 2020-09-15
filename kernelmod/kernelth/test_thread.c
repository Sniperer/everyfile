#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("kthread example");

/**
 *	kthread_create - create a thread on the current node
 *	@threadfn: the function to run in the thread
 *	@data: data pointer for @threadfn()
 *	@namefmt: printf-style format string for the thread name
 *	@arg...: arguments for @namefmt.
 */
static struct task_struct *test_task;
static int test_init_module(void){
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

int threadfunc(void *data){
	while(1){
		set_current_state(TASK_UNINTERRUPTIBLE);
		if(kthread_should_stop()) break;
		if(msg_buf_size>0){
			
		}
		else{
			schedule_timeout(HZ);
		}
	}

	return 0;
}

static void test_cleanup_module(void){
	if(test_task){
		kthread_stop(test_task);
		test_task = NULL;
	}
}

int __init test_thread_init(void){
	
}

void __exit test_thread_exit(void){

}

module_init(test_thread_init);
module_exit(test_thread_exit);
