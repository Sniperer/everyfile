#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
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

#define NETLINK_TEST 30
#define MSG_LEN 1024
#define MAX_PLOAD 1024

void    ef_init_daemon();
int     ef_init_lockfile_to_unique();
int     ef_init_config();
void   *ef_thread_listen(void *);
/*
 *  Everyfile daemon program is used to configure the environment and close/open program.
 */

typedef struct __user_msg_info{
    struct nlmsghdr hdr;
    char msg[MSG_LEN];
}user_msg_info;

void *ef_kernel_conn(){
	int					skfd, len, ret, maxsock;
	user_msg_info		u_info;
	struct nlmsghdr		*nh;
	struct sockaddr_nl	saddr, daddr;
	fd_set				fdsr;
	struct timeval		tv;
	
	skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
	if(skfd==-1){
		perror("create socket error\n");
		return ;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = 100;
	saddr.nl_groups = 0;
	if(bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr))!=0){
		perror("bind() error\n");
		close(skfd);
		return ;
	}

	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;
	daddr.nl_groups = 0;

	nh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
	memset(nh, 0, sizeof(struct nlmsghdr));
	nh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
	nh->nlmsg_flags = 0;
	nh->nlmsg_type = 0;
	nh->nlmsg_seq = 0;
	nh->nlmsg_pid = saddr.nl_pid;
	char *umsg = "SYN";
	memcpy(NLMSG_DATA(nh), umsg, strlen(umsg));
	ret = sendto(skfd, nh, nh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
	if(!ret){
		perror("sendto error\n");
		close(skfd);
		exit(-1);
	}
	printf("send kernel:%s\n", umsg);
/*
	memset(&u_info, 0, sizeof(u_info));
	len = sizeof(struct sockaddr_nl);
	ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&saddr, &len);
	if(!ret){
		perror("recv from kernel error\n");
		close(skfd);
		exit(-1);
	}
	printf("from kernel:%s\n", u_info.msg);
*/
	//	close(skfd);

	
	while(1){
		FD_ZERO(&fdsr);
		FD_SET(skfd, &fdsr);

		tv.tv_sec = 20;
		tv.tv_usec = 0;

		ret = select(skfd+1, &fdsr, NULL, NULL, &tv);
		if(ret < 0){
			perror("select error!\n");
			break;
		}
		else if(ret == 0){
			printf("timeout!\n");
			continue;
		}
		if(FD_ISSET(skfd, &fdsr)){
			char *pbuf = "hello user.\n";
			memset(&u_info, 0, sizeof(u_info));
			len = sizeof(struct sockaddr_nl);
			ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&saddr, &len);
			if(!ret){
				perror("recv from kernel error\n");
				exit(-1);
			}
//			printf("from kernel:%s\n", u_info.msg);
			if(strcmp(u_info.msg, "hello")==0){
				printf("init.\n");
				
			}
			else if(strcmp(u_info.msg, "seq")==0){
				printf("continue.\n");
			}
			else{
				break;
			}
		}
	}

	free((void *)nh);
	
	return ;
}

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
 *  becoming a session leader to lose controlling TTY and ensuring future opens won't allocate controlling
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
