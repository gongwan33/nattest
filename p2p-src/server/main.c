#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT1 61000
#define ip1   "192.168.1.216"
#define ip2   "192.168.1.116"

static char pathname[50] = "./natinfo.log";
static int sfd;
static struct sockaddr_in sin;	
static int sin_len;
static char recv_str[100];
static int port = PORT1;

int local_net_init(){
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	//sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_addr.s_addr = inet_addr(ip1);

	sin.sin_port = htons(port);
	sin_len = sizeof(sin);

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(!sfd) return -1;

	if(bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", port);

	return 0;
}

int main(){
	int ret = 0;
	ret = local_net_init();
	if(ret < 0){
		printf("local net init failed!!\n");
		return ret;
	}

	printf("-------------------Wait for Login---------------------\n");
	int i = 0;
	for(i = 0; i < 10; i++){		
		recvfrom(sfd, recv_str, sizeof(recv_str), 0, (struct sockaddr *)&sin, &sin_len);
		printf("Recieve from %s [%d]:%s\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), recv_str);
	}
	close(sfd);

	return 0;
}
