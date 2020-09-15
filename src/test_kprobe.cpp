/*
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

#define MAX_SYMBOL_LEN 64
static char symbol[MAX_SYMBOL_LEN]="_do_fork";
module_param_string(symbol,symbol,sizeof(symbol),0644);

static struct kprobe kp={
    .symbol_name = symbol,
};

static int handler_pre(struct kprobe *p, struct pt_reg *regs){
    #ifdef CONFIG_X86
        pr_info("<%s> pre_handler: p->addr = %pF,ip = %lx, flags = 0x%lx\n",
            p->symbol_name,p->addr,regs->ip,regs->flags);
    #endif
    #ifdef CONFIG_ARM64
        pr_info("<%s> pre_handler: p->addr = %pF, pc = 0x%lx,"
            " pstate = 0xx%lx\n",
            p->symbol_name,p->addr,(long)regs>pc,(long)regs->pstate);
    #endif

    return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
    #ifdef CONFIG_X86
        pr_info("<%s> post_handler: p->addr = %pF, flags = 0x%lx\n",
            p->symbol_name,p->addr,regs->flags);
    #endif
    #ifdef CONFIG_ARM64
        pr_info("<%s> post_handler: p->addr = %pF, pstate = 0x%lx\n",
            p->symbol_name,p->addr,(long)regs->pstate);
    #endif
}

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr){
    pr_info("fault_handler: p->addr = %pF, trap #%dn",p->addr,trapnr);
    return 0;
}

static int __init kprobe_init(void){
    int ret;
    kp.pre_handler=handler_pre;
    kp.post_handler=handler_post;
    kp.fault_handler=handler_fault;

    ret=register_kprobe(&kp);
    if(ret<0){
        pr_err("register_kprobe failed, returned %d\n",ret);
        return ret;
    }
    pr_info("Planted kprobe at %pF\n",kp.addr);
    return 0;
}

static void __exit kprobe_exit(void){
    unregister_kprobe(&kp);
    pr_info("kprobe at %pF unregistered\n",kp.addr);
}

module_init(kprobe_init);
*/