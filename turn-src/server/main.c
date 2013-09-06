#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define PORT1 61000
#define master_ip "192.168.1.216"
#define server_ip "58.214.236.114"
#define PEER_SHEET_LEN 200

#define UNAME "zhoujie"
#define PASSWD "123456"

static char pathname[50] = "./natinfo.log";
static int sfd,listenfd;
static struct sockaddr_in sin, recv_sin;	
static int sin_len, recv_sin_len;
static char recv_str[50];
static int port = PORT1;
static char Uname[10];
static char Passwd[10];
static char buff[20];

static char Peers_Login[PEER_SHEET_LEN][20];
static int  Peers_Sheet_Index = 0;
static int ret = 0;
static int n = 0;



int local_net_init(){
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	//sin.sin_addr.s_addr = inet_addr(master_ip);

	sin.sin_port = htons(port);
	sin_len = sizeof(sin);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(!listenfd) return -1;

	if(bind(listenfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", port);

	return 0;
}

int UAP_check(){
	int i = 0,k = 0;

	if( listen(listenfd, 10) == -1){
		printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}

	printf("======waiting for master's request======\n");
	while(1){
		if( (sfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
			printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
			return -1;
		}

		n = recv(sfd, buff, 20, 0);
		if(n == -1)
			printf("recv error: %s(errno: %d)",strerror(errno),errno);
		buff[n] = '\0';
		for(i = 0; i < 20; i++){
			if(buff[i] == ',')
				k = i;
		}
		memcpy(Uname,buff,k);
		Uname[k] = '\0';
		memcpy(Passwd,buff+k+1,n-k-1);
		Passwd[n-k-1] = '\0';

		printf("Uname = %s\n",Uname);
		printf("Passwd = %s\n",Passwd);


		return 0;

	}
}

int main(){
	int ret = 0;

	ret = local_net_init();
	if(ret < 0){
		printf("local net init fail!\n");
		return ret;
	}

	ret = UAP_check();

	return 0;	
}


