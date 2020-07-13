#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "lock_file.h"
#include "../ef_err.h"

void    ef_init_daemon();
int     ef_init_lockfile_to_unique();
int     ef_init_config();
void   *ef_thread_listen(void *);
/*
 *  Everyfile daemon program is used to configure the environment and close/open program.
 */

int main(){
    ef_init_daemon();
    if(ef_init_lockfile_to_unique()){
        syslog(LOG_ERR,"daemon already running");
        exit(1);
    }
    if(ef_init_config()){
        syslog(LOG_ERR,"daemon config file read error");
    }
    /*
     *  Using SIGHUP to signal daemon to re-read configure file because a daemon program have no terminal.
     */
    sa.sa_handler=SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    if(sigaction(SIGHUP,&sa,NULL)<0)
        err_quit("%s[%d]: can't restore SIGHUP default",__FILE__,__LINE__);
    
    sigfillset(&mask);
    if((err=pthread_sigmask(SIG_BLOCK,&mask,NULL))!=0)
        err_exit(err,"%s[%d]: SIG_BLOCK error",__FILE__,__LINE__);
    if((err=pthread_create(&tid,NULL,thr_fn,0))!=0)
        err_exit(err,"%s[%d]: can't create thread",__FILE__,__LINE__);
    
    exit(0);
}

/*
 *  Initialing daemon program that means clearing file creation mask, closing all file descriptors, 
 *  becoming a session leader to lose controlling TTY ans ensuring future opens won't allocate controlling
 *  TTYs. Because stdin, stdout and stderr have been closed, program needs a log file.  
 */

void ef_init_daemon(){
    int                 i;
    pid_t               pid;
    struct rlimit       rl;
    struct sigaction    sa;
    umask(0);
    if(getrlimit(RLIMIT_NOFILE,&rl)<0){
        err_sys("%s[%d]:can't get file limit",__FILE__,__LINE__);
    }
    if((pid=fork())<0){
        err_sys("%s[%d]:can't fork",__FILE__,__LINE__);
    }
    else if(pid!=0)
        exit(0);
    setsid();

    sa.sa_handler=SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;

    if(sigaction(SIGHUP,&sa,NULL)<0)
        err_quit("%s[%d]:can't ignore SIGHUP");
    if((pid=fork())<0)
        err_quit("%s[%d]:can't fork");
    else if(pid!=0)
        exit(0);
    
    if(chdir("/")<0)
        err_quit("%s[%d]:can't change directory to /");
    if(rl.rlim_max==RLIM_INFINITY)
        rl.rlim_max=1024;
    for(i=0;i<rl.rlim_max;i++)
        close(i);

    openlog(__FILE__,LOG_CONS,LOG_DAEMON);
}

#define LOCKFILE "/var/run/everyfiled.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

/*
 *  Function returns 1 when some error happens or daemon have already been running;
 *  returns 0 when daemon program is unique.
 *    
 */
int ef_init_lockfile_to_unique(){
    int fd=open(LOCKFILE,O_RDWR|O_CREAT,LOCKMODE);
    char buf[16];
    if(fd<0){
        syslog(LOG_ERR,"%s[%d]:can't open %s: %s",__FILE__,__LINE__,LOCKFILE,strerror(errno));
        exit(1);
    }
    if(write_lock_whole_file(fd)<0){
        if(errno==EACCES||errno==EAGAIN){
            close(fd);
            return 1;
        }
        syslog(LOG_ERR,"%s[%d]:can't lock %s: %s",__FILE__,__LINE__,LOCKFILE,strerror(errno));
        exit(1);
    }
    ftruncate(fd,0);
    sprintf(buf,"%ld",(long)getpid());
    write(fd,buf,strlen(buf)+1);
    return 0;
}

/*
 *  Function returns 1 when config-file has errors; return 0 when success to re-read. 
 */
int ef_init_config(){
    return 0;
}

void *ef_thread_listen(void *arg){
    int     err,signo;
    for(;;){
        err=sigwait(&mask,&signo);
        if(err!=0){
            syslog(LOG_ERR,"sigwait failed");
            exit(1);
        }
        switch(signo){
            case SIGHUP:
                syslog(LOG_INFO,"RE-reading configuration file");
                ef_init_config();
                break;
            case SIGTERM:
                syslog(LOG_INFO,"got SIGTERM; exiting");
                exit(0);
            default:
                syslog(LOG_INFO,"unexpected fignal %d\n",signo);
        }
    }
    return (0);
}