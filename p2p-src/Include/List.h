/*AUTHOR:WANGGONG, CHINA
 *VERSION:1.0
 *Function:LIST HEAD
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>
#include <stdlib.h>

#define PREFETCH 0

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({   \
		const typeof(((type *)0)->member) * __mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); })

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#if PREFETCH
static inline void prefetch(const void *addr)
{
	__asm__("ldw 0(%0), %%r0" : : "r" (addr));  //just aimed at improving the speed of iterating the list
}

#define list_for_each_entry(pos, head, member)    \
	for (pos = list_entry((head)->next, typeof(*pos), member); \
			prefetch(pos->member.next), &pos->member != (head);  \
			pos = list_entry(pos->member.next, typeof(*pos), member))
#endif

#define list_for_each(pos, head) for(pos = (head)->next; pos != (head); pos = pos->next)

#if PREFETCH
#define list_for_each(pos, head) \
    for (pos = (head)->next; prefetch(pos->next), pos != (head); \
		    pos = pos->next)
#endif

struct list_head {
	struct list_head *next, *prev;
};

struct node_net{
	char * Uname;
	char * Passwd;
	struct sockaddr_in * recv_sin_m;
	struct sockaddr_in * recv_sin_s;
	struct sockaddr_in * local_sin_m;
	struct sockaddr_in * local_sin_s;
	int pole_res;
	int sin_len;
    	
	struct list_head list;
 };

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}


static inline void __list_add(struct list_head *new,
		struct list_head *prev,
		struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}


static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}


struct list_head head, *plist;

void init_list(){
	INIT_LIST_HEAD(&head);
}

void add_item(struct node_net *item){
	list_add(&(item->list), &head);
}

struct node_net *node;

struct node_net * find_item(char *name){
	list_for_each(plist, &head){
		node = list_entry(plist, struct node_net, list);
	//	printf("%s\n",node->Uname);
	//	printf("%d\n",node->sin_len);
		if(strcmp(node->Uname, name) == 0) return node;
	}

	return NULL;
}

struct node_net * find_item_by_ip(struct sockaddr_in *ip){
	char in_ip[20];
	char slv[20];
	char mst[20];
	sprintf(in_ip, "%s", inet_ntoa(ip->sin_addr));
	sprintf(slv, "%s", inet_ntoa(node->recv_sin_s->sin_addr));
	list_for_each(plist, &head){
		node = list_entry(plist, struct node_net, list);
	//	printf("%s\n",node->Uname);
		sprintf(slv, "%s", inet_ntoa(node->recv_sin_s->sin_addr));
		sprintf(mst, "%s", inet_ntoa(node->recv_sin_m->sin_addr));
		if(strcmp(mst, in_ip) == 0)
		{
		   	return node;
		}
		else if(strcmp(slv, in_ip) == 0)
		{
			return node;
		}
	}

	return NULL;
}

void printListIp(){
	list_for_each(plist, &head){
		node = list_entry(plist, struct node_net, list);
	     printf("rs %s\n", inet_ntoa(node->recv_sin_s->sin_addr));
	     printf("ls %s \n", inet_ntoa(node->local_sin_s->sin_addr));
	     printf("rm %s\n", inet_ntoa(node->recv_sin_m->sin_addr));
	     printf("lm %s \n", inet_ntoa(node->local_sin_m->sin_addr));
	}
}

int del_item(char *name){
	list_for_each(plist, &head){
    struct node_net *tmp_node = list_entry(plist, struct node_net, list);
		if(strcmp(tmp_node->Uname, name) ==0){
		   	list_del(&tmp_node->list);
			free(tmp_node->Uname);
			free(tmp_node->Passwd);
			free(tmp_node->recv_sin_s);
			free(tmp_node->recv_sin_m);
			free(tmp_node->local_sin_s);
			free(tmp_node->local_sin_m);

			free(tmp_node);
			return 0;
		}
	}

	return -1;
}

int empty_item(){
	list_for_each(plist, &head){
    struct node_net *tmp_node = list_entry(plist, struct node_net, list);
		   	list_del(&tmp_node->list);
			free(tmp_node->Uname);
			free(tmp_node->Passwd);
			free(tmp_node->recv_sin_s);
			free(tmp_node->recv_sin_m);
			free(tmp_node->local_sin_s);
			free(tmp_node->local_sin_m);
			free(tmp_node);
	}
	return list_empty(&head);
}
#endif
