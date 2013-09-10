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
#include <JEANP2PPRO.h>

#define server_ip_1 "192.168.1.216"
#define server_ip_2 "192.168.1.116"

#define USERNAME "wang"
#define PASSWD "123456"

#define ACT_NETCARD "eth0"
#define server_port 61000
#define local_port 6788

static struct sockaddr_in servaddr1, servaddr2, local_addr, recv_sin;
static struct ifreq ifr, *pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd;
static int port, sin_size, recv_sin_len;
static char mac[6], ip[4], buff[1024];
static pthread_t keep_connection;


int local_net_init(){
	memset(&local_addr, 0, sizeof(local_addr));
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

void init_recv_sin(){
	memset(&recv_sin, 0, sizeof(recv_sin));
	recv_sin.sin_family = AF_INET;
	recv_sin.sin_addr.s_addr = inet_addr("1.1.1.1");
	recv_sin.sin_port = htons(10000);
	recv_sin_len = sizeof(recv_sin);
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

int set_rec_timeout(int usec, int sec){
	struct timeval tv_out;
    tv_out.tv_sec = sec;
    tv_out.tv_usec = usec;

	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
}

void Send_VUAPS(){
	char Sen_W;
	Sen_W = V_UAP_S;
	sprintf(ip_info,"%c %s %s", Sen_W, USERNAME, PASSWD);
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_CMD(char Ctls, char Res){
	char Sen_W;
	Sen_W = Ctls;
	sprintf(ip_info,"%c %c ", Sen_W, Res);
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void *Keep_con(){
	pthread_detach(pthread_self());
	while(1){
		Send_CMD(KEEP_CON, 0x01);
		printf("Send KEEP_CON!\n");
		sleep(10);
	}
}

int main(){
	int  i;
	char Ctl_Rec[50];
	char Rec_W;
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
		Send_VUAPS();
		printf("Send uname and passwd\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		char result;
		sscanf(Ctl_Rec, "%c %c", &Rec_W, &result);

		if(Rec_W == GET_REQ){
			printf("Receive ctl_w = %d result = %d\n", Rec_W, result);
			if(result == 4){
				printf("Verify and find node success!\n");
				break;
			}
			else if(result == 6){
				printf("Verify success but find node failed. Maybe node not exists!\n");
				return NO_NODE;
			}
			else if(result == 5){ 
				printf("Verify failed!\n");
				return WRONG_VERIFY;
			}
		}
	}

	if(i >= 10) return OUT_TRY;
	
	ret = pthread_create(&keep_connection, NULL, Keep_con, NULL);
	if (ret != 0)
		printf("can't create thread: %s\n", strerror(ret));

	printf("------------------ Establish connection!-------------------\n");


	
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
	while(1);
	close(sockfd);
	return 0;
}
