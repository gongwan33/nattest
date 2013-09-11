#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>

#include "info.h"


static struct sockaddr_in server_addr, master_addr, dst_addr, local_addr;
static struct ifreq ifr, *pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd,sfd,sockfd_udp;
static int port,sin_size;
static char  ip[4], buff[1024];
static char Accaunt_info[20];
static char verify_buff[5];
static int peer_ready = 0;
static long fsize;
static char * buff1;
static int create_thread = 1;
static int wait_peer = 1;

int local_net_init(){
	int on,ret1;

	memset(&master_addr, 0, sizeof(master_addr));
	master_addr.sin_family = AF_INET;
	master_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	master_addr.sin_port = htons(master_port);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return -1;
	}


	on = 1;			//Bind already in use
	ret1 = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

	if(bind(sockfd, (struct sockaddr *) (&master_addr),sizeof(master_addr)) == -1){
		printf ("Bind error: %s\a\n", strerror (errno));
		return -2;
	}

	return 0;
}
int dst_net_init_udp(){
	int ret1;
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(local_port);

	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.sin_family = AF_INET;
	//dst_addr.sin_addr.s_addr = inet_addr(server_ip);
	if( inet_pton(AF_INET, server_ip, &dst_addr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",server_ip);
		return -1;
	}	
	dst_addr.sin_port = htons(server_port3);
	bzero(&(dst_addr.sin_zero), 8);
	if((sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return -1;
	}
	if(bind(sockfd_udp, (struct sockaddr *) (&local_addr),sizeof(local_addr)) == -1){
		printf ("Bind error: %s\a\n", strerror (errno));
		return -2;
	}	
	return 0;
}


int get_local_ip_port(){
	int i = 0;
	sin_size = sizeof(master_addr);
	getsockname(sockfd, (struct sockaddr*)&master_addr, &sin_size);
	port = ntohs(master_addr.sin_port);

	memset(&ifr, 0, sizeof(ifr));
	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;

	if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0){
		printf("SIOCGIFCONF screwed up\n");
		return -1;
	}

	pifr = (struct ifreq *)(ifc.ifc_req);

	for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; pifr++){
		strcpy(ifr.ifr_name, pifr->ifr_name);    //eth0 eth1 ...

		if(strcmp(ACT_NETCARD, ifr.ifr_name) != 0)
			continue;

		if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0){
			printf("ip screwed up\n");
			return -2;
		}
		memcpy(ip, ifr.ifr_addr.sa_data+2, 4);
		//printf("%s\n",inet_ntoa(*(struct in_addr *)ip));
	}	

	printf("local ip is %s\n",inet_ntoa(*(struct in_addr *)ip));
	printf("local port is %d\n", port);

	return 0; 
}


int set_server_struct(char * ip1, int port){
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	
	if( inet_pton(AF_INET, ip1, &server_addr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",ip1);
		return -1;
	}

	return 0;
}

int Send_VUAP(){
	sprintf(Accaunt_info,"%s,%s",UNAME,PASSWD);
	printf("info = %s\n",Accaunt_info);
	if( connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}
	if( send(sockfd, Accaunt_info, strlen(Accaunt_info), 0) < 0)
	{
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	return 0;
}

int wait_for_callback(){
	int n;

	//	pthread_detach(pthread_self());


	n = recv(sockfd, verify_buff, 20, 0);
	if(n == -1)
		printf("recv error: %s(errno: %d)",strerror(errno),errno);
	else{
		verify_buff[n] = '\0';
		printf("verify =%d\n",verify_buff[0]); 
	}

	return verify_buff[0];
}

void * wait_for_peer(){
	int ret1,n;
	char peer_info[20];	

	n = recv(sockfd, peer_info, 20, 0);
	printf("n = %d\n",n);
	if(n == -1)
		printf("recv error: %s(errno: %d)",strerror(errno),errno);
	else{
		peer_info[n] = '\0';
		printf("peer_info = %d\n",peer_info[0]);
		if(peer_info[0] == 1){ 
			printf("Peer is ready!\n");
			peer_ready = 1;
		}
	}
}

FILE* Open_file(char * s){
	FILE *fp;
	fp = NULL;
	printf("File path:%s\n",s);
	if((fp = fopen(s,"r")) == NULL){
		printf("file open error\n");
		return NULL;
	}
	else 
		return fp;
}

void * transmit_data(){
	int ret = 0;
	int n,i;
	char *buff2 ="abcd";
	int sendbytes;
	while(wait_peer){
		if(peer_ready == 1){
			wait_peer =0;
			ret = dst_net_init_udp();
			if(ret < 0){
				printf("Local bind failed!!%d\n", ret);
			}
			sleep(1);
			//if((sendbytes = sendto(sockfd_udp, buff2, 20, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr))) < 0){
			//	printf("error:%s(%d)\n",strerror(errno),errno);
			n = fsize/1024 + 1;
			printf("n = %d\n",n);
			for(i = 0; i < n; i++){
				if(i != n-1){
					if((sendbytes = sendto(sockfd_udp, buff1+i*1024, 1024*sizeof(char), 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr))) < 0)
						printf("error:%s(%d)\n",strerror(errno),errno);
					else
						printf("bytes send:%d\n",sendbytes);
				}
				else
					if((sendbytes = sendto(sockfd_udp, buff1+i*1024, fsize - (n-1)*1024*sizeof(char), 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr))) < 0)
						printf("error:%s(%d)\n",strerror(errno),errno);
					else
						printf("bytes send:%d\n",sendbytes);
			}

		}
	}
	
}

int main(int argc, char* argv[]){
	int i,count = 0;
	int ret;
	void * status;
	void * status1;
	FILE * fp;
	fp = NULL;
	buff1 = NULL;	
	
	struct h264 * h_264;
	pthread_t pthread_wait_for_peer;
	pthread_t pthread_transmit_data;

	if(argc == 1){
		printf("Please type in a file name\n");
		return 0;
	}
	fp = Open_file(argv[1]);
	if(fp == NULL){
		return -1;
	}
	else{
		ret = fseek(fp, 0, SEEK_END);
		fsize = ftell(fp)+1;
		ret = fseek(fp, 0, SEEK_SET);
		printf("File size is %ld\n",fsize);
		buff1 = (char *)malloc(fsize * sizeof(char));
		count = fread(buff1, 1024, (fsize/1024+1),fp);
		printf("count = %d\n",count);
	}
	
	ret = local_net_init();
	if(ret < 0){
		printf("local bind fail!\n",ret);
		return ret;
	}
	
	if(ret != 0){
		printf("recv thread create error!\n");
	}
	
	//ret = get_local_ip_port();
	if(ret < 0){
		printf("get local ip & port fail!\n");
		return ret;
	}

	ret = set_server_struct(server_ip,server_port1);
	if(ret < 0){
		printf("set server ip fail!\n");
		return ret;
	}
	printf("------------------- Connection and user name verifying ---------------------\n");

	for(i = 0; i < 5; i++){
		ret = Send_VUAP();
		if(ret == 0){
			printf("send uname & passwd ok,wait for varify...\n");
			break;
		}
	}


	printf(".................................Wait for server's callback........................\n ");

	ret = wait_for_callback();
	if(ret < 0){
		printf("recv error!\n");
		return 0;
	}
	else if(ret == 0){
		printf("Verify fail!Wrong uname or passwd!\n");
	}
	else if(ret == 1){
		printf("Verify success!Wait for peer...\n");
		pthread_create(&pthread_wait_for_peer, NULL, wait_for_peer, NULL);
	}

	pthread_create(&pthread_transmit_data, NULL, transmit_data, NULL);

//	pthread_join(pthread_wait_for_peer,status);
//	pthread_join(pthread_transmit_data,status1);
	while(1);
	close(sockfd);

//	printf(".................................Wait for peer's connection........................\n ");
//	ret = pthread_create(&pthread_wait_for_peer, NULL, wait_for_peer, NULL);
	
//	while(1){
//		if(peer_ready == 1){
//			printf("peer is ready!Start to transmit data...\n");
//		}

//	}
	return 0;
}
