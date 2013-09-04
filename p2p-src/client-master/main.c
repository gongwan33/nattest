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

#define server_ip_1 "192.168.1.216"
#define server_ip_2 "192.168.1.116"

#define USERNAME "wang"
#define PASSWD "123456"

#define ACT_NETCARD "eth1"
#define server_port 61000
#define local_port 6888

static struct sockaddr_in servaddr1,servaddr2,local_addr;
static struct ifreq ifr,*pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd;
static int port, sin_size;
static char mac[6], ip[4], buff[1024];


int local_net_init(){
	memset(&local_addr, 0, sizeof local_addr);
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(local_port);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return -1;
	}

	if(bind(sockfd, (struct sockaddr *) (&local_addr),sizeof(local_addr)) == -1){
		printf ("Bind error: %s\a\n", strerror (errno));
		return -2;
	}

	return 0;
}

int get_local_ip_port(){
	int i = 0;
	sin_size = sizeof(local_addr);
	getsockname(sockfd, (struct sockaddr*)&local_addr, &sin_size);
	port = ntohs(local_addr.sin_port);

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

int set_ip1_struct(char * ip1, int port){
	memset(&servaddr1, 0, sizeof(servaddr1));
	servaddr1.sin_family = AF_INET;
	servaddr1.sin_port = htons(port);
	
	if( inet_pton(AF_INET, ip1, &servaddr1.sin_addr) <= 0){
		printf("inet_pton error for %s\n",ip1);
		return -1;
	}

	return 0;
}

int main(){
	int  i;
	int ret = 0;
	
	ret = local_net_init();
	if(ret < 0){
		printf("Local bind failed!!%d\n", ret);
		return ret;
	}

	ret = get_local_ip_port();
	if(ret < 0){
		printf("Geting local ip and port failed!!%d\n", ret);
		return ret;
	}

	ret = set_ip1_struct(server_ip_1, server_port);
	if(ret < 0){
		printf("Set ip1 failed!!%d\n", ret);
		return ret;
	}

	printf("------------------- Connection and user name verifying ---------------------\n");
	for(i = 0; i < 10; i++){

		sprintf(ip_info,"%s %s", USERNAME, PASSWD);
		printf("send %s\n", ip_info);

		sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1,sizeof(servaddr1)); 

		usleep(1000000);
	}

	/*
	   for(j = 0; j < 10; j++){

	   sprintf(ip_info,"#1 %s [%d]", inet_ntoa(*(struct in_addr *)ip), port);
	   printf("send %s\n", ip_info);

	   for(i = 0; i < count; i++){
	   sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1,sizeof(servaddr1)); 
	   }

	   usleep(1000000);
	   }
	   */

	close(sockfd);
	exit(0);	
	return 0;
}
