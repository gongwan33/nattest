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

#define KEEP_CONNECT_PACK 0
#define MAX_TRY 10
//#define server_ip_1 "192.168.1.216"
//#define server_ip_1 "192.168.1.4"
#define server_ip_1 "58.214.236.114"
#define server_ip_2 "192.168.1.116"

#define USERNAME "wang"
#define PASSWD "123456"

#define ACT_NETCARD "eth0"
#define server_port 61000
#define local_port 6788

static struct sockaddr_in servaddr1, local_addr, recv_sin, master_sin, host_sin;
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

void init_host_sin(char * str_ip, int p){
	memset(&host_sin, 0, sizeof(host_sin));
	host_sin.sin_family = AF_INET;
	host_sin.sin_addr.s_addr = inet_addr(str_ip);
	host_sin.sin_port = htons(p);
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

	init_host_sin(inet_ntoa(*(struct in_addr*)ip), port);

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

int Send_VUAPS(){
	char Sen_W;
	Sen_W = V_UAP_S;
	if(strlen(USERNAME) > 10 || strlen(PASSWD) > 10) return -1;

	ip_info[0] = Sen_W;
	memcpy(ip_info + 1, USERNAME, 10);
	memcpy(ip_info + 12, PASSWD, 10);
	memcpy(ip_info + 34, &host_sin, sizeof(struct sockaddr_in));

	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

void Send_IP_REQ(){
	char Sen_W;
	Sen_W = REQ_M_IP;
	sprintf(ip_info,"%c %s", Sen_W, USERNAME);
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_POL(char req,struct sockaddr_in * sock){
	ip_info[0] = req;
	if(req == POL_SENT) 
		memcpy(ip_info + 1, USERNAME, 10);
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)sock, sizeof(struct sockaddr_in));
}

void Send_CMD(char Ctls, char Res){
	ip_info[0] = Ctls;
	ip_info[1] = Res;
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_CMD_TO_MASTER(char Ctls, char Res){
	ip_info[0] = Ctls;
	ip_info[1] = Res;
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&master_sin, sizeof(struct sockaddr_in));
}

void *Keep_con(){
	pthread_detach(pthread_self());
	while(1){
		Send_CMD(KEEP_CON, 0x01);
		printf("Send KEEP_CON!\n");
		sleep(10);
	}
}

void clean_rec_buff(){
	char tmp[50];
	int ret;
	set_rec_timeout(100000, 0);//(usec, sec)
	while(ret > 0){
		ret = recvfrom(sockfd, tmp, 10, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		printf("Clean recv buff %d.\n", ret);
	}
	set_rec_timeout(0, 1);//(usec, sec)
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
	for(i = 0; i < MAX_TRY; i++){
		Send_VUAPS();
		printf("Send uname and passwd\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		char result;
		result = Ctl_Rec[1];

		if(Ctl_Rec[0] == GET_REQ){
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

	if(i >= MAX_TRY) return OUT_TRY;

#if	KEEP_CONNECT_PACK
	ret = pthread_create(&keep_connection, NULL, Keep_con, NULL);
	if (ret != 0)
		printf("can't create thread: %s\n", strerror(ret));
#endif

	clean_rec_buff();
	printf("------------------ Request master IP!-------------------\n");

	for(i = 0; i < MAX_TRY; i++){
		memset(Ctl_Rec, 0, 50);
		Send_IP_REQ();
		printf("Send IP_REQ.\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		Rec_W = Ctl_Rec[0];

		if(Rec_W == RESP_M_IP){
			printf("Receive ctl_w = %d\n", Rec_W);
			memcpy(&master_sin, Ctl_Rec + 1, sizeof(struct sockaddr_in));
			printf("Get master IP: %s\n", inet_ntoa(master_sin.sin_addr));
			break;
		}

	}

	if(i >= MAX_TRY) return OUT_TRY;
	
	clean_rec_buff();
	printf("------------------ Establish connection!-------------------\n");
	for(i = 0; i < 2; i++)
		Send_POL(POL_REQ, &master_sin);

	clean_rec_buff();
	for(i = 0; i < MAX_TRY; i++){
		memset(Ctl_Rec, 0, 50);
		Send_POL(POL_SENT, &servaddr1);
		printf("Send POL_SENT.\n");

		set_rec_timeout(0, 1);//(usec, sec)
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);

		//printf("OP = %d\n", Ctl_Rec[0]);

		if(Ctl_Rec[0] == GET_REQ){
			if(Ctl_Rec[1] == 0x0b){
				printf("Sever has got POL_SENT.\n");
				break;
			}
		}

	}

	if(i >= MAX_TRY) return OUT_TRY;

	while(1){
		set_rec_timeout(0, 0);//(usec, sec)
		memset(Ctl_Rec, 0, 50);
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		printf("OPCODE = %d\n", Ctl_Rec[0]);

		switch(Ctl_Rec[0]){
			case POL_REQ:
				Send_CMD_TO_MASTER(GET_REQ, 0x0a);
				printf("Receive GET_REQ\n");
				break;

			case S_POL_REQ:
			printf("Get M_POL_REQ from server.\n");
			for(i = 0; i < MAX_TRY; i++){
				memset(Ctl_Rec, 0, 50);
				Send_POL(POL_REQ, &master_sin);
				printf("Send POL_REQ to master.\n");

				set_rec_timeout(0, 1);//(usec, sec)
				recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
				if(Ctl_Rec[0] == GET_REQ && Ctl_Rec[1] == 0x0a){
					printf("Pole ok! Connection established.\n");
					Send_CMD(GET_REQ, 0x10);
					break;
				}
			}

			if(i >= MAX_TRY){
				Send_CMD(GET_REQ, 0x11);
				printf("Pole failed! Requiring slave mode.");
			}

			break;	

		}
	}

	close(sockfd);
	return 0;
}
