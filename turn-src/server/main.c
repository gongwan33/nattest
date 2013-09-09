#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#define PORT1 61000
#define PORT2 61001
#define PORT3 61002
#define master_port 1000
#define slave_port 2000
#define master_ip "192.168.1.172"
#define slave_ip "192.168.1.172"
#define server_ip "58.214.236.114"
#define PEER_SHEET_LEN 200

#define UNAME "zhoujie"
#define PASSWD "123456"

static char ver[1];
static char ver_slave[1];
static char pathname[50] = "./natinfo.log";
static int sfd, listenfd, listenfd1, sfd1, listenfd2;
static struct sockaddr_in sin, recv_sin, master_addr, slave_addr;	
static int sin_len, recv_sin_len;
static char recv_str[50];
static int port1 = PORT1;
static int port2 = PORT2;
static int port3 = PORT3;
static int masterport = master_port;
static char Uname[10];
static char Passwd[10];
static char buff[20];
static char buffer[20];

static char Peers_Login[PEER_SHEET_LEN][20];
static int  Peers_Sheet_Index = 0;
static int ret = 0;
static int n = 0;
static int Verify_master = 0;
static int Verify_slave = 0;


int local_net_init(int sock,int Port){
	int on,ret1;

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	//sin.sin_addr.s_addr = inet_addr(master_ip);;
	sin.sin_port = htons(Port);
	sin_len = sizeof(sin);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(!sock) return -1;
	on = 1;			//Bind already in use
	ret1 = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );


	if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", Port);
	return sock;
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
			Verify_master = 1;
			printf("Verify success!\n");
			return 1;
		}
		else{
			Verify_master = 0;
			printf("Verify failed!Wrong Uname or passwd!\n");
			return 0;
		}


	}
}

int send_info(){
	int i;
	ver[0] = (int)Verify_master;
	ver[1] = '\0';

	if( send(sfd, ver, strlen(ver), 0) < 0)
	{
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	return 0;

}

void * wait_for_slave(){
	int i = 0,k = 0,ret = 0;
	int sendbytes;
	char slave_info[2];
	//pthread_detach(pthread_self());

	listenfd1 = local_net_init(listenfd1,port3);
	if(listenfd1 < 0){
		printf("local net init fail!\n");
		return ret;
	}	

	if( listen(listenfd1, 10) == -1){
		printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}

	printf("======waiting for slave's request======\n");
	while(1){
		if( (sfd1 = accept(listenfd1, (struct sockaddr*)NULL, NULL)) == -1){
			printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
			return -1;
		}

		n = recv(sfd1, buffer, 20, 0);
		if(n == -1)
			printf("recv error: %s(errno: %d)",strerror(errno),errno);
		buffer[n] = '\0';
		for(i = 0; i < 20; i++){
			if(buffer[i] == ',')
				k = i;
		}
		memcpy(Uname,buffer,k);
		Uname[k] = '\0';
		memcpy(Passwd,buffer+k+1,n-k-1);
		Passwd[n-k-1] = '\0';

		printf("Uname = %s\n",Uname);
		printf("Passwd = %s\n",Passwd);

		if((strcmp(Uname,"zhoujie") == 0) && (strcmp(Passwd,"123456") == 0)){ 
			Verify_slave = 1;
			printf("Slave verify success!\n");
		}
		else{
			Verify_slave = 0;
			printf("Slave verify failed!Wrong Uname or passwd!\n");
		}

		slave_info[0] = Verify_slave;
		if(Verify_master == 1)
			slave_info[1] = 1;
		else if(Verify_master == 0)
			slave_info[1] = 2;
		slave_info[2] = '\0';

		sendbytes = send(sfd1, slave_info, strlen(slave_info), 0);
		printf("sendbytes = %d\n",sendbytes);
		if(sendbytes < 0)
		{
			printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
			return -1;
		}
	}
}

void * wait_for_master(){
	int i, ret = 0;

	listenfd = local_net_init(listenfd,port1);
	if(listenfd < 0){
		printf("local net init fail!\n");
		return ret;
	}

	ret = UAP_check(); //check for uname & passwd
	
	ret = send_info();

	if(ret < 0){
		printf("send check info error!\n");
	}

	while(1){	
		if(Verify_slave == 1){
			ret = send_slave_info();
			if(ret < 0){
				printf("send slave info fail!\n");
				break;
			}
			else if(ret == 0){
				nbytes = recv_data();
			}
			break;
		}
		else{
			//			printf("peer is not ready\n");
			sleep(1);
		}
	}
}

int recv_data(){
	
}

int send_slave_info(){
	int i,sendbytes;

	ver_slave[0] = (int)Verify_slave;
	ver_slave[1] = '\0';

	sendbytes = send(sfd, ver_slave, strlen(ver_slave), 0);
	printf("sendbytes = %d\n",sendbytes);
	if(sendbytes < 0)
	{
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	return 0;
}

int main(){
	int ret = 0;
	void * status;
	void * status1;
	pthread_t pthread_wait_for_slave;
	pthread_t pthread_wait_for_master;

	pthread_create(&pthread_wait_for_master, NULL, wait_for_master, NULL);
	sleep(1);
	pthread_create(&pthread_wait_for_slave, NULL, wait_for_slave, NULL);

	while(1){
		if((Verify_slave && Verify_master) ==1 ){
			printf("Transmit start!\n");

		}
		else{
			sleep(1);
		}
	}
	pthread_join(pthread_wait_for_slave,status);
	pthread_join(pthread_wait_for_master,status1);

	//while(1);	
	close(listenfd);
	return 0;	
}


