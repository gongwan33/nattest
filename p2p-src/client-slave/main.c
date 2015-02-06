/*AUTHOR:WANGGONG, CHINA
 *VERSION:1.0
 *FUNCTION:SLAVE
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
#include <DSet.h>

#define MAX_TRY 10
#define SEND_BUFF_SIZE 1024*3
#define MAX_RECEIVE 1024*1024
#define MAX_RECV_BUF 1024*1024*5

#define TURN_DATA_SIZE 1024*3
#define KEEP_CONNECT_PACK 0
//#define server_ip_1 "192.168.1.216"
//#define server_ip_1 "192.168.1.114"
#define server_ip_1 "192.168.1.109"
//#define server_ip_1 "192.168.1.4"
//#define server_ip_1 "58.214.236.114"

#define USERNAME "wang"
#define PASSWD "123456"

#define ACT_NETCARD "eth0"
#define server_port 61000
#define server_turn_port 61001
#define server_cmd_port 61002
#define local_port 6788

static char recvSign;
static struct sockaddr_in servaddr1, local_addr, recv_sin, master_sin, host_sin, turnaddr;
static struct ifreq ifr, *pifr;
static struct ifconf ifc;
static char ip_info[50];
static int sockfd;
static int cmdfd;
static int port, sin_size, recv_sin_len;
static char mac[6], ip[4], buff[1024];
static pthread_t keep_connection;
static char pole_res;
static unsigned int sendIndex;
static unsigned int getNum;
static unsigned int sendNum;
static char* recvBuf;
static char* recvProcessBuf;
static char* recvProcessBackBuf;
static pthread_t recvDat_id;
static pthread_mutex_t recvBuf_lock;
static unsigned int recvBufP;
static unsigned int recvProcessBufP;
static unsigned int recvProcessBackBufP;
static char recvThreadRunning = 0;

unsigned char connectionStatus = FAIL;

int JEAN_recv_timeout = 1000;//1s
int commonKey = 0;

int local_net_init(int localPort){
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(localPort);

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

	memset(&turnaddr, 0, sizeof(turnaddr));
	turnaddr.sin_family = AF_INET;
	turnaddr.sin_port = htons(server_turn_port);
	
	if( inet_pton(AF_INET, ip1, &turnaddr.sin_addr) <= 0){
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

int Send_TURN(){
	char Sen_W;
	Sen_W = TURN_REQ;
	if(strlen(USERNAME) > 10 || strlen(PASSWD) > 10) return -1;

	ip_info[0] = Sen_W;
	memcpy(ip_info + 1, USERNAME, 10);
	memcpy(ip_info + 12, PASSWD, 10);
	memcpy(ip_info + 34, &host_sin, sizeof(struct sockaddr_in));

	sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

int Send_CMDOPEN(){
	struct p2p_head head;
	memcpy(&head.logo, "CMD", 3);
	head.data[0] = 'S';

	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

int Send_VUAPS(){
	struct p2p_head head;
	memcpy(&head.logo, "SUP", 3);
	if(strlen(USERNAME) > 10 || strlen(PASSWD) > 10) 
		return -1;

	memcpy(head.data, USERNAME, 10);
	memcpy(head.data + 10, PASSWD, 10);
	memcpy(head.data + 20, &host_sin, sizeof(struct sockaddr_in));

	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
	return 0;
}

void Send_IP_REQ(){
	struct p2p_head head;
	memcpy(&head.logo, "MIP", 3);
	memcpy(head.data, USERNAME, 10);
	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
}

void Send_POL(char req,struct sockaddr_in * sock){
	struct p2p_head head;
	if(req == POL_SENT)
	{
		memcpy(&head.logo, "POL", 3);
		memcpy(head.data, USERNAME, 10);
		sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)sock, sizeof(struct sockaddr_in));
		return;
	}
	else
	{
		ip_info[0] = req;
		sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)sock, sizeof(struct sockaddr_in));
		return;
	}
}

void Send_CMD(char Ctls, char Res){
	struct p2p_head head;
	if(Ctls == GET_REQ)
		memcpy(head.logo, "GRQ", 3);
	head.data[0] = Res;
	sendto(sockfd, &head, sizeof(struct p2p_head), 0, (struct sockaddr *)&servaddr1, sizeof(servaddr1));
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

void sendGet(unsigned int index)
{
	struct get_head getSt;

	memcpy(&getSt, "GET", 3);

#if TEST_LOST
	int rnd = 0;
	int lost_emt = LOST_PERCENT;
	rnd = rand()%100;
	if(rnd <= lost_emt)
	    printf("Lost!!: rnd = %d, lost_emt = %d, lost_posibility = %d percent\n", rnd, lost_emt, lost_emt);

	if(rnd > lost_emt)
	{
#endif

	if(connectionStatus == P2P)
	{
		sendto(sockfd, &getSt, sizeof(struct get_head), 0, (struct sockaddr *)&master_sin, sizeof(struct sockaddr_in));
	}
	else if(connectionStatus == TURN)
	{
	    sendto(sockfd, &getSt, sizeof(struct get_head), 0, (struct sockaddr *)&turnaddr, sizeof(turnaddr));
	}
#if TEST_LOST
	}
#endif

}

int findIndexInBuf(char *buf, int *start, int *end, int *datLen, u_int32_t index)
{
	int scanP = 0;
	struct load_head head;

	while(scanP + sizeof(struct load_head) < *datLen)
	{
		if(buf[scanP] == 'J' && buf[scanP + 1] == 'E' && buf[scanP + 2] == 'A' && buf[scanP + 3] == 'N')
		{
			memcpy(&head, buf + scanP, sizeof(struct load_head));
//			printf("BackBuf: index %d", head.index);
			if(index == head.index)
			{
				*start = scanP + sizeof(struct load_head);
				*end = scanP + head.length + sizeof(struct load_head);
				memcpy(buf + scanP, buf + scanP + head.length + sizeof(struct load_head), *datLen - scanP - head.length - sizeof(struct load_head));
				*datLen -= (sizeof(struct load_head) + head.length);
				return head.priority;
			}

			if(*datLen - scanP - sizeof(struct load_head) >= head.length)
				scanP = scanP + sizeof(struct load_head) + head.length;
			else
				break;
		}
		else
			scanP++;
	}

	return -1;

}

int init_CMD_CHAN()
{
    struct sockaddr_in pin;

	bzero(&pin,sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = inet_addr(server_ip_1);
	pin.sin_port = htons(server_cmd_port);

	if((cmdfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error opening socket \n");
		return -1;
	}

	if(connect(cmdfd, (void *)&pin, sizeof(pin)) == -1)
	{
		printf("Error connecting to socket \n");
		return -1;
	}

	return 0;
}

int close_CMD_CHAN()
{
	close(cmdfd);
}

int send_cmd(char *data, int len)
{
	int sendLen = 0;
	if(len < 0)
		return -1;
	sendLen = send(cmdfd, data, len, 0);
	if(sendLen == -1)
	{
		printf("Error in send\n");
		return -1;
	}

	return sendLen;
}

int recv_cmd(char *data, int len)
{
	int recvLen = 0;
	recvLen = recv(cmdfd, data, len, 0);
	if(recvLen == -1)
	{
		printf("Error in recv\n");
		return -1;
	}

	return recvLen;
}

void* recvData(void *argc)
{
	recvThreadRunning = 1;

	int recvLen = 0;
	int recv_size = 0;
	int err = 0;
	socklen_t optlen = 0;
	char *retryData;
	u_int32_t lastIndex = 0;
	int pauseSign = 0;
	int recordLostNum = 0;
	int getLostNum = 0;

	pthread_detach(pthread_self());

	recvBufP = 0;
	recvProcessBufP = 0;

	optlen = sizeof(recv_size); 
	err = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen); 
	if(err < 0)
	{ 
		printf("Fail to get recbuf size\n"); 
	} 

	set_rec_timeout(1, 0);//(usec, sec)
	while(recvSign)
	{
		recvLen = 0;
		if(connectionStatus == P2P)
		{
			recvLen = recvfrom(sockfd, recvProcessBuf + recvProcessBufP, MAX_RECEIVE, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		}
		else if(connectionStatus == TURN)
		{
			recvLen = recvfrom(sockfd, recvProcessBuf + recvProcessBufP, MAX_RECEIVE, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		}
		else 
			break;

		if(recvLen <= 0)
		{
//			usleep(100);
			continue;
		}

	    getNum += recvLen;
		recvProcessBufP += recvLen;

		if(recvProcessBufP > MAX_RECV_BUF - recv_size)
		{
			printf("recvBuf overflow!!\n");
			recvProcessBufP = 0;
		}

		if(pauseSign == 0)
		{
			recordLostNum = 0;
			getLostNum = 0;
		}

		if(getLostNum >= recordLostNum - 1)
			pauseSign = 0;

//		printf("buf back point at %d, pauseSign = %d\n", recvProcessBackBufP, pauseSign);
		if(pauseSign == 0 && recvProcessBackBufP >= 0)
		{
#if PRINT
			printf("pause status out!\n");
#endif
			memcpy(recvProcessBuf + recvProcessBufP, recvProcessBackBuf, recvProcessBackBufP);
			recvProcessBackBufP = 0;
		}

	    if(recvProcessBufP >= sizeof(struct load_head))
		{
			int scanP = 0;
			struct load_head head;
			struct get_head get;

			while(scanP + sizeof(struct load_head) < recvProcessBufP)
			{
				if(recvProcessBuf[scanP] == 'J' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'A' && recvProcessBuf[scanP + 3] == 'N')
				{
					int lostNum = 0;
					memcpy(&head, recvProcessBuf + scanP, sizeof(struct load_head));
					lostNum = head.index - lastIndex;
//					printf("lastindex %d index %d \n", lastIndex, head.index);

					if(pauseSign == 1 && lostNum == 1)
						getLostNum++;

					if(lostNum <= 0 && head.index != 0)
					{
						scanP += (head.length + sizeof(struct load_head));
						continue;
					}

					if(head.index != 0 && lostNum > 1)
					{
						if(recvProcessBackBufP != 0)
						{
						    int start, end;
						    int pri;
                            pri = findIndexInBuf(recvProcessBackBuf, &start, &end, &recvProcessBackBufP, lastIndex + 1);
					//		printf("found pack in back %d\n", pri);
						    if(pri >= 0)
						    {
#if PRINT
					            printf("load head: J %d\n", lastIndex + 1);
#endif
								if(pri > 0)
								    sendGet(lastIndex + 1);
								lastIndex = lastIndex + 1;
								if(end - start > 0)
									memcpy(recvBuf+recvBufP, recvProcessBackBuf + start, end - start);
								else
								{
#if PRINT
									printf("Empty load!Actually lost.\n");
#endif
								}

								break;
						    }
						}
#if PRINT
						printf("lost pack happen, reget!!\n");
#endif
                        int i = 0;
						pauseSign = 1;
						for(i = 1;i < lostNum; i++)
						{
						}

						if(recvProcessBackBufP + recvProcessBufP - scanP < MAX_RECV_BUF)
						{
						    memcpy(recvProcessBackBuf + recvProcessBackBufP, recvProcessBuf + scanP, recvProcessBufP - scanP);
						    recvProcessBackBufP = recvProcessBackBufP + (recvProcessBufP - scanP);
						    recvProcessBufP = 0;
						}
						else
						{
							printf("back buf overflow!!\n");
							recvProcessBackBufP = 0;
						}
	
						if(lostNum > recordLostNum)
							recordLostNum = lostNum;

						break;
					}

					lastIndex = head.index;

#if PRINT
					printf("load head: %c %d %d %d %d\n", head.logo[0], head.index, head.get_number, head.priority, (unsigned int)head.length);
#endif
					if(recvProcessBufP - scanP - sizeof(struct load_head) >= head.length)
					{
						if(head.priority > 0)
							sendGet(head.index);
				
					    if(head.length == 0)
						{
#if PRINT
							printf("Empty load!Actually lost.\n");
#endif
						}

						if(recvBufP + head.length > MAX_RECV_BUF)
						{
							printf("recv processed buf overflow!!\n");
							recvBufP = 0;
						}
						pthread_mutex_lock(&recvBuf_lock);
						memcpy(recvBuf + recvBufP, recvProcessBuf + scanP + sizeof(struct load_head), head.length);
						recvBufP += head.length;
						pthread_mutex_unlock(&recvBuf_lock);
						scanP = scanP + sizeof(struct load_head) + head.length;
					}
					else
						break;
				}
				else if(recvProcessBuf[scanP] == 'G' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'T')
				{
					memcpy(&get, recvProcessBuf + scanP, sizeof(struct get_head));
#if PRINT
					printf("get index: %d\n", get.index);
#endif
					scanP = scanP + sizeof(struct get_head);
				}
				else
					scanP++;
			}

			if(scanP == recvProcessBufP)
			{
				recvProcessBufP = 0;
				continue;
			}
			else if(scanP < recvProcessBufP)
			{
				recvProcessBufP -= scanP;
				memcpy(recvProcessBuf, recvProcessBuf + scanP, recvProcessBufP);
			}
		}
		else if(recvProcessBufP >= sizeof(struct get_head))
		{
			int scanP = 0;
			struct get_head get;

			while(scanP + sizeof(struct get_head) <= recvProcessBufP)
			{
				if(recvProcessBuf[scanP] == 'G' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'T')
				{
					memcpy(&get, recvProcessBuf + scanP, sizeof(struct get_head));
//					unreg_buff(get.index);
					scanP = scanP + sizeof(struct get_head);
				}
				else
					scanP++;
			}

			if(scanP == recvProcessBufP)
			{
				recvProcessBufP = 0;
				continue;
			}
			else if(scanP < recvProcessBufP)
			{
				recvProcessBufP -= scanP;
				memcpy(recvProcessBuf, recvProcessBuf + scanP, recvProcessBufP);
			}

		}

//		usleep(100);
	}

	recvThreadRunning = 0;

}

int JEAN_init_slave(int setServerPort, int setLocalPort, char *setIp)
{
	int ret = 0;	
	int  i;
	char Ctl_Rec[50];
	char Rec_W;
	char Pole_ret = -1;

	recvSign = 1;
    recvBuf = (char*)malloc(MAX_RECV_BUF);
    recvProcessBuf = (char*)malloc(MAX_RECV_BUF);
    recvProcessBackBuf = (char*)malloc(MAX_RECV_BUF);

    initRing();	
	
    if (pthread_mutex_init(&recvBuf_lock, NULL) != 0) 
	{
		printf("mutex init error\n");
		return -1;
	}

	ret = local_net_init(setLocalPort);
	if(ret < 0){
		printf("Local bind failed!!%d\n", ret);
		return ret;
	}

	ret = get_local_ip_port();
	if(ret < 0){
		printf("Geting local ip and port failed!!%d\n", ret);
		return ret;
	}

	ret = set_ip1_struct(setIp, setServerPort);
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

	while(Pole_ret == -1){
		set_rec_timeout(0, 0);//(usec, sec)
		memset(Ctl_Rec, 0, 50);
		recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		printf("OPCODE = %d\n", Ctl_Rec[0]);

		switch(Ctl_Rec[0]){
			case POL_REQ:
				Send_CMD_TO_MASTER(GET_REQ, 0x0a);
				printf("Receive GET_REQ\n");
				break;

			case CON_ESTAB:
				pole_res = Ctl_Rec[1];
//				commonKey = (Ctl_Rec[2] | (Ctl_Rec[3]<<8) | (Ctl_Rec[4]<<16) | (Ctl_Rec[5]<<24));
				Send_CMD(GET_REQ, 0x14);
//				printf("Pole result = %d, key = 0x%x.\n", pole_res, commonKey);
				Pole_ret = pole_res;
#ifndef TEST_TURN
				if(Pole_ret == 1)
					connectionStatus = P2P;
				else
#endif
				{
					int i = 0;
					connectionStatus = TURN;
					for(i = 0; i < MAX_TRY + 1 ; i++){
						printf("send turn \n");
						Send_TURN();
						char result = 0;

						recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
						if(Ctl_Rec[0] == GET_REQ) 
							break;
					}

					if(i >= MAX_TRY + 1) return OUT_TRY;

				}

				clean_rec_buff();
				set_rec_timeout(0, 1);//(usec, sec)
				for(i = 0; i < MAX_TRY + 1 ; i++){
					sleep(1);
					printf("require cmd channel open \n");
					Send_CMDOPEN();
					char result = 0;

					recvfrom(sockfd, Ctl_Rec, sizeof(Ctl_Rec), 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
					if(Ctl_Rec[0] == GET_REQ) 
						break;
				}

				if(i >= MAX_TRY + 1) return OUT_TRY;

				if(recvThreadRunning == 0)
					pthread_create(&recvDat_id, NULL, recvData, NULL);

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
	return 0;
}

int JEAN_send_slave(char *data, int len, unsigned char priority, unsigned char video_analyse)
{
	int sendLen = 0;
    char *buffer;
	struct load_head lHead;

	buffer = (char *)malloc(len + sizeof(struct load_head));
	memcpy(lHead.logo, "JEAN", 4);
	lHead.index = sendIndex;
	lHead.priority = priority;
	lHead.length = len;
	lHead.direction = 0;

	memcpy(buffer, &lHead, sizeof(lHead));
	memcpy(buffer + sizeof(lHead), data, len);

#if TEST_LOST
	int rnd = 0;
	int lost_emt = LOST_PERCENT;
	rnd = rand()%100;
	if(rnd <= lost_emt)
	    printf("Lost!!: rnd = %d, lost_emt = %d, lost_posibility = %d percent\n", rnd, lost_emt, lost_emt);

	if(rnd > lost_emt)
	{
#endif

    if(connectionStatus == P2P)
	{
	    sendLen = sendto(sockfd, buffer, len + sizeof(lHead), 0, (struct sockaddr *)&master_sin, sizeof(struct sockaddr_in));
	}
	else if(connectionStatus == TURN)
	{
	    sendLen = sendto(sockfd, buffer, len + sizeof(lHead), 0, (struct sockaddr *)&turnaddr, sizeof(turnaddr));
	}
	else 
	{
		return NO_CONNECTION; 
	}

#if TEST_LOST
	}
#endif

	if(priority > 0)
		reg_buff(sendIndex, buffer, priority, len);
	sendIndex++;
    sendNum += sendLen;

    return sendLen;
}

int JEAN_recv_slave(char *data, int len, unsigned char priority, unsigned char video_analyse)
{
	int recvLen = 0;
    unsigned long int waitTime = 0;

	while(recvBufP == 0 && waitTime < JEAN_recv_timeout * 10)
	{
		usleep(10);
		waitTime += 10;
	}

	if(recvBufP == 0)
		return 0;

	pthread_mutex_lock(&recvBuf_lock);
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
		recvBufP -= len;
	}
	pthread_mutex_unlock(&recvBuf_lock);
    return recvLen;
}

int JEAN_close_slave()
{
	recvSign = 0;
	free(recvBuf);
	free(recvProcessBuf);
	free(recvProcessBackBuf);
	emptyRing();

	pthread_mutex_destroy(&recvBuf_lock);
	close(sockfd);
	return 0;
}

int main(){
	int ret = 0;
	char data[10];
	char rcv[10];
	int len;

	ret = JEAN_init_slave(server_port, local_port, server_ip_1);
	if(ret < 0)
		return ret;

//	ret = init_CMD_CHAN();
//	if(ret < 0)
//		return ret;
//
//	int i = 0;
//	while(i < 10000)
//	{
////		len = JEAN_recv_slave(data, sizeof(data), 1, 0);
////		if(len > 0)
////			printf("recv: %s %d\n", data, len);
//
//		len = recv_cmd(rcv, 10);
//		if(len > 0)
//			printf("cmd recv!!!!!! %s\n", rcv);
//
////		JEAN_send_slave(data, sizeof(data), 1, 0);
////		printRingStatus();
//		usleep(1000);
//		i++;
//	}
//
//	close_CMD_CHAN();
//	JEAN_close_slave();
	return 0;
}
