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

#define ACT_NETCARD "eth0"
//#define server_ip "58.214.236.114"
#define server_ip "192.168.1.102"
#define master_ip "192.168.1.102"
#define UNAME "zhoujie"
#define PASSWD "123456"
#define server_port1 61000
#define server_port2 61001
#define master_port 1000

static struct sockaddr_in server_addr, master_addr;
static struct ifreq ifr, *pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd,sfd;
static int port,sin_size;
static char  ip[4], buff[1024];
static char Accaunt_info[20];
static char verify_buff[5];

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
	if( listen(sockfd, 10) == -1){
                printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
                return -1;
        }

        printf("======waiting for server's callback======\n");
        while(1){
                if( (sfd = accept(sockfd, (struct sockaddr*)NULL, NULL)) == -1){
                        printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                        return -1;
                }
		
	
                n = recv(sfd, verify_buff, 20, 0);
                if(n == -1)
                        printf("recv error: %s(errno: %d)",strerror(errno),errno);
              	else{
			verify_buff[n] = '\0';
			printf("verify =%d\n",verify_buff[0]); 
		}

		return verify_buff[0];
		}
}

int main(){
	int i;
	int ret,pthread_id;
	pthread_t recv_thread;

	ret = local_net_init();
	if(ret < 0){
		printf("local bind fail!\n",ret);
		return ret;
	}
	
//	ret = pthread_create(&recv_thread, NULL, wait_for_callback, NULL);
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

	close(sockfd);

	ret = local_net_init();
	if(ret < 0){
		printf("local bind fail!\n",ret);
		return ret;
	}

	ret = wait_for_callback();
	if(ret < 0){
		printf("recv error!\n");
		return 0;
	}
	else if(ret == 0){
		printf("Verify fail!Wrong uname or passwd!\n");
	}
	else if(ret == 1){
		printf("Verify success! Start to transmit data...\n");
	}

	close(sockfd);
	return 0;
}
