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

void init_daemon();
int init_lockfile();
int init_config();

int main(){
    init_daemon();
    if(init_lockfile()){
        err_quit("%s[%d]:already running",__FILE__,__LINE__);
    }
    if(init_config()){
        err_quit("%s[%d]:can't set configure file",__FILE__,__LINE__);
    }
}

void init_daemon(){
    int i,fd0,fd1,fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
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

    fd0=open("/dev/NULL",O_RDWR);
    fd1=dup(0);
    fd2=dup(0);

    openlog(__FILE__,LOG_CONS,LOG_DAEMON);
    if(fd0!=0||fd1!=1||fd2!=2){
        syslog(LOG_ERR,"unexpected file descriptors %d %d %d",fd0,fd1,fd2);
        exit(1);
    }
}

#define LOCKFILE "/var/run/everyfile.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int init_lockfile(){
    int fd=open(LOCKFILE,O_RDWR|O_CREAT,LOCKMODE);
    char buf[16];
    if(fd<0){
        syslog(LOG_ERR,"%s[%d]:can't open %s: %s",__FILE__,__LINE__,LOCKFILE,strerror(errno));
        exit(1);
    }
    if(write_lock_whole_file(fd)<0){
        if(errno==EACCES||errno==EAGAIN){
            close(fd);
            return (1);
        }
        syslog(LOG_ERR,"%s[%d]:can't lock %s: %s",__FILE__,__LINE__,LOCKFILE,strerror(errno));
        exit(1);
    }
    ftruncate(fd,0);
    sprintf(buf,"%ld",(long)getpid());
    write(fd,buf,strlen(buf)+1);
    return (0);
}

int init_config(){
    return 0;
}