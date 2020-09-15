#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

#define NETLINK_TEST	30
#define MSG_LEN			1024
#define USER_PORT		100

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("netlink example");

DEFINE_SPINLOCK(msg_lock);
static int msg_ready = 0;
static struct task_struct *test_task;
struct sock *nlsk = NULL;
extern struct net init_net;

int send_usrmsg(char *pbuf, uint16_t len){
	struct sk_buff	*nl_skb;
	struct nlmsghdr *nlh;
	int				ret;

	nl_skb = nlmsg_new(len, GFP_ATOMIC); //create a new netlink message
	if(!nl_skb){
		printk("netlink alloc failure\n");
		return -1;
	}

	nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_TEST, len, 0); //add a netlink message to an skb
	if(nlh==NULL){
		printk("nlmsg_put failaure \n");
		nlmsg_free(nl_skb);
		return -1;
	}

	memcpy(nlmsg_data(nlh), pbuf, len); //add pbuf to nlmsg payload 
	ret = netlink_unicast(nlsk, nl_skb, USER_PORT, MSG_DONTWAIT); //unicast a message to a single socket
	printk("netlink_unicast %d\n", ret);
	return ret;
}

static void netlink_rcv_msg(struct sk_buff *skb){
	struct nlmsghdr *nlh=NULL;
	char *umsg = NULL;
	char *kmsg = "hello users!!!";

	if(skb->len >= nlmsg_total_size(0)){
		nlh = nlmsg_hdr(skb);
		umsg = NLMSG_DATA(nlh);
		if(umsg){
			if(strcmp(umsg, "SYN")==0){
				//wake_up_process();
			}
			else if(strcmp(umsg, "ACK")==0){
				//wake_up_process();
			}
			else{
				send_usrmsg("ERR",4);
			}
		}
	}
}

struct netlink_kernel_cfg cfg = {
	.input = netlink_rcv_msg,
};

int __init test_netlink_init(void){
	nlsk = (struct sock*)netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);
	if(nlsk==NULL){
		printk("netlink_kernel_create error!\n");
		return -1;
	}
	printk("test_netlink_init\n");
	return 0;
}

void __exit test_netlink_exit(void){
	if(nlsk){
		netlink_kernel_release(nlsk);
		nlsk=NULL;
	}
	printk("test_netlink_exit!!!\n");
}

module_init(test_netlink_init);
module_exit(test_netlink_exit);
