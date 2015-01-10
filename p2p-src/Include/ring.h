#ifndef RING_H
#define RING_H

#define TRY_MAX_TIMES 20
#define RING_LEN 8

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void initRing();
int reg_buff(unsigned int index, char *pointer);
int unreg_buff(unsigned int index);
void emptyRing();
void printRingStatus();

struct buf_node
{
    unsigned int index;
    char *pointer;
};

static pthread_mutex_t ring;
static struct buf_node buf_list[RING_LEN];
static char empty_list[RING_LEN];

void printRingStatus()
{
    int i = 0;
	printf("ring status:");
	for(i = 0; i < RING_LEN; i++)
		printf("%d ", empty_list[i]);
	printf("\n");

	printf("ring data:");
	for(i = 0; i < RING_LEN; i++)
		printf("%d ", buf_list[i].index);
	printf("\n");

}

void initRing()
{
	memset(empty_list, -1, RING_LEN);
	memset(buf_list, -1, RING_LEN);
}

void emptyRing()
{
	int i = 0;
	for(i = 0; i < RING_LEN; i++)
	{
        if(empty_list[i] == 1)
		{
			free(buf_list[i].pointer);
			empty_list[i] = -1;
		}
	}

}

int getEmpPos()
{
	int i = 0;
	for(i = 0; i < RING_LEN; i++)
	{
        if(empty_list[i] == -1)
			return i;
	}
	return -1;
}

int getIndexPos(unsigned int index)
{
    int i = 0 ;
	for(i = 0; i < RING_LEN; i++)
		if(buf_list[i].index == index)
			if(empty_list[i] == 1)
				return i;

	return -1;
}

int reg_buff(unsigned int index, char *pointer)
{
	int times = 0;
    int pos = 0;

	pthread_mutex_lock(&ring);
	pos = getEmpPos();
	while(pos == -1 && times < TRY_MAX_TIMES)
	{
		pos = getEmpPos();
		usleep(10000);
		times++;
	}

	if(times >= TRY_MAX_TIMES)
		emptyRing();

	pos = getEmpPos();
    buf_list[pos].index = index;
    buf_list[pos].pointer = pointer;
    empty_list[pos] = 1;

	pthread_mutex_unlock(&ring);
    return 0;	
}

int unreg_buff(unsigned int index)
{
	int pos = 0;
	pthread_mutex_lock(&ring);
	pos = getIndexPos(index);

	printf("pos: %d\n", pos);

	if(pos == -1)
		return -1;

	free(buf_list[pos].pointer);
    empty_list[pos] = -1;
	pthread_mutex_unlock(&ring);
	return 0;
}

#endif
