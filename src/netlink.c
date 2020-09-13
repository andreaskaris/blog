/*
 *
 * Netlink route monitor
 * Also check https://gist.github.com/cl4u2/5204374
 *
 */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <unistd.h>


#define BUF_SIZE 65536

void netlink_receiver();

void main(int argc, char **argv) {
	netlink_receiver();
}

void netlink_receiver() {
	printf("Starting netlink receiverd\n");
	struct sockaddr_nl addr;
	char buf[BUF_SIZE];
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if(fd == -1) {
		printf("Could not create AF_NETLINK socket\n");
	}	
	memset(&addr, 0, sizeof(struct sockaddr_nl));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = RTMGRP_IPV4_ROUTE;
	if(bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_nl)) == -1) {
		printf("Could not bind server socket\n");
		return;
	}
	int bytesReceived;
	char caddrstr[INET_ADDRSTRLEN];
	int len;
	// /usr/include/linux/netlink.h
	struct nlmsghdr * nlh;
	struct rtmsg * rtm;
	struct rtattr * rta;
	int rta_length;
	for (;;) {
		bytesReceived = recv(fd, buf, sizeof(buf), 0);
		if(bytesReceived == -1) {
			printf("Could not receive\n");
		}
		nlh = (struct nlmsghdr *) buf;
		printf(	"nlmsg_len: %d, "
			"nlmsg_type %d, "
			"nlmsg_flags: %d, "
			"nlmsg_seq: %d, "
			"nlmsg_pid: %d\n",
			nlh->nlmsg_len,
			nlh->nlmsg_type,
			nlh->nlmsg_flags,
			nlh->nlmsg_seq,
			nlh->nlmsg_pid);

		rtm = (struct rtmsg *) NLMSG_DATA(nlh);
		printf( "Netmask: %d\n", rtm->rtm_dst_len);
	}
}
