/*AUTHOER:WANGGONG, CHINA
 *VERSION:1.0
 *FUNCTION:MASTER
 */
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
#include <commonkey.h>
#include <ring.h>

#define MAX_TRY 10
#define SEND_BUFF_SIZE 1024*3
#define MAX_RECEIVE 1024*1024
#define MAX_RECV_BUF 1024*1024*10

//#define server_ip_1 "192.168.1.216"
#define server_ip_1 "192.168.40.131"
//#define server_ip_1 "192.168.1.4"
//#define server_ip_1 "58.214.236.114"

#define USERNAME "wang"
#define PASSWD "123456"

#define ACT_NETCARD "eth0"
#define server_port 61000
#define local_port 6888

static char recvSign;
static struct sockaddr_in servaddr1, local_addr, recv_sin, slave_sin, host_sin;
static struct ifreq ifr, *pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd;
static int port, sin_size, recv_sin_len;
static char mac[6], ip[4], buff[1024];
static pthread_t keep_connection;
static char pole_res;
static unsigned int sendIndex;
static unsigned int getNum;
static unsigned int sendNum;
static char* recvBuf;
static pthread_t recvDat_id;
static unsigned int recvBufP;

int commonKey = 0;

static unsigned char connectionStatus = FAIL;

int local_net_init(int port){
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(port);

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

void init_host_sin(char * str_ip, int p){
	memset(&host_sin, 0, sizeof(host_sin));
	host_sin.sin_family = AF_INET;
	host_sin.sin_addr.s_addr = inet_addr(str_ip);
	host_sin.sin_port = htons(p);
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

void set_rec_timeout(int usec, int sec){
	struct timeval tv_out;
    tv_out.tv_sec = sec;
    tv_out.tv_usec = usec;

	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
}

int Send_VUAP(){
	char Sen_W;
	Sen_W = V_UAP;
	if(strlen(USERNAME) > 10 || strlen(PASSWD) > 10) return -1;

	ip_info[0] = Sen_W;
	memcpy(ip_info + 1, USERNAME, 10);
	memcpy(ip_info + 12, PASSWD, 10);
	memcpy(ip_info + 34, &host_sin, sizeof(struct sockaddr_in));

	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

int Send_CLOSE(){
	char Sen_W;
	Sen_W = MASTER_QUIT;
	if(strlen(USERNAME) > 10 || strlen(PASSWD) > 10) return -1;

	ip_info[0] = Sen_W;
	memcpy(ip_info + 1, USERNAME, 10);
	memcpy(ip_info + 12, PASSWD, 10);
	memcpy(ip_info + 34, &host_sin, sizeof(struct sockaddr_in));

	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

void Send_POL(char req,struct sockaddr_in * sock){
	ip_info[0] = req;
	sendto(sockfd, ip_info, 2, 0, (struct sockaddr *)sock, sizeof(struct sockaddr_in));
}

void Send_CMD(char Ctls, char Res){
	ip_info[0] = Ctls;
	ip_info[1] = Res;
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_CMD_TO_SLAVE(char Ctls, char Res){
	ip_info[0] = Ctls;
	ip_info[1] = Res;
	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&slave_sin, sizeof(struct sockaddr_in));
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

void Send_Turn_Dat(char *data, unsigned int len, char priority){
	char send_buff[SEND_BUFF_SIZE];
	send_buff[0] = TURN_REQ;
	
	send_buff[1] = (len >>  8);
	send_buff[2] = (len & 0xff);

	send_buff[3] = priority;

	memcpy(send_buff + 4, data, len);

	sendto(sockfd, send_buff, len + 4, 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));	
}

void sendGet(unsigned int index)
{
	char buf[7] = {'G','E','T'};
	buf[3] = index & 0xff;
	buf[4] = (index>>8) & 0xff;
	buf[5] = (index>>16) & 0xff;
	buf[6] = (index>>24) & 0xff;

    if(connectionStatus == P2P)
	{
	    sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&slave_sin, sizeof(struct sockaddr_in));
	}
	else if(connectionStatus == TURN)
	{
	    sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	}
}

void* recvData(void *argc)
{
	int recvLen = 0;
	pthread_detach(pthread_self());

	while(recvSign)
	{
		recvLen = 0;
		if(connectionStatus == P2P)
		{
			recvLen = recvfrom(sockfd, recvBuf + recvBufP, MAX_RECEIVE, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		}
		else if(connectionStatus == TURN)
		{
			recvLen = recvfrom(sockfd, recvBuf + recvBufP, MAX_RECEIVE, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		}
		else 
			break;

		if(recvBuf[recvBufP] == 'J' && recvBuf[recvBufP + 1] == 'E' && recvBuf[recvBufP + 2] == 'A' && recvBuf[recvBufP + 3] == 'N')
		{
			int indexGet = 0;
			indexGet = (indexGet | recvBuf[recvBufP + 4] | recvBuf[recvBufP + 5]<<8 | recvBuf[recvBufP + 6]<<16 | recvBuf[recvBufP + 7]<<24);
			printf("get %d\n", indexGet);
			sendGet(indexGet);
		}
		else if(recvBuf[recvBufP] == 'G' && recvBuf[recvBufP + 1] == 'E' && recvBuf[recvBufP + 2] == 'T')
		{
			int indexGet = 0;
			indexGet = (indexGet | recvBuf[recvBufP + 3] | recvBuf[recvBufP + 4]<<8 | recvBuf[recvBufP + 5]<<16 | recvBuf[recvBufP + 6]<<24);
			unreg_buff(indexGet);
		}


		recvBufP += recvLen;
		if(recvBufP > MAX_RECV_BUF)
			recvBufP = 0;

	    getNum += recvLen;
	}

}

int JEAN_init_master(int serverPort, int localPort, char *setIp)
{
	int  i;
	char Ctl_Rec[50];
	char Rec_W;
	int ret = 0;
	char Pole_ret = -1;
    
	recvSign = 1;
    recvBuf = (char*)malloc(MAX_RECV_BUF);

    initRing();	
	ret = local_net_init(localPort);
	if(ret < 0){
		printf("Local bind failed!!%d\n", ret);
		return ret;
	}

	ret = get_local_ip_port();
	if(ret < 0){
		printf("Geting local ip and port failed!!%d\n", ret);
		return ret;
	}

	ret = set_ip1_struct(setIp, serverPort);
	if(ret < 0){
		printf("Set ip1 failed!!%d\n", ret);
		return ret;
	}

	set_rec_timeout(0, 1);//(usec, sec)

	printf("------------------- Connection and user name verifying ---------------------\n");
	for(i = 0; i < MAX_TRY; i++){
		Send_VUAP();
		printf("Send uname and passwd\n");

		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		char result;
		result = Ctl_Rec[1];

		if(Ctl_Rec[0] == V_RESP){
			printf("Receive ctl_w = %d result = %d\n", Rec_W, result);
			if(result == 1){
				printf("Verify and regist success!\n");
				break;
			}
			else if(result == 3){
				printf("Verify success but regist failed. Maybe node already exists!\n");
				break;
			}
			else if(result == 2){ 
				printf("Verify failed!\n");
				return WRONG_VERIFY;
			}
		}
	}

	if(i >= MAX_TRY) return OUT_TRY;
	
	ret = pthread_create(&keep_connection, NULL, Keep_con, NULL);
	if (ret != 0)
		printf("can't create thread: %s\n", strerror(ret));

	clean_rec_buff();
	printf("------------------ Wait for slave IP!-------------------\n");

	int slaver_act = 0;
	while(!slaver_act){
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		Rec_W = Ctl_Rec[0];

		if(Rec_W == S_IP){
			slaver_act = 1;
			memset(&slave_sin, 0, sizeof(slave_sin));
			memcpy(&slave_sin, Ctl_Rec + 1, sizeof(struct sockaddr_in));
			printf("Get slave IP info! Slave IP is %s\n", inet_ntoa(slave_sin.sin_addr));

			for(i = 0; i < MAX_TRY + 1 ; i++){
				Send_CMD(GET_REQ, 0x08);
				char result = 0;

				recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
				if(Ctl_Rec[0] == GET_REQ && Ctl_Rec[1] == 0x9) break;
			}

			if(i >= MAX_TRY + 1) return OUT_TRY;

		}
	}

	clean_rec_buff();
	printf("------------------ Wait for slave to establish connection!-------------------\n");
	while(Pole_ret == -1){
		memset(Ctl_Rec, 0, 50);

		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		Rec_W = Ctl_Rec[0];

		switch(Rec_W){
			case POL_REQ:
				printf("Get pole request!\n");
				Send_CMD_TO_SLAVE(GET_REQ, 0x0a);
			break;

			case CON_ESTAB:
				pole_res = Ctl_Rec[1];
				commonKey = Ctl_Rec[2] || (Ctl_Rec[3]<<8) || (Ctl_Rec[4]<<16);
				Send_CMD(GET_REQ, 0x14);
				printf("Pole result = %d, key = %d.\n", pole_res, commonKey);
				Pole_ret = pole_res;
				if(Pole_ret == 1)
					connectionStatus = P2P;
				else
					connectionStatus = TURN;

				pthread_create(&recvDat_id, NULL, recvData, NULL);
				break;

			case M_POL_REQ:
			Send_CMD(GET_REQ, 0x12);
			printf("Get M_POL_REQ from server.\n");
			for(i = 0; i < MAX_TRY; i++){
				memset(Ctl_Rec, 0, 50);
				Send_POL(POL_REQ, &slave_sin);
				printf("Send POL_REQ to slave.\n");
				recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
				if(Ctl_Rec[0] == GET_REQ && Ctl_Rec[1] == 0x0a){
					printf("Pole ok! Connection established.\n");
					Send_CMD(GET_REQ, 0x0e);
					break;
				}
			}
			if(i >= MAX_TRY){
				Send_CMD(GET_REQ, 0x0f);
				printf("Pole failed! Requiring slave mode.\n");
			}
			break;	

		}
	}

}

int JEAN_send_master(char *data, int len, unsigned char priority, unsigned char video_analyse)
{
	int sendLen = 0;
    char *buffer;
	struct load_head lHead;

	buffer = (char *)malloc(len + sizeof(struct load_head));
	memcpy(lHead.logo, "JEAN", 4);
	lHead.index = sendIndex;
	lHead.get_number = getNum;
	lHead.priority = priority;
	lHead.length = len;

	memcpy(buffer, &lHead, sizeof(lHead));
	memcpy(buffer + sizeof(lHead), data, len);
    if(connectionStatus == P2P)
	{
	    sendLen = sendto(sockfd, buffer, len + sizeof(lHead), 0, (struct sockaddr *)&slave_sin, sizeof(struct sockaddr_in));
	}
	else if(connectionStatus == TURN)
	{
	    sendLen = sendto(sockfd, buffer, len + sizeof(lHead), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	}
	else 
	{
		return NO_CONNECTION; 
	}

	reg_buff(sendIndex, buffer);
	sendIndex++;
    sendNum += sendLen;

    return sendLen;
}

int JEAN_recv_master(char *data, int len, unsigned char priority, unsigned char video_analyse)
{
	int recvLen = 0;

	if(recvBufP < len)
	{
		memcpy(data, recvBuf, recvBufP);
		recvLen = recvBufP;
		recvBufP = 0;
	}
	else
	{
		memcpy(data, recvBuf, len);
		recvLen = len;
        memcpy(recvBuf, recvBuf + len, recvBufP - len);
	}


    return recvLen;
}

int JEAN_close_master()
{
	int i = 0;
	char Ctl_Rec[50];

	recvSign = 0;
	clean_rec_buff();
	sleep(1);
	for(i = 0; i < MAX_TRY + 1 ; i++){
		printf("send close \n");
		Send_CLOSE();
		char result = 0;

		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		if(Ctl_Rec[0] == GET_REQ) 
			break;
	}

	if(i >= MAX_TRY + 1) return OUT_TRY;

	free(recvBuf);
	emptyRing();
	close(sockfd);
	return 0;
}

int main(){
    int ret = -1;
	char data[10] = "test";

	ret = JEAN_init_master(server_port, local_port, server_ip_1);
	if(ret < 0)
		return ret;

    JEAN_send_master(data, sizeof(data), 1, 0);
	printRingStatus();
	sleep(1);
    JEAN_send_master(data, sizeof(data), 1, 0);
	printRingStatus();

    JEAN_close_master();
	return 0;
}
