#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

struct node_net{
	char Uname[10];
	char Passwd[10];
	struct sockaddr_in recv_sin_l;
	int sin_len;
};

void init_list(){
}

void add_item(struct node_net node){
}

struct node_net * find_item(char * name){
}

int del_item(char * name){
}
