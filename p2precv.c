#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT1 61000
#define ip1   "192.168.1.115"
#define ip2   "192.168.1.116"
#define wanggong_nat_ip "192.168.1.110" 
#define wanggong_ip "192.10.1.2"


char pathname[50] = "./natinfo.log";

int main(){
	int sin_len;
	char recv[100];
	int port = PORT1;

	int sfd;
	struct sockaddr_in sin;	
	FILE  *fp;
  	if((fp = fopen(pathname,"a+")) == NULL)//打开文件没有就创建
    	{
        	printf("文件还未创建!\n");
    	}
	
	printf("-------------------Nat Test Listening(UDP)---------------------\n");
	fprintf(fp, "-------------------Nat Test Listening(UDP)---------------------\n");
	
	fclose(fp);
	while(1){
		if((fp = fopen(pathname,"a+")) == NULL)//打开文件没有就创建
    		{
        		printf("文件还未创建!\n");
    		}	
	

		bzero(&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
//		sin.sin_addr.s_addr = inet_addr("192.168.1.115");

		//	sin.sin_addr.s_addr = inet_addr(wanggong_nat_ip);
			printf("bind  \n");
		

		sin.sin_port = htons(port);
		sin_len = sizeof(sin);
				
		sfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(!sfd) return -1;
	
		if(bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
			printf("bind erro\n");
			return -1;
		}	
		
		printf("bind to port [%d]\n", port);
		fprintf(fp, "bind to port [%d]\n", port);
		int i = 0;
		for(i = 0; i < 5; i++){	
			printf("waiting...\n");	
			recvfrom(sfd, recv, sizeof(recv), 0, (struct sockaddr *)&sin, &sin_len);
			printf("Recieve from %s [%d]:%s\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), recv);
			fprintf(fp, "Recieve from %s [%d]:%s\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), recv);
		}

		if(port < (PORT1 + 2) )
			port++;
		else if(port >= (PORT1 + 2)){
			port = PORT1;
		}

		fclose(fp);
		close(sfd);
	}

	close(sfd);

	return 0;
}
