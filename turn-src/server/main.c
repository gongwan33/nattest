#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#include "info.h"

static char ver[1];
static char ver_slave[1];
static char pathname[50] = "./natinfo.log";
static int sfd, listenfd, listenfd1, sfd1, listenfd2, sfd_udp, sockfd_udp_dst;
static struct sockaddr_in sin, recv_sin, master_addr, slave_addr,local_addr, dst_addr;	
static int sin_len, recv_sin_len;
static char recv_str[50];
static int port1 = server_port1;
static int port2 = server_port2;
static int port3 = server_port3;
static int masterport = master_port;
static char Uname[10];
static char Passwd[10];
static char buff[20];
static char buffer[20];
static char data_buff[10 * 1024];
static int Start_send = 0;

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

int local_net_init_udp(){
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	//sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_addr.s_addr = inet_addr(server_ip);

	sin.sin_port = htons(port3);
	sin_len = sizeof(sin);

	sfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(!sfd_udp) return -1;
	
	if(bind(sfd_udp, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", port3);

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

	listenfd1 = local_net_init(listenfd1,port2);
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
				Start_send = 1;
				printf("send slave info ok!\n");
			}
			break;
		}
		else{
			//			printf("peer is not ready\n");
			sleep(1);
		}
	}
}

int dst_net_init_udp(){
	int ret1;
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(server_port4);

	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.sin_family = AF_INET;
	//dst_addr.sin_addr.s_addr = inet_addr(server_ip);
	if( inet_pton(AF_INET,slave_ip, &dst_addr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",server_ip);
		return -1;
	}	
	dst_addr.sin_port = htons(slave_port2);
	bzero(&(dst_addr.sin_zero), 8);
	if((sockfd_udp_dst = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return -1;
	}
	if(bind(sockfd_udp_dst, (struct sockaddr *) (&local_addr),sizeof(local_addr)) == -1){
		printf ("Bind error: %s\a\n", strerror (errno));
		return -2;
	}	
	return 0;
}
void * recv_data(){
	int ret;
	int num_recv = 0;
	int sendbytes;

	ret = local_net_init_udp();
	if(ret < 0){
		printf("Local bind failed!!%d\n", ret);
	}
	
	ret = dst_net_init_udp();
	if(ret < 0){
		printf("Local bind failed!!%d\n", ret);
	}

	recv_sin_len = sizeof(recv_sin);
	while(1){
		if((num_recv = recvfrom(sfd_udp, data_buff, sizeof(data_buff), 0, (struct sockaddr *)&recv_sin, &recv_sin_len)) < 0){
			perror("recv");
		}
		else{
			printf("recv = %d\n",num_recv);
			if((sendbytes = sendto(sockfd_udp_dst, data_buff, num_recv, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr))) < 0)
				perror("send");
			else
				printf("bytes send to slave: %d\n",sendbytes);
		}
	}
	//	printf("buff:%s\n",data_buff);

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
	void * status2;

	pthread_t pthread_wait_for_slave;
	pthread_t pthread_wait_for_master;
	pthread_t pthread_start_recv_data;

	pthread_create(&pthread_wait_for_master, NULL, wait_for_master, NULL);
	sleep(1);
	pthread_create(&pthread_wait_for_slave, NULL, wait_for_slave, NULL);

	while(1){
		if(Start_send){
			printf("Transmit start!\n");
			pthread_create(&pthread_start_recv_data, NULL, recv_data, NULL);
			break;
		}
		else{
			sleep(1);
		}
	}
	
	pthread_join(pthread_wait_for_slave,status);
	pthread_join(pthread_wait_for_master,status1);
	pthread_join(pthread_start_recv_data,status2);

	while(1);	
	close(listenfd);
	close(listenfd1);
	close(sfd);
	close(sfd1);
	return 0;	
}


