/*
 * NOTE: This example is works on x86 and powerpc.
 * Here's a sample kernel module showing the use of kprobes to dump a
 * stack trace and selected registers when _do_fork() is called.
 *
 * For more information on theory of operation of kprobes, see
 * Documentation/kprobes.txt
 *
 * You will see the trace data in /var/log/messages and on the console
 * whenever _do_fork() is invoked to create a new process.
 * 
 * 0x01: it's neccessary but not agent
 * 0x02: it's not neccessary and not agent
 * 
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

#ifdef CONFIG_SW
#include <uapi/linux/ptrace.h>
#endif



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

typedef struct __vfs_op_args__{
	unsigned char major, minor;
	char* path;
	char buf[PATH_MAX];
}vfs_op_args;

typedef struct __krp_partition__{
	unsigned char major, minor;
	struct list_head list;
	char root[0];
}krp_partition;

static DEFINE_SPINLOCK(sl_parts);
static LIST_HEAD(partitions);

#define MAX_SYMBOL_LEN	64
static char symbol[MAX_SYMBOL_LEN] = "vfs_create";
module_param_string(symbol, symbol, sizeof(symbol), 0644);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp = {
	.symbol_name	= symbol,
};

/*
 *	判断文件系统挂载位置是否特殊，对于特殊的位置，不将其输出。
 */
static int mounted_at(const char *mp, const char *root){
	return strcmp(mp,root)==0||(strlen(mp)>strlen(root)&&strstr(mp, root)==mp&&mp[strlen(root)]=='/');
}

int is_special_mp(const char *mp){
	if(mp==0||*mp!='/')
		return 1;
	return mounted_at(mp,"/sys")||mounted_at(mp, "/proc") ||
		mounted_at(mp, "/run") || mounted_at(mp, "/dev");
}

void __init parse_mounts_info(char *buf, struct list_head *parts){
	if(buf==0)
		return ;
	unsigned int major, minor;
	char mp[NAME_MAX], *line=buf;
	while(sscanf(line, "%*d %*d %d:%d %*s %250s %*s %*s %*s %*s %*s %*s\n",&major, &minor, mp)==3){
		line = strchr(line, '\n')+1;
		if(is_special_mp(mp))
			continue;

		krp_partition *part=kmalloc(sizeof(krp_partition)+strlen(mp)+1, GFP_KERNEL);
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
	struct file *filp = filp_open(filename, O_RDONLY,0);
	if(IS_ERR(filp)){
		pr_err("error opening %s\n", filename);
		return 0;
	}
	mm_segment_t old_fs=get_fs();

	char *buf=0;
	int size=ALLOC_UNIT;
	*real_size=0;
	while(1){
		buf=kmalloc(size, GFP_KERNEL);
		if(unlikely(buf==0))
			break;
		loff_t off=0;
		char __user *user_buf=(char __user*)buf;
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


/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	pr_info("<%s> pre_handler: p->addr = 0x%p, ip = %lx, flags = 0x%lx\n",
		p->symbol_name, p->addr, regs->ip, regs->flags);

	/* A dump_stack() here will give a stack backtrace */
	return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
#ifdef CONFIG_X86
	pr_info("<%s> post_handler: p->addr = 0x%p, flags = 0x%lx\n",
		p->symbol_name, p->addr, regs->flags);
#endif
#ifdef CONFIG_PPC
	pr_info("<%s> post_handler: p->addr = 0x%p, msr = 0x%lx\n",
		p->symbol_name, p->addr, regs->msr);
#endif
#ifdef CONFIG_MIPS
	pr_info("<%s> post_handler: p->addr = 0x%p, status = 0x%lx\n",
		p->symbol_name, p->addr, regs->cp0_status);
#endif
#ifdef CONFIG_TILEGX
	pr_info("<%s> post_handler: p->addr = 0x%p, ex1 = 0x%lx\n",
		p->symbol_name, p->addr, regs->ex1);
#endif
#ifdef CONFIG_ARM64
	pr_info("<%s> post_handler: p->addr = 0x%p, pstate = 0x%lx\n",
		p->symbol_name, p->addr, (long)regs->pstate);
#endif
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	pr_info("fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}

static void __init init_mounts_info(void){
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

static int __init kprobe_init(void)
{
	init_mounts_info();
	int ret;
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_info("Planted kprobe at %p\n", kp.addr);
	return 0;
}

static void __exit kprobe_exit(void)
{
	unregister_kprobe(&kp);
	pr_info("kprobe at %p unregistered\n", kp.addr);
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");
