/*
 *
 * Simple example for UDP client /server
 *
 */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>

#define BUF_SIZE 10
#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"
#define MSG "MSG"

void receiver();
void sender();

int main(int argc, char **argv) {
	if(argc < 2) {
		printf("Please specify the role of this process (sender|receiver\n");
		return 1;
	}
	bool is_receiver = 1;
	if(strcmp(argv[1], "sender") == 0) {
		is_receiver = 0;
	}
	printf("This node is a %s\n", is_receiver ? "receiver" : "sender");

	if(is_receiver)
		receiver();
	else
		sender();
}

void receiver() {
	printf("Starting receiver\n");
	struct sockaddr_in addr, caddr;
	char buf[BUF_SIZE];
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1) {
		printf("Could not create AF_INET SOCK_DGRAM socket\n");
	}	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDRESS, &(addr.sin_addr));
	if(bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
		printf("Could not bind server socket\n");
		return;
	}
	int bytesReceived;
	char caddrstr[INET_ADDRSTRLEN];
	int len;
	for (;;) {
	        int len = sizeof(struct sockaddr_in);
		bytesReceived = recvfrom(
					fd, 
					buf, 
					BUF_SIZE, 
					0, 
			 		(struct sockaddr *) &caddr,
			 		&len);
		if(bytesReceived == -1) {
			printf("Issue with recvfrom\n");
			return;
		}
		inet_ntop(AF_INET, &caddr.sin_addr, caddrstr,INET_ADDRSTRLEN);
		printf("Received datagram from %s\n", caddrstr);
		printf("Datagram content: %s\n", buf);
	}
}

void sender() {
	printf("Starting sender\n");
	struct sockaddr_in addr, caddr;
	char buf[BUF_SIZE];
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1) {
		printf("Could not create AF_INET SOCK_DGRAM socket\n");
	}	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	char * msg = MSG; 
	inet_pton(AF_INET, SERVER_ADDRESS, &(addr.sin_addr));
	if(sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != strlen(msg)) {
		printf("Could not send message to receiver\n");
		return;
	}
}
