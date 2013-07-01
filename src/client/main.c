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

#define server_ip_1 "58.214.236.114"
#define server_ip_2 "192.168.0.188"
#define server_port 61000
#define count 20 

int main(){
	int sockfd;
	int n, i, j;
	int port, sin_size;
	char mac[6], ip[4], buff[1024];
    	struct sockaddr_in servaddr1,servaddr2,local_addr;
	struct ifreq ifr,*pifr;
    	struct ifconf ifc;
	char ip_info[50];
	
	memset (&local_addr, 0, sizeof local_addr);
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl (INADDR_ANY);
        local_addr.sin_port = htons(1888);
	
	
		
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
	
	memset(&ifr, 0, sizeof(ifr));
    	ifc.ifc_len = sizeof(buff);
    	ifc.ifc_buf = buff;
    	if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0){
        	printf("SIOCGIFCONF screwed up\n");
        	return 0;
    	}

    	pifr = (struct ifreq *)(ifc.ifc_req);
    	
	for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; pifr++){
        	strcpy(ifr.ifr_name, pifr->ifr_name);    //eth0 eth1 ...
        	if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0){
            		printf("ip screwed up\n");
            		return 0;
        	}
        	memcpy(ip, ifr.ifr_addr.sa_data+2, 4);
        	printf("%s\n",inet_ntoa(*(struct in_addr *)ip));
    	}	




	printf("%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
        printf("in child process port is: %d\n", port);
	for(j = 0; j <3; j++){
		memset(&servaddr1, 0, sizeof(servaddr1));
 		servaddr1.sin_family = AF_INET;
    		servaddr1.sin_port = htons(server_port+j);

		memset(&servaddr2, 0, sizeof(servaddr2));
 		servaddr2.sin_family = AF_INET;
    		servaddr2.sin_port = htons(server_port+j);

    		if( inet_pton(AF_INET, server_ip_1, &servaddr1.sin_addr) <= 0){
    		printf("inet_pton error for %s\n",server_ip_1);
    		exit(0);
    		}

		if( inet_pton(AF_INET, server_ip_2, &servaddr2.sin_addr) <= 0){
    		printf("inet_pton error for %s\n",server_ip_2);
    		exit(0);
    		}

		printf("-------------------Nat Test Package Sending---------------------\n");
	
		sprintf(ip_info,"%s [%d]", inet_ntoa(*(struct in_addr *)ip), port);
	
		for(i = 0; i < count; i++){
			sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr1,sizeof(servaddr1)); 
		}
		
		for(i = 0; i < count; i++){
			sendto(sockfd, ip_info, sizeof(ip_info), 0, (struct sockaddr *)&servaddr2,sizeof(servaddr2)); 
		}

	sleep(1);
       	}

  	close(sockfd);
    	exit(0);	
	return 0;
}
