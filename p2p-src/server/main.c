/*AUTHOR:WANGGONG, CHINA
 *VERSION:1.0
 *Function:Server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <JEANP2PPRO.h>
#include <List.h>
#include <DSet.h>
#include <commonkey.h>
#include <errno.h>

#define MAX_RECV_BUF  1024
#define TURN_DATA_SIZE 1024*1024*10
#define MAX_TRY 10
#define PORT1 61000
#define TURN_PORT 61001

#define PEER_SHEET_LEN 200
#define UNAME "wang"
#define PASSWD "123456"

static char pathname[50] = "./natinfo.log";
static int sfd;
static int turnSfd;
static struct sockaddr_in sin, recv_sin;	
static struct sockaddr_in turn_recv_sin;	
static int sin_len, recv_sin_len;
static char *recv_str;
static int port = PORT1;
static char Uname[10];
static char Passwd[10];
static char turnSign = 0;
static pthread_t turn_id;
static char turnThreadRunning = 0;

static int  Peers_Sheet_Index = 0;
static struct node_net *Peer_Login;
static char *recvProcessBuf;
static int recvProcessBufP = 0;

int local_net_init(){
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	//sin.sin_addr.s_addr = inet_addr(ip1);

	sin.sin_port = htons(port);
	sin_len = sizeof(sin);

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(!sfd) return -1;

	if(bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
		printf("bind erro\n");
		return -2;
	}	

	printf("bind to port [%d]\n", port);

	return 0;
}

void init_recv_sin(){
	bzero(&recv_sin, sizeof(recv_sin));
	recv_sin.sin_family = AF_INET;
	recv_sin.sin_addr.s_addr = inet_addr("1.1.1.1");
	recv_sin.sin_port = htons(10000);
	recv_sin_len = sizeof(recv_sin);
}

void set_rec_timeout(int usec, int sec){
	struct timeval tv_out;
    tv_out.tv_sec = sec;
    tv_out.tv_usec = usec;

	setsockopt(sfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
}

struct node_net * Find_Peer(char * user){
	return find_item(user);
}

void Send_CMD(char Ctl, char res){
	char RESP[50];
	RESP[0]	= Ctl;
	RESP[1] = res;
	sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)&recv_sin, recv_sin_len);
}

void Send_CMD_TO_IP(char Ctl, char res, struct sockaddr_in * s){
	char RESP[50];
	RESP[0]	= Ctl;
	RESP[1] = res;
//	if(res == 1)
//	{
//        RESP[2] = COMMON_KEY_1 & 0xff;  
//		RESP[3] = (COMMON_KEY_1>>8) & 0xff;
//		RESP[4] = (COMMON_KEY_1>>16) & 0xff;
//		RESP[5] = (COMMON_KEY_1>>24) & 0xff;
//	    printf("commonkey establish: 0x%x\n", (RESP[2] | (RESP[3]<<8) | (RESP[4]<<16) | (RESP[5]<<24)));
//	}
	sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)s, sizeof(struct sockaddr_in));
}

int Send_CMD_TO_SLAVE(char Ctl, char * name){
	char RESP[50];
	struct node_net * tmp_node;
	tmp_node = find_item(name);
	if(tmp_node == NULL) return -1;
	
	RESP[0] = Ctl;
	sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)tmp_node->recv_sin_s, sizeof(struct sockaddr_in));

	return 0;
}

int Send_S_IP(char * name){
	char RESP[50];
	char GET_W;
	char res;
	struct node_net * tmp_node;
	int i;
	tmp_node = find_item(name);
	if(tmp_node == NULL) return -1;

	RESP[0] = S_IP;
	memcpy(RESP + 1, tmp_node->recv_sin_s, sizeof(struct sockaddr_in));
	
	set_rec_timeout(0, 1);	
	for(i = 0; i < MAX_TRY; i++){
		sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)tmp_node->recv_sin_m, recv_sin_len);
		printf("Send slave ip to master!%s\n", inet_ntoa(tmp_node->recv_sin_s->sin_addr));
		recvfrom(sfd, recv_str, MAX_RECV_BUF, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		if(recv_str[0] == GET_REQ && recv_str[1] == 0x08){
			Send_CMD(GET_REQ, 0x09);
		   	break;
		}
	}
	set_rec_timeout(0, 0);	
}

int Send_M_IP(char * name){
	char RESP[50];
	struct node_net * tmp_node;
	int i;
	tmp_node = find_item(name);
	if(tmp_node == NULL) return -1;

	RESP[0] = RESP_M_IP;
	memcpy(RESP + 1, tmp_node->recv_sin_m, sizeof(struct sockaddr_in));
	
	sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)tmp_node->recv_sin_s, recv_sin_len);
	printf("Send master ip to slave!%s\n", inet_ntoa(tmp_node->recv_sin_m->sin_addr));

	return 0;
}

int Send_M_POL_REQ(char * name){
	char RESP[50];
	struct node_net * tmp_node;
	int i;
	tmp_node = find_item(name);
	if(tmp_node == NULL) return -1;

	RESP[0] = M_POL_REQ;
	
	sendto(sfd, RESP, sizeof(RESP), 0, (struct sockaddr *)tmp_node->recv_sin_m, recv_sin_len);

	return 0;
}

int Peer_Login_Init(){
	Peer_Login = (struct node_net *)malloc(sizeof(struct node_net));
	if(Peer_Login == NULL) return -1;
	Peer_Login->Uname = (char *)malloc(10);
	Peer_Login->Passwd = (char *)malloc(10);
	Peer_Login->sin_len = sizeof(struct sockaddr_in);
	Peer_Login->recv_sin_s = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	Peer_Login->recv_sin_m = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	Peer_Login->local_sin_m = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	Peer_Login->local_sin_s = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

	if(Peer_Login->Uname && Peer_Login->Passwd && Peer_Login->sin_len && Peer_Login->recv_sin_s && Peer_Login->recv_sin_m) return 0;
	else return -1;
}

int Peer_Set(char * name, char * passwd, struct sockaddr_in * r, struct sockaddr_in * l){
	int ret = Peer_Login_Init();
	if(ret < 0){
		printf("Malloc failed when init Peer login sheet!!\n");
		return INIT_PEER_LOGIN_FAIL;
	}

	strcpy(Peer_Login->Uname, name);
	strcpy(Peer_Login->Passwd, passwd);
	memcpy(Peer_Login->recv_sin_m, r, sizeof(struct sockaddr_in));
	memcpy(Peer_Login->local_sin_m, l, sizeof(struct sockaddr_in));
	
	add_item(Peer_Login);

	return 0;
}

int Peer_Set_Slave(char * name, struct sockaddr_in * r, struct sockaddr_in * l){
	struct node_net *tmp_node = find_item(name);
	if(tmp_node != NULL){
		memcpy(tmp_node->recv_sin_s, r, sizeof(struct sockaddr_in));
		memcpy(tmp_node->local_sin_s, l, sizeof(struct sockaddr_in));
		return 0;
	}
	return -1;
}

void clean_rec_buff(){
	char tmp[50];
	int ret;
	set_rec_timeout(100000, 0);//(usec, sec)
	while(ret > 0){
		ret = recvfrom(sfd, tmp, 10, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		printf("Clean recv buff %d.\n", ret);
	}
	set_rec_timeout(0, 1);//(usec, sec)
}

void* turnThread(void *argc)
{
	turnThreadRunning = 1;
	pthread_detach(pthread_self());

	struct sockaddr_in tRecv_sin;	
	int recvLen;
	int length;
	struct sockaddr_in  *p_sin;
	int tRecv_sin_len;
	struct node_net *p_node;
	int optlen;
	int recv_size;
	int err;

	printf("---------------Turn thread start----------------\n");
	recvProcessBufP = 0;
	recvProcessBuf = (char *)malloc(TURN_DATA_SIZE);

	bzero(&turn_recv_sin, sizeof(turn_recv_sin));
	turn_recv_sin.sin_family = AF_INET;
	turn_recv_sin.sin_addr.s_addr = htonl(INADDR_ANY);

	turn_recv_sin.sin_port = htons(TURN_PORT);
	sin_len = sizeof(turn_recv_sin);

	turnSfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(!turnSfd)
	{
	    turnThreadRunning = 0;
		return;
	}

	if(bind(turnSfd, (struct sockaddr *)&turn_recv_sin, sizeof(turn_recv_sin)) != 0){
		printf("bind erro\n");
	    turnThreadRunning = 0;
		return;
	}	

	printf("turn bind to port [%d]\n", TURN_PORT);


	bzero(&tRecv_sin, sizeof(tRecv_sin));
	tRecv_sin.sin_family = AF_INET;
	tRecv_sin.sin_addr.s_addr = inet_addr("1.1.1.1");
	tRecv_sin.sin_port = htons(10000);
	tRecv_sin_len = sizeof(tRecv_sin);

	optlen = sizeof(recv_size); 
	err = getsockopt(turnSfd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen); 
	if(err < 0)
	{ 
		printf("Fail to get recbuf size\n"); 
	} 

	while(turnSign == 1)
	{
		printf("hello\n");
		recvLen = recvfrom(turnSfd, recvProcessBuf + recvProcessBufP, TURN_DATA_SIZE, 0, (struct sockaddr *)&tRecv_sin, &tRecv_sin_len);

		if(recvLen <= 0)
		{
			//			usleep(100);
			continue;
		}

		recvProcessBufP += recvLen;

		if(recvProcessBufP > TURN_DATA_SIZE - recv_size)
		{
			printf("recvBuf overflow!!\n");
			recvProcessBufP = 0;
		}

		if(recvProcessBufP >= sizeof(struct load_head))
		{
			int scanP = 0;
			struct load_head head;
			struct get_head get;
			struct retry_head retry;

			while(scanP + sizeof(struct load_head) < recvProcessBufP)
			{
				if(recvProcessBuf[scanP] == 'J' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'A' && recvProcessBuf[scanP + 3] == 'N')
				{
					memcpy(&head, recvProcessBuf + scanP, sizeof(struct load_head));

#if PRINT
					printf("load head: %c %d %d %d %d\n", head.logo[0], head.index, head.get_number, head.priority, (unsigned int)head.length);
#endif
					if(recvProcessBufP - scanP - sizeof(struct load_head) >= head.length)
					{
						p_node = find_item_by_ip(&tRecv_sin);
						if(p_node != NULL && head.direction == 0) 
							p_sin = p_node->recv_sin_m;
						else if(p_node != NULL && head.direction == 1)
							p_sin = p_node->recv_sin_s;

						if(p_node == NULL){
							printf("Node not found. Turn request rejected!\n");
							break;
						}

						if(p_sin != NULL) 
							sendto(turnSfd, recvProcessBuf + scanP, sizeof(struct load_head) + head.length, 0, (struct sockaddr *)p_sin, sizeof(struct sockaddr));
						else 
							printf("Erro: node info lost!\n");


						scanP = scanP + sizeof(struct load_head) + head.length;
					}
					else
						break;
				}
				else if(recvProcessBuf[scanP] == 'G' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'T')
				{
					memcpy(&get, recvProcessBuf + scanP, sizeof(struct get_head));

					p_node = find_item_by_ip(&tRecv_sin);
					if(p_node != NULL && get.direction == 0) 
						p_sin = p_node->recv_sin_m;
					else if(p_node != NULL && get.direction == 1)
						p_sin = p_node->recv_sin_s;

					if(p_node == NULL){
						printf("Node not found. Turn request rejected!\n");
						break;
					}

					if(p_sin != NULL) 
						sendto(turnSfd, recvProcessBuf + scanP, sizeof(struct get_head), 0, (struct sockaddr *)p_sin, sizeof(struct sockaddr));
					else 
						printf("Erro: node info lost!\n");


					scanP = scanP + sizeof(struct get_head);
				}
				else if(recvProcessBuf[scanP] == 'R' && recvProcessBuf[scanP + 1] == 'T' && recvProcessBuf[scanP + 2] == 'Y')
				{
					memcpy(&retry, recvProcessBuf + scanP, sizeof(struct retry_head));

					p_node = find_item_by_ip(&tRecv_sin);
					if(p_node != NULL && retry.direction == 0) 
						p_sin = p_node->recv_sin_m;
					else if(p_node != NULL && retry.direction == 1)
						p_sin = p_node->recv_sin_s;

					if(p_node == NULL){
						printf("Node not found. Turn request rejected!\n");
						break;
					}

					if(p_sin != NULL) 
						sendto(turnSfd, recvProcessBuf + scanP, sizeof(struct retry_head), 0, (struct sockaddr *)p_sin, sizeof(struct sockaddr));
					else 
						printf("Erro: node info lost!\n");


					scanP = scanP + sizeof(struct retry_head);
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
			struct retry_head retry;

			while(scanP + sizeof(struct get_head) <= recvProcessBufP)
			{
				if(recvProcessBuf[scanP] == 'G' && recvProcessBuf[scanP + 1] == 'E' && recvProcessBuf[scanP + 2] == 'T')
				{
					memcpy(&get, recvProcessBuf + scanP, sizeof(struct get_head));

					p_node = find_item_by_ip(&tRecv_sin);
					if(p_node != NULL && get.direction == 0) 
						p_sin = p_node->recv_sin_m;
					else if(p_node != NULL && get.direction == 1)
						p_sin = p_node->recv_sin_s;

					if(p_node == NULL){
						printf("Node not found. Turn request rejected!\n");
						break;
					}

					if(p_sin != NULL) 
						sendto(turnSfd, recvProcessBuf + scanP, sizeof(struct get_head), 0, (struct sockaddr *)p_sin, sizeof(struct sockaddr));
					else 
						printf("Erro: node info lost!\n");

					scanP = scanP + sizeof(struct get_head);
				}
				else if(recvProcessBuf[scanP] == 'R' && recvProcessBuf[scanP + 1] == 'T' && recvProcessBuf[scanP + 2] == 'Y')
				{
					memcpy(&retry, recvProcessBuf + scanP, sizeof(struct retry_head));

					p_node = find_item_by_ip(&tRecv_sin);
					if(p_node != NULL && retry.direction == 0) 
						p_sin = p_node->recv_sin_m;
					else if(p_node != NULL && retry.direction == 1)
						p_sin = p_node->recv_sin_s;

					if(p_node == NULL){
						printf("Node not found. Turn request rejected!\n");
						break;
					}

					if(p_sin != NULL) 
						sendto(turnSfd, recvProcessBuf + scanP, sizeof(struct retry_head), 0, (struct sockaddr *)p_sin, sizeof(struct sockaddr));
					else 
						printf("Erro: node info lost!\n");


					scanP = scanP + sizeof(struct retry_head);
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

	}

	free(recvProcessBuf);
	close(turnSfd);

	turnThreadRunning = 0;
}

int main(){
	int ret = 0;
	char Get_W;
	char res;
	struct sockaddr_in tmp_sin, *p_sin;
	struct node_net *p_node;
	struct load_head turnHead;
	int length = 0;
	char priority;
	unsigned int recvLen = 0;

	init_list();

	recv_str = (char *)malloc(MAX_RECV_BUF);
	
	ret = local_net_init();
	if(ret < 0){
		printf("local net init failed!!\n");
		return ret;
	}

	init_recv_sin();

	printf("------------------- Welcome to JEAN P2P SYSTEM ---------------------\n");

	while(1){	
		set_rec_timeout(0, 0);//(usec, sec)
		memset(recv_str, 0, 50);
		recvLen = recvfrom(sfd, recv_str, MAX_RECV_BUF, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
		Get_W = recv_str[0];
//		printf("OPCODE = %d\n", Get_W);

		switch(Get_W){
			case V_UAP:
				memset(Uname, 0, 10);
				Get_W = recv_str[0];
				memcpy(Uname, recv_str + 1, 10);
				memcpy(Passwd, recv_str + 12, 10);
				memcpy(&tmp_sin, recv_str + 34, sizeof(struct sockaddr_in));

				//printf("Recieve from %s [%d]:%d %s %s\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname, Passwd);
				if(Uname == NULL){
					printf("Error:User name is NULL!!");
					break;
				}

				printf("Recieve from %s [%d]:%d %s ***\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname);
				printf("Master local IP is %s\n", inet_ntoa(tmp_sin.sin_addr));
				printf("Verify result: Uname = %d Passwd = %d\n", strcmp(UNAME, Uname), strcmp(PASSWD, Passwd));

				if((strcmp(UNAME, Uname) != 0) || (strcmp(PASSWD, Passwd) != 0)){
					printf("Username or password error!!\n");
					Send_CMD(V_RESP, 0x2);
					printf("Send response.\n");
				}
				else{
					printf("Username and password verifying passed!!\n");

					int Insert_Success = 0;
					if(Find_Peer(Uname) == 0){
						if(Peers_Sheet_Index < PEER_SHEET_LEN){
							ret = Peer_Set(Uname, Passwd, &recv_sin, &tmp_sin);
							printf("rec: %s local: %s\n", inet_ntoa(recv_sin.sin_addr), inet_ntoa(tmp_sin.sin_addr));
							if(ret < 0){
								printf("Set peer failed!\n");
								return ret;
							}

							printListIp();
							Peers_Sheet_Index++;
							Insert_Success = 1;
							printf("Registor success!! Now index at %d\n", Peers_Sheet_Index);
						}
						else{
							Insert_Success = 0;
							printf("Registor error!! Now index at %d\n", Peers_Sheet_Index);
						}
					}

					if(!Insert_Success)
						Send_CMD(V_RESP, 0x3);
					else
						Send_CMD(V_RESP, 0x1);
					printf("Send response.\n");
				}
				break;

			case V_UAP_S:
				memset(Uname, 0, 10);
				Get_W = recv_str[0];
				memcpy(Uname, recv_str + 1, 10);
				memcpy(Passwd, recv_str + 12, 10);
				memcpy(&tmp_sin, recv_str + 34, sizeof(struct sockaddr_in));

				//printf("Recieve from %s [%d]:%d %s %s\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname, Passwd);
				if(Uname == NULL){
					printf("Error:User name is NULL!!");
					break;
				}

				printf("Recieve from %s [%d]:%d %s ***\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname);
				printf("Slave local IP is %s\n", inet_ntoa(tmp_sin.sin_addr));
				printf("Verify result: Uname = %d Passwd = %d\n", strcmp(UNAME, Uname), strcmp(PASSWD, Passwd));

				if((strcmp(UNAME, Uname) != 0) || (strcmp(PASSWD, Passwd) != 0)){
					printf("Username or password error!!\n");
					Send_CMD(GET_REQ, 0x5);
					printf("Send response.\n");
				}
				else{
					printf("Username and password verifying passed!!\n");

					int Find_Success = 0;
					if(Find_Peer(Uname) != 0){
							Find_Success = 1;
							printf("Node find success!! Now index at %d\n", Peers_Sheet_Index);
					}

					if(Find_Success){
						Send_CMD(GET_REQ, 0x4);
						printf("Send response.\n");
						printf("rec: %s local: %s\n", inet_ntoa(recv_sin.sin_addr), inet_ntoa(tmp_sin.sin_addr));
						ret = Peer_Set_Slave(Uname, &recv_sin, &tmp_sin);
						if(ret < 0){
							printf("Login sheet is broken!\n");
							return LOGIN_SHEET_BROKEN;
						}
							
						printListIp();
						Send_S_IP(Uname);
					}
					else{
						Send_CMD(GET_REQ, 0x6);
						printf("Send response.\n");
					}
				}
				break;
				
			case REQ_M_IP:
				memset(Uname, 0, 10);
				sscanf(recv_str, "%c %s", &Get_W, Uname);
				//printf("Recieve from %s [%d]:%d %s %s\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname, Passwd);
				if(Uname == NULL){
					printf("Error:User name is NULL!!");
					break;
				}

				printf("Recieve from %s [%d]:%d %s ***\n", inet_ntoa(recv_sin.sin_addr), ntohs(recv_sin.sin_port), Get_W, Uname);

				int Find_Success = 0;
				if(Find_Peer(Uname) != 0){
					Find_Success = 1;
					printf("Node find success!! Now index at %d\n", Peers_Sheet_Index);
				}

				if(Find_Success){
					Send_M_IP(Uname);
				}
				break;

			case KEEP_CON:
				Send_CMD(GET_REQ, 0x03);
				printf("KEEP_CON has already responsed.\n");
				break;

			case GET_REQ:
				if(recv_str[1] == 0x08){
					Send_CMD(GET_REQ, 0x9);
					printf("IP confirm pack has already responsed.\n");
				}
				break;

			case MASTER_QUIT:
				turnSign = 0;
				memset(Uname, 0, 10);
				sscanf(recv_str, "%c %s", &Get_W, Uname);

				if(0 == del_item(Uname))
				{
					Send_CMD(GET_REQ, 0x1);
					printf("Node Deleted.\n");
				}
				else
				{
					Send_CMD(GET_REQ, 0x1);
					printf("Node Deleted.\n");
				}
				break;

			case POL_SENT:
				memset(Uname, 0, 10);
				memcpy(Uname, recv_str + 1, 10);

				Send_CMD(GET_REQ, 0xb);
				printf("Get POL_SENT.\n");

				clean_rec_buff();
				int i = 0;
				int master_mode = 1;
				int cmd_sent = 0;
				struct node_net * tmpn;

				tmpn = find_item(Uname);

				set_rec_timeout(0, 10);//(usec, sec)
				for(i = 0; i < MAX_TRY; i++){
					memset(recv_str, 0, 50);
					if(!cmd_sent){
						ret = Send_M_POL_REQ(Uname);
						if(ret < 0) printf(" Username connect failed.\n");
						printf("Master connecting slave...\n");
					}
					recvfrom(sfd, recv_str, MAX_RECV_BUF, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
					if(recv_str[0] == GET_REQ && recv_str[1] == 0x0e){
						printf("Pole ok! Connection established.\n");
						tmpn->pole_res = 1;
						break;
					}
					else if(recv_str[0] == GET_REQ && recv_str[1] == 0x0f){
						printf("Pole failed! Change to slave mode.\n");
						master_mode = 0;
						tmpn->pole_res = 0;
						break;
					}
					else if(recv_str[0] == GET_REQ && recv_str[1] == 0x12)
						cmd_sent = 1;
				}

				if(i >= MAX_TRY){
						printf("Pole failed! Change to slave mode.\n");
						master_mode = 0;
						tmpn->pole_res = 0;
				}
				
				clean_rec_buff();
				cmd_sent = 0;
				if(master_mode == 0){
					set_rec_timeout(0, 10);//(usec, sec)
					for(i = 0; i < MAX_TRY; i++){
						memset(recv_str, 0, 50);
						if(!cmd_sent){
							ret = Send_CMD_TO_SLAVE(S_POL_REQ ,Uname);
							if(ret < 0) printf(" Username connect failed.\n");
							printf("Slave connecting master...\n");
						}

						recvfrom(sfd, recv_str, MAX_RECV_BUF, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
						if(recv_str[0] == GET_REQ && recv_str[1] == 0x10){
							printf("Pole ok! Connection established.\n");
							tmpn->pole_res = 1;
							break;
						}
						else if(recv_str[0] == GET_REQ && recv_str[1] == 0x11){
							printf("Pole failed!\n");
							tmpn->pole_res = 0;
							break;
						}
						else if(recv_str[0] == GET_REQ && recv_str[1] == 0x13)
							cmd_sent = 1;
					}
				}

				if(i >= MAX_TRY){
						printf("Pole failed!\n");
						master_mode = 0;
						tmpn->pole_res = 0;
				}

				int res_count = 0;
				set_rec_timeout(0, 1);//(usec, sec)
				for(i = 0; i < MAX_TRY; i++){
					if(tmpn->pole_res == 0){
						Send_CMD_TO_IP(CON_ESTAB, 2, tmpn->recv_sin_m);
						Send_CMD_TO_IP(CON_ESTAB, 2, tmpn->recv_sin_s);
					}
					else{
						Send_CMD_TO_IP(CON_ESTAB, 1, tmpn->recv_sin_m);
						Send_CMD_TO_IP(CON_ESTAB, 1, tmpn->recv_sin_s);
					}
					printf("Send connection result(%d) to master and slave.\n", tmpn->pole_res);

					recvfrom(sfd, recv_str, MAX_RECV_BUF, 0, (struct sockaddr *)&recv_sin, &recv_sin_len);
					if(recv_str[0] == GET_REQ && recv_str[1] == 0x14){
						break;
					}

				}

				if(i >= MAX_TRY){
					printf("ERRO: Some node offline!\n");
				}

				break;
				
			case TURN_REQ:
				Send_CMD(GET_REQ, 0x1);
				turnSign = 1;
				printf("Entering turn thread!\n");
				recvProcessBufP = 0;
				if(turnThreadRunning == 0)
					pthread_create(&turn_id, NULL, turnThread, NULL);
				printf("%d\n", (int)turn_id);
				break;

		}
	}

	turnSign = 0;
	free(recv_str);
	empty_item();
	close(sfd);
	return 0;
}
