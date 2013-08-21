#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define MAXline 1024
#define server_ip "58.214.236.114"
#define client_ip "192.168.1.168"
#define server_port 61000
#define count 1000 
#define gap_between_package 10 //unit: us
#define test_size 6 

int main(){
	int sockfd;
	int n, i, j;
	int package_count = 0;
	int port, sin_size;
   	char recvline[MAXline], sendline[MAXline];
    	struct sockaddr_in servaddr,local_addr;
	char ip_info[50];
	memset (&local_addr, 0, sizeof local_addr);
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl (INADDR_ANY);
        local_addr.sin_port = htons(0);
	char *test_package;	
	char test_size_info[10];
	test_package = (char *)malloc(test_size);
	
		
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
   		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
    		return 0;
   	}
	if (bind(sockfd, (struct sockaddr *) (&local_addr),sizeof(local_addr)) == -1){
           	printf ("Bind error: %s\a\n", strerror (errno));
           	return 0;
        }
	
	sin_size = sizeof(local_addr);
	getsockname(sockfd, (struct sockaddr*)&local_addr, &sin_size);
        port=ntohs(local_addr.sin_port);
        printf("in child process port is: %d\n", port);
		memset(&servaddr, 0, sizeof(servaddr));
 		servaddr.sin_family = AF_INET;
    		servaddr.sin_port = htons(server_port);

    		if( inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0){
    		printf("inet_pton error for %s\n",server_ip);
    		exit(0);
    		}

		printf("-------------------Nat Test Package Sending---------------------\n");
	
		//sprintf(ip_info,"%s [%d]", client_ip, port);
		sprintf(test_package,"test_package");
		sprintf(test_size_info,"%d", test_size);
		
		sendto(sockfd, test_size_info, sizeof(test_size_info), 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

	
		for(i = 0; i < count; i++){
	    //sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr,sizeof(servaddr)); 
		int send_num = sendto(sockfd, test_package, test_size, 0, (struct sockaddr *)&servaddr,sizeof(servaddr)); 
//		printf("send_num = % d\n", send_num);
		package_count++;
		usleep(gap_between_package);
		}

	printf("Package Num = %d\n", package_count);
  	close(sockfd);
    	exit(0);	
	return 0;
}
