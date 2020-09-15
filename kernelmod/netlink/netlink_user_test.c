#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/netlink.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define NETLINK_TEST 30
#define MSG_LEN 1024
#define MAX_PLOAD 1024

typedef struct __user_msg_info{
	struct nlmsghdr hdr;
	char msg[MSG_LEN];
}user_msg_info;

int main(int argc, char **argv){
	int					skfd, len, ret, maxsock;
	user_msg_info		u_info;
	struct nlmsghdr		*nh;
	struct sockaddr_nl	saddr, daddr;
	fd_set				fdsr;
	struct timeval		tv;
	
	skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
	if(skfd==-1){
		perror("create socket error\n");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = 100;
	saddr.nl_groups = 0;
	if(bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr))!=0){
		perror("bind() error\n");
		close(skfd);
		return -1;
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
	
	return 0;
}
