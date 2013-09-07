#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define PORT1 61000
#define PORT2 61001
#define master_port 1000
#define master_ip "192.168.1.102"
#define server_ip "58.214.236.114"
#define PEER_SHEET_LEN 200

#define UNAME "zhoujie"
#define PASSWD "123456"

static char ver[1];
static char pathname[50] = "./natinfo.log";
static int sfd,listenfd;
static struct sockaddr_in sin, recv_sin, master_addr;	
static int sin_len, recv_sin_len;
static char recv_str[50];
static int port1 = PORT1;
static int port2 = PORT2;
static int masterport = master_port;
static char Uname[10];
static char Passwd[10];
static char buff[20];

static char Peers_Login[PEER_SHEET_LEN][20];
static int  Peers_Sheet_Index = 0;
static int ret = 0;
static int n = 0;
static int Verify = 0;


int local_net_init(int Port){
	int on,ret1;

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	//sin.sin_addr.s_addr = inet_addr(master_ip);;
	sin.sin_port = htons(Port);
	sin_len = sizeof(sin);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(!listenfd) return -1;
	on = 1;			//Bind already in use
	ret1 = setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );


	if(bind(listenfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", Port);

	return 0;
}

int set_master_struct(char * ip1, int port1){
	memset(&master_addr, 0, sizeof(master_addr));
	master_addr.sin_family = AF_INET;
	master_addr.sin_port = htons(port1);
	
	if( inet_pton(AF_INET, ip1, &master_addr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",ip1);
		return -1;
	}

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
		
		if((strcmp(Uname,"zhoujie") == 0) && (strcmp(Passwd,"123456") == 0)){
			Verify = 1;
			printf("Verify success!\n");
		}
		else{
			Verify = 0;
			printf("Verify failed!Wrong Uname or passwd!\n");
		}


		return Verify;
	}
}

int send_info(){
	ver[0] = (int)Verify;
	ver[1] = '\0';

	if( connect(listenfd, (struct sockaddr*)&master_addr, sizeof(master_addr)) < 0){
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}
	if( send(listenfd, ver, strlen(ver), 0) < 0)
	{
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	return 0;

}

int main(){
	int ret = 0;

	ret = local_net_init(port1);
	if(ret < 0){
		printf("local net init fail!\n");
		return ret;
	}

	ret = UAP_check(); //check for uname & passwd

	close(listenfd);
	
	ret = local_net_init(port2);
	if(ret < 0){
		printf("local net init fail!\n");
		return ret;
	}

	ret = set_master_struct(master_ip,masterport);
	if(ret < 0){
		printf("set server ip fail!\n");
		return ret;
	}

	ret = send_info();
	if(ret < 0){
		printf("send check info error!\n");
	}
	return 0;	
}


