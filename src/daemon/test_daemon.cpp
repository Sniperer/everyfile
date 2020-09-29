#include <syslog.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/resource.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <fstream>
#include <linux/netlink.h>

#include "../ef_db.h"
#include "../ef_file.h"
#include "lock_file.h"

#define NETLINK_TEST 30
#define MSG_LEN 1024
#define MAX_PLOAD 1024

#define LOCKFILE "/var/run/everyfiled.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int ef_init_lockfile_to_unique(){
	int fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
	char buf[16];
	if(fd < 0){
		syslog(LOG_ERR, "%s[%d]:can't open %s: %s", __FILE__, __LINE__, LOCKFILE, strerror(errno));
		exit(1);
	}
	if(write_lock_whole_file(fd) < 0){
		if(errno==EACCES||errno==EAGAIN){
			close(fd);
			return 1;
		}
		syslog(LOG_ERR, "%s[%d]:can't lock %s: %s", __FILE__, __LINE__, LOCKFILE, strerror(errno));
		exit(1);
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf)+1);
	return 0;
}

bool start_daemon(){
	int fd;

	switch(fork()){
		case -1:
			printf("fork() failed!\n");
			return false;
		case 0:
			break;
		default:
			exit(0);
	}

	if(setsid() == -1){
		printf("setsid() failed.\n");
		return false;
	}

	switch(fork()){
		case -1:
			printf("fork() failed.\n");
			return false;
		case 0:
			break;
		default:
			exit(0);
	}

	umask(0);
	chdir("/");

	long maxfd;
	if((maxfd = sysconf(_SC_OPEN_MAX)) != -1){
		for(fd=0;fd<maxfd;fd++){
			close(fd);
		}
	}

	fd=open("/dev/null", O_RDWR);
	if(fd == -1){
		printf("open(\"/dev/null\") failed.\n");
		return false;
	}

	if(dup2(fd, STDIN_FILENO) == -1){
		printf("dup2(STDIN) failed.\n");
		return false;
	}

	if(dup2(fd, STDOUT_FILENO) == -1){
		printf("dup2(STDOUT) failed.\n");
		return false;
	}

	if(dup2(fd, STDERR_FILENO) == -1){
		printf("dup2(STDERR) failed.\n");
		return false;
	}

	if(fd > STDERR_FILENO){
		if(close(fd) == -1){
			printf("close() failed.\n");
			return false;
		}
	}

	return true;
}

const ef_db *ef_db::_instance = new ef_db();
file_info_buf buf;

typedef struct __user_msg_info{
	struct nlmsghdr hdr;
	char msg[MSG_LEN];
}user_msg_info;

void *ef_kernel_conn(void *){
	ef_db			   *db_instance = ef_db::get_instance();
	file_info			info;
	int					skfd, ret, maxsock;
	socklen_t			len;
	user_msg_info		u_info;
	struct nlmsghdr		*nh;
	struct sockaddr_nl	saddr, daddr;
	fd_set				fdsr;
	struct timeval		tv;

	skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
	if(skfd == -1){
		exit(-1);
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = 100;
	saddr.nl_groups = 0;
	if(bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0){
		close(skfd);
		exit(-1);
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
	char *umsg = "ACK";
	memcpy(NLMSG_DATA(nh), umsg, strlen(umsg));
	ret = sendto(skfd, nh, nh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
	if(!ret){
		close(skfd);
		exit(-1);
	}

	while(1){
		FD_ZERO(&fdsr);
		FD_SET(skfd, &fdsr);

		tv.tv_sec = 20;
		tv.tv_usec = 0;

		ret = select(skfd+1, &fdsr, NULL, NULL, &tv);
		if(ret < 0){
			break;
		}
		else if(ret == 0){

		}
		else{
			if(FD_ISSET(skfd, &fdsr)){
				memset(&u_info, 0, sizeof(u_info));
				len = sizeof(struct sockaddr_nl);
				for(int i=0; i<MSG_LEN; i++)
					u_info.msg[i]='\0';
				ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&saddr, &len);

				if(!ret){
					exit(-1);
				}
				//solve msg
				ret = u_info.msg[0]-'0';
				switch(ret){
					//vfs_create
					case 1:
						info.file_type=EREG;
						for(int i=0; i<MSG_LEN && u_info.msg[i]!=NULL; i++)
							info.file_name+=u_info.msg[i];
						db_instance->add(info);
						break;
					//vfs_unlink
					case 2:
						for(int i=0; i<MSG_LEN && u_info.msg[i]!=NULL; i++)
							info.file_name+=u_info.msg[i];
						db_instance->rm(info.file_name);
						break;
					//vfs_mkdir
					case 3:
						info.file_type=EDIR;
						for(int i=0; i<MSG_LEN && u_info.msg[i]!=NULL; i++)
							info.file_name+=u_info.msg[i];
						db_instance->add(info);
						break;
					//vfs_rmdir
					case 4:
						for(int i=0; i<MSG_LEN && u_info.msg[i]!=NULL; i++)
							info.file_name+=u_info.msg[i];
						db_instance->rm(info.file_name);
						break;	
					//vfs_symlink
					case 5:
						break;
					//security_inode_create
					case 6:
						info.file_type=EREG;
						for(int i=0; i<MSG_LEN && u_info.msg[i]!=NULL; i++)
							info.file_name+=u_info.msg[i];
						db_instance->add(info);
						break;
					default:
						break;
				}
				ret = sendto(skfd, nh, nh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
				if(!ret){
					close(skfd);
					exit(-1);
				}
			}
		}
	}
}

int main(int argc, char *argv[]){
	start_daemon();
	if(ef_init_lockfile_to_unique()){
		syslog(LOG_ERR, "daemon already running");
		exit(1);
	}
	void *status;
	pthread_t pid;
	pthread_create(&pid, NULL, ef_kernel_conn, 0);
	pthread_join(pid, &status);
	return 0;
}
