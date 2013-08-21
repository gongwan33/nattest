#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT1 61000
#define count 1000 

int main(){
	int sin_len;
	char *recv;
	int port = PORT1;

	int package_count = 0;

	int sfd;
	struct sockaddr_in sin;	

	recv = (char *)malloc(1024*1024);
	
	printf("-------------------Nat Test Listening(UDP)---------------------\n");


		bzero(&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_port = htons(port);
		sin_len = sizeof(sin);
				
		sfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(!sfd) return -1;
	
		if(bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) != 0){
			printf("bind erro\n");
			return -1;
		}	
		
		printf("bind to port [%d]\n", port);

		int i = 0;
		for(i = 0; i < count; i++){		
			int rec_size = recvfrom(sfd, recv, sizeof(recv), 0, (struct sockaddr *)&sin, &sin_len);
			package_count++;
		//	printf("Recieve from %s [%d]:%s\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), recv);
			printf("recv:Package Num = %d; size = %d\n", package_count, rec_size);
		}


		close(sfd);
		free(recv);


	return 0;
}
