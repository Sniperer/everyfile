/*
 * This module is works on x86.
 *
 * For more information on theory of operation of kprobes, see
 * Documentation/kprobes.txt
 *
 * You will see the trace data in /var/log/messages.
 * 
 * 0x01: it's neccessary but not agent
 * 0x02: it's not neccessary and not agent
 * 0x03: it's neccessary and agent
 * 0x04: it's not neccessary but agent
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/namei.h>
#include <linux/init.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>

#ifdef CONFIG_SW
#include <uapi/linux/ptrace.h>
#endif


#define NETLINK_TEST	30
#define MSG_LEN			1024
#define USER_PORT		100

typedef struct __vfs_op_args__{
	unsigned char		major, minor;
	char				*path;
	char				buf[PATH_MAX];
}vfs_op_args;

typedef struct __krp_partition__{
	unsigned char		major, minor;
	struct list_head	list;
	char				root[0];
}krp_partition;

typedef struct __krp_entry{
	struct kprobe		kp;
	void				*data;
}krp_entry;

typedef struct __nl_msg{
	struct list_head	list;
	unsigned long		flags;
	unsigned long		len;
	char				path[0]; 
}nl_msg;

static DEFINE_SPINLOCK(sl_parts);
static LIST_HEAD(partitions);
static DEFINE_SPINLOCK(sl_buf);
static LIST_HEAD(msg_buf);
static DEFINE_SPINLOCK(sl_file);
static unsigned int file_on = 0;
static unsigned int msg_buf_size = 0;
static DEFINE_SPINLOCK(sl_msg);
static unsigned int	msg_ready = 0; 
static struct		task_struct *send_kthread;

/*
 *	使用netlink socket用于内核态程序向用户态程序进行通信，为保持数据同步，由用户态向
 *	内核发送报文触发回调函数，发送vfs信息。
 *	
 */
struct sock			*nlsk = NULL;
extern struct net	init_net;
struct file			*fp = NULL;
loff_t				pos = 0;

int send_msg(nl_msg *_msg){
	struct sk_buff	*nl_skb;
	struct nlmsghdr *nlh;
	int8_t			ret;
	char			*_flag;
	pr_info("send %ld to user.\n", _msg->len);
	nl_skb = nlmsg_new(_msg->len+2, GFP_ATOMIC);
	if(!nl_skb){
		printk("netlink alloc failtrue.\n");
		return -1;
	}

	nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_TEST, _msg->len+2, 0);
	if(nlh == NULL){
		printk("nlmsg_put failtrue.\n");
		nlmsg_free(nl_skb);
		return -1;
	}
	_flag = (char *)kmalloc(2, GFP_KERNEL);
	*_flag = (char)(_msg->flags%10+'0');
	*(_flag+1) = ' ';
	memcpy(nlmsg_data(nlh), _flag, 2);
	memcpy(nlmsg_data(nlh)+2, _msg->path, _msg->len);
	ret = netlink_unicast(nlsk, nl_skb, USER_PORT, MSG_DONTWAIT);
	printk("netlink_unicast %d.\n", ret);
	//kfree(_flag);
	//kfree_skb(nl_skb);
	return ret;
}

int send_file_on(void){
	struct sk_buff		*nl_skb;
	struct nlmsghdr		*nlh;
	int8_t				ret;
	char				*msg;
	nl_skb = nlmsg_new(sizeof(loff_t)+8, GFP_ATOMIC);
	if(!nl_skb){
		printk("nlmsg_put failtrue.\n");
		nlmsg_free(nl_skb);
		return -1;
	}
	sprintf(msg, "FILEON%lld", pos);
	memcpy(nlmsg_data(nlh), msg, sizeof(loff_t)+8);
	ret = netlink_unicast(nlsk, nl_skb, USER_PORT, MSG_DONTWAIT);
	return ret;
}

static void rcv_msg(struct sk_buff *skb){
	struct nlmsghdr *nlh = NULL;
	char			*umsg = NULL;

	if(skb->len >= nlmsg_total_size(0)){
		nlh = nlmsg_hdr(skb);
		umsg = NLMSG_DATA(nlh);
		if(umsg){
		//	printk("kernel recv from user: %s.\n", umsg);
			//开启发送线程
			spin_lock(&sl_msg);
			msg_ready++;
			spin_unlock(&sl_msg);
			if(strcmp(umsg, "SYN") == 0){
				wake_up_process(send_kthread);
			}
			else if(strcmp(umsg, "ACK") == 0){
				wake_up_process(send_kthread);
			}
		}
	}

	//kfree_skb(skb);
}

struct netlink_kernel_cfg cfg = {
	.input = rcv_msg,
};

unsigned long get_arg(struct pt_regs* regs, int n){ 
/*
	x86_64 args order: rdi, rsi, rdx, rcx, r8, r9, then comes the stack
*/
	switch(n){
#if defined(CONFIG_X86_64)
		case 1: return regs->di;
		case 2: return regs->si;
		case 3: return regs->dx;
		case 4: return regs->cx;
		case 5: return regs->r8;
		case 6: return regs->r9;
#endif // CONFIG_X86_64
		default:
			return 0;
	}
	return 0;
}

/*
 *	调用之前必须锁定sl_buf。
 */
static inline void *get_msg_from_list(void){
	if(msg_buf.next == &msg_buf)
		return NULL;
	else 
		return (void *)msg_buf.next;
}

int send_func(void *data){
	/*
	 *  由dameon持续发送ACK包，如果没有可用消息，则不进行回复，dameon等待超时，重复。
	 *  kernel查找可用消息，通过msg_buf和fileon状态，如果file_on状态打开，文件内有消息，否则，查询msg_buf_size.
	 *	@msg_ready: 线程周期唤醒，通过其标识是否受到ack包产生异步回调，若是继续执行，否则让出CPU.
	 *	@msg_buf_size: 控制消息缓冲区的大小，每次发送信息从缓冲区减少，先判断file_on，再判断msg_buf_size
	 */
	nl_msg *msg;
	while(1){
		set_current_state(TASK_INTERRUPTIBLE);
		msg = (void *)NULL;
		if(kthread_should_stop()) break;
		spin_lock(&sl_msg);
		if(msg_ready){
			msg_ready = 0;
		}
		else{
			spin_unlock(&sl_msg);
			schedule_timeout(10*HZ);
		}
		spin_unlock(&sl_msg);
		spin_lock(&sl_file);
		//pr_info("get sl_file mutex lock.\n");
		if(file_on){
			send_file_on();
			pos = 0;
			file_on = 0;
		}
		else {
			spin_lock(&sl_buf);
			//pr_info("get sl_buf mutex lock.\n");
			if(msg_buf_size > 0){
				/*	如果可用消息数不为空,发送头消息.
				 *
				 */
				msg = (nl_msg *)get_msg_from_list();
				if(msg != NULL){
					//printk("NULL msg.\n");
					send_msg(msg);
					list_del(&msg->list);
					msg_buf_size--;
					//kfree(msg);
				}		
			}
			spin_unlock(&sl_buf);
		}	
		spin_unlock(&sl_file);
	}
	return 0;
}

#define MAX_SYMBOL_LEN	64
#define MAX_MSG_BUF_LEN 128
#define SYS_CALL_NUM    7

/*
 *	调用前必须获取sl_buf锁。
 */
static void add_msg_to_file(void){
	nl_msg *msg;
	mm_segment_t old_fs;
	char *buf;
	msg = (nl_msg *)get_msg_from_list();
	if(msg != NULL){
		list_del(&msg->list);
		msg_buf_size--;
	}
	buf = (char *)kmalloc(2+msg->len, GFP_KERNEL);
	*buf = (msg->flags%10+'0');
	*(buf+1) = ' ';
	memcpy(buf+2, msg->path, msg->len);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	kernel_write(fp, buf, 2+msg->len, &pos);
	set_fs(old_fs);
}

static int add_vfs_changed(char *fl_path, uint16_t path_len, unsigned long flags){
	nl_msg	*msg = (nl_msg *)kmalloc(sizeof(nl_msg)+path_len, GFP_KERNEL);
	if(msg == NULL)
		return 1;
	msg->flags = flags;
	msg->len = path_len;
	memcpy(msg->path, fl_path, path_len);
	spin_lock(&sl_buf);
	list_add_tail(&msg->list, &msg_buf);
	msg_buf_size++;
	if(msg_buf_size > MAX_MSG_BUF_LEN){
		add_msg_to_file();
		spin_lock(&sl_file);
		file_on = 1;
		spin_unlock(&sl_file);
	}
	spin_unlock(&sl_buf);
	return 0;
}

static void get_root(char *root, unsigned char major, unsigned char minor){
	krp_partition *part;
	*root=0;
	spin_lock(&sl_parts);
	list_for_each_entry(part, &partitions, list) {
		if(part->major == major && part->minor == minor){
			strcpy(root, part->root);
			break;
		}
	}
	spin_unlock(&sl_parts);
}

static int pre_handler_common(struct kprobe* p, struct pt_regs *regs){
	struct dentry	*de = (struct dentry*)get_arg(regs,2);
	krp_entry		*entry = container_of(p, struct __krp_entry, kp);
	vfs_op_args		*args;
	char			*path;
	entry->data = (vfs_op_args*)kmalloc(sizeof(vfs_op_args),GFP_KERNEL);
	if(entry->data==0||de==0||de->d_sb==0)
		return 1;
	args = entry->data;
	args->path = 0;
	args->major = MAJOR(de->d_sb->s_dev);
	args->minor = MINOR(de->d_sb->s_dev);
	path=dentry_path_raw(de, args->buf,sizeof(args->buf));
	if(IS_ERR(path))
		return 1;
	args->path=path;
	return 0;
}

static int mounted_at(const char *, const char *);
int is_special_mp(const char *);

static void post_handler_common(struct kprobe* p, struct pt_regs *regs, unsigned long flags){
	unsigned int	len, ret_val = 0;
	char			root[NAME_MAX], *fl_path;
	krp_entry		*entry;
	vfs_op_args		*args;
	ret_val=regs_return_value(regs);
	if(!is_syscall_success(regs)){
		pr_info("syscall %ld failed.\n", flags);
		return;	
	}
	entry = container_of(p, struct __krp_entry, kp);
	args = (vfs_op_args *)entry->data;
	if(args == 0 || args->path == 0){
		pr_info("args->path null? in proc[%d]: %s\n", current->pid, current->comm);
	}
	get_root(root, args->major, args->minor);
	if(*root==0)
		return ;
	if(is_special_mp(root))
		return ;
	pr_info("args->path %s%s OK in proc[%d]: %s\n", root, args->path, current->pid, current->comm);
	if(mounted_at(root, "/")){
		fl_path = (char *)kmalloc(strlen(args->path), GFP_KERNEL);
		if(fl_path == NULL) return ;
		memcpy(fl_path, args->path, strlen(args->path));
		len = strlen(args->path);
	}
	else{
		fl_path = (char *)kmalloc(strlen(root)+strlen(args->path), GFP_KERNEL);
		if(fl_path == NULL)	return;
		memcpy(fl_path, root, strlen(root));
		memcpy(fl_path+strlen(root), args->path, strlen(args->path));
		len = strlen(root) + strlen(args->path);
	}
	if(add_vfs_changed(fl_path, len, flags)){
		printk("%s add to buffer error.\n", fl_path);
	}
	kfree(fl_path);
}

static int fault_handler_common(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	pr_info("fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}

#define DECL_VFS_KRP(fn, act) static void post_handler_##fn(struct kprobe *p, struct pt_regs *regs, unsigned long ignored)\
{\
	return post_handler_common(p, regs, act);\
}\
\
static struct __krp_entry entry_##fn = {\
	.kp={\
		.symbol_name	= ""#fn"",\
		.pre_handler	= pre_handler_common,\
		.post_handler	= post_handler_##fn,\
		.fault_handler  = fault_handler_common,\
	},\
	.data=NULL,\
};

/*static struct __krp_entry entry_vfs_mkdir = {
	.kp={
		.symbol_name  = "vfs_mkdir",
		.pre_handler  = pre_handler_common,
		.post_handler = post_handler_common
	},
	.data=NULL
};
*/
DECL_VFS_KRP(vfs_create, 1);
DECL_VFS_KRP(vfs_unlink, 2);
DECL_VFS_KRP(vfs_mkdir, 3);
DECL_VFS_KRP(vfs_rmdir, 4);
DECL_VFS_KRP(vfs_symlink, 5);
DECL_VFS_KRP(security_inode_create, 6);

/*
 *	判断文件系统挂载位置是否特殊，对于特殊的位置，不将其输出。
 *  0x01: 有些目录不是文件系统挂载的根目录，如/var，但同时不是用户敏感的
 *  目录，因为包含了lock文件和一些临时文件等，我们需要将其过滤掉。过滤功能交由
 *  内核实现还是用户程序实现，是一个值得考虑的问题。
 */
static int mounted_at(const char *mp, const char *root){
	return strcmp(mp,root)==0||(strlen(mp)>strlen(root)&&strstr(mp, root)==mp&&mp[strlen(root)]=='/');
}

int is_special_mp(const char *mp){
	if(mp==0||*mp!='/')
		return 1;
	return mounted_at(mp,"/sys")||mounted_at(mp, "/proc") ||
		mounted_at(mp, "/run") || mounted_at(mp, "/dev") || mounted_at(mp, "/var") ||
		mounted_at(mp, "/tmp");
}

/*
 * 按设备号排入链表，由get_root获取当前文件系统挂载的根目录。
 */
void parse_mounts_info(char *buf, struct list_head *parts){
	unsigned int		major, minor;
	char				mp[NAME_MAX],*line=buf;
	krp_partition		*part;
	if(buf==0)
		return ;
	while(sscanf(line, "%*d %*d %d:%d %*s %250s %*s %*s %*s %*s %*s %*s\n",&major, &minor, mp)==3){
		line = strchr(line, '\n')+1;
		if(is_special_mp(mp))
			continue;

		part=kmalloc(sizeof(krp_partition)+strlen(mp)+1, GFP_KERNEL);
		if(unlikely(part==0)){
			pr_err("krp-partition kmalloc failed for %s\n", mp);
			continue;
		}
		part->major=major;
		part->minor=minor;
		strcpy(part->root,mp);
		list_add_tail(&part->list, parts);
	}
}

/*
 * 封装了kernel读文件.4kB一分配,与linux page size一致。
 */
#define ALLOC_UNIT (1<<12)

char* __init read_file_content(const char *filename, int *real_size){
	struct file		*filp = filp_open(filename, O_RDONLY,0);
	mm_segment_t	old_fs;
	char			*buf;
	int				size;
	loff_t			off;
	char __user		*user_buf;
	if(IS_ERR(filp)){
		pr_err("error opening %s\n", filename);
		return 0;
	}
	old_fs=get_fs();
	set_fs(KERNEL_DS);
	buf=0;
	size=ALLOC_UNIT;
	*real_size=0;
	while(1){
		buf=kmalloc(size, GFP_KERNEL);
		if(unlikely(buf==0))
			break;
		off=0;
		user_buf=(char __user*)buf;
		*real_size=filp->f_op->read(filp, user_buf, size, &off);
		if(*real_size>0&&*real_size<size){
			buf[*real_size]=0;
			break;
		}

		real_size+=ALLOC_UNIT;
		kfree(buf);

	}
	set_fs(old_fs);
	filp_close(filp, 0);
	pr_info("%s size: %d\n",filename,*real_size);
	return buf;
}

/*
 * init_mounts_info: this is called before function kprobe_exit to initialize file system
 * root path by dev.major and dev.minor.
 */
static void init_mounts_info(void){
	// 0x01: if mnt_namespace is valid?
	//char *cur_path = 0, *path_buf = kmalloc(PATH_MAX, GFP_KERNEL);
	// 0x02: get current path 
	// 0x02: get cmd line
	// 0x02: get mount info
	int parts_count=0;
	krp_partition *part;
	int size=0;
	char* buf=read_file_content("/proc/self/mountinfo", &size);

	parse_mounts_info(buf, &partitions);
	list_for_each_entry(part, &partitions, list){
		parts_count++;
		pr_info("mp: %s, major: %d, minor: %d\n",part->root,part->major,part->minor);
	}

	if(buf)
		kfree(buf);
}

static struct kprobe* krp_entrys_kp[] = {&entry_vfs_create.kp, &entry_vfs_unlink.kp, 
	&entry_vfs_mkdir.kp, &entry_vfs_rmdir.kp, &entry_vfs_symlink.kp, &entry_security_inode_create.kp
};
/*
 * kprobe_exit: this is called when the module init.
 */
static int __init kprobe_init(void)
{
	int		ret;
	init_mounts_info();
	fp = filp_open("/var/log/everyfile.log", O_RDWR | O_CREAT, 0);
	if(IS_ERR(fp)){
		printk("open file error!\n");
		return -1;
	}
	nlsk = (struct sock *)netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);
	send_kthread = kthread_create(send_func, NULL, "send_kthread");
	if(nlsk == NULL){
		printk("netlink_kernel_create error!\n");
		return -1;
	}
	ret = register_kprobes(krp_entrys_kp, sizeof(krp_entrys_kp)/sizeof(void*));
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_info("Planted kprobe at %p\n", entry_vfs_mkdir.kp.addr);
	return 0;
}

/*
 * kprobe_exit: this is called when the module exit. 
 */
static void __exit kprobe_exit(void)
{
	unregister_kprobe(&entry_vfs_mkdir.kp);
	kthread_stop(send_kthread);
	while(msg_buf_size){
		add_msg_to_file();
		msg_buf_size--;
	}
	pr_info("kprobe at %p unregistered\n", entry_vfs_mkdir.kp.addr);
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");
