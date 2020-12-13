// gcc -O6 -Wall -pedantic -Wextra ringbuffer.c pipebuf.c -o pipebuf -lpthread
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include "ringbuffer.h"

int stop = 0;

void * consumer_proc(void *arg)
{
	unsigned int cnt;
	struct ringbuffer *ring_buf = (struct ringbuffer *)arg;

	cnt = 0;

	while(!stop || !ringbuffer_is_empty(ring_buf))
	{
		{
			char i;
			
			if (ringbuffer_is_empty(ring_buf)) {
				usleep(1000);
				continue;
			}

			ringbuffer_get(ring_buf, &i, sizeof(i));

			putchar(i);
			fflush(stdout);
			cnt++;
		}
	}
	return NULL;
}

void * producer_proc(void *arg)
{
	struct ringbuffer *ring_buf = (struct ringbuffer *)arg;
	int i;

	while(1)
	{
		if (ringbuffer_is_full(ring_buf)) {
			usleep(1000);
			continue;
		}
		i = getchar();
		if (i == EOF)
			break;

		ringbuffer_put(ring_buf, (char*)&i, 1);
	}
	stop = 1;
	return NULL;
}


int consumer_thread(void *arg)
{
	int err;
	pthread_t tid;
	err = pthread_create(&tid, NULL, consumer_proc, arg);
	if (err != 0)
	{
		fprintf(stderr, "Failed to create consumer thread.errno:%u, reason:%s\n",
				errno, strerror(errno));
		return -1;
	}
	return tid;
}
int producer_thread(void *arg)
{
	int err;
	pthread_t tid;
	err = pthread_create(&tid, NULL, producer_proc, arg);
	if (err != 0)
	{
		fprintf(stderr, "Failed to create consumer thread.errno:%u, reason:%s\n",
				errno, strerror(errno));
		return -1;
	}
	return tid;
}



int main(int argc, char *argv[])
{
	struct ringbuffer *ring_buf;
	pthread_t produce_pid, consume_pid; 
	unsigned int size; // maximal power of 2 is 2G

	assert(argc == 2);
	assert(strlen(argv[1])>0);
	size = atoi(argv[1]);
	switch (argv[1][strlen(argv[1])-1])
	{
		case 'G': size *= 0x04000000; break;
		case 'M': size *= 0x00010000; break;
		case 'K': size *= 0x00000400; break;
	}
	ring_buf = ringbuffer_create(size);
	if (!ring_buf) {
		perror("ringbuffer_create()");
		exit(1);
	}

	produce_pid  = producer_thread((void*)ring_buf);
	consume_pid  = consumer_thread((void*)ring_buf);

	pthread_join(produce_pid, NULL);
	pthread_join(consume_pid, NULL);

	ringbuffer_destroy(ring_buf);

	return 0;
}
