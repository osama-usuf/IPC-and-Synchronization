#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mqueue.h>
#include "common.h" //common header for both lwp

# define BUFFERSIZE 10

//prototypes for thread fns.
void *producer(void * arg); 
void *consumer(void * arg);

int main()
{
	pthread_t ptid, ctid; 
	pthread_create(&ctid, NULL, consumer, NULL); //creating producer and consumer threads, inititating functions are the ones prototyped above
	pthread_create(&ptid, NULL, producer, NULL);
	pthread_join(ctid, NULL); //waiting for threads to finish
	pthread_join(ptid, NULL);
	return 0;
}

void *producer(void* arg)
{
	mqd_t mq;
	struct mq_attr attr;
	char buffer[MAX_SIZE + 1] = {'\0'};
	int *gend_vals;
	int max_vals = 1000;
	int val_count = 0;
	srand(time(NULL));

	/* initialize the file in w mode, also the queue attributes */

	FILE* fp;
	fp = fopen("producer.txt", "w");
	if (fp == NULL)
		exit(-1);



	attr.mq_flags = 0;//Blocking send, producer will be blocked if buffer is full
	attr.mq_maxmsg = BUFFERSIZE;//MQ Size, (10)
	attr.mq_msgsize = MAX_SIZE;
	attr.mq_curmsgs = 0;

	/* create the message queue */
	mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);
	CHECK((mqd_t) - 1 != mq);

	/* allocate array for producer */

	gend_vals = malloc (max_vals * sizeof(int));
	if (gend_vals == NULL) {
		printf("gend_vals malloc error\n");
		exit(1);
	}


	while (val_count < max_vals)
	{

		mq_getattr(mq, &attr); 	//getting updated que attributes

		if (attr.mq_curmsgs < attr.mq_maxmsg) 
		{
			int random = rand() % 10; //generating random int and sending to mqueue
			gend_vals[val_count++] = random;
			//converting int to string for writing to file
			sprintf(buffer, "%d", random);
			CHECK(0 <= mq_send(mq, buffer, MAX_SIZE, 0));
			attr.mq_curmsgs++;
		}
	}

	printf("Producer produced:\n");
	for (int i = 0; i < max_vals; i++) {
		printf("%d\n", gend_vals[i]);
		fprintf(fp, "%d \n", gend_vals[i]);
	}
	fclose(fp);
	// /* cleanup */
    free(gend_vals);
	CHECK((mqd_t) - 1 != mq_close(mq));
	CHECK((mqd_t) - 1 != mq_unlink(QUEUE_NAME));
	pthread_exit(NULL);
}

void *consumer(void* arg)
{
	/* opening mq, setting variables and opening file in w mode */

	mqd_t mq;
	char buffer[MAX_SIZE];
	struct mq_attr attr;
	int max_vals = 1000;
	int* recvd_vals;

	FILE* fp;
	fp = fopen("consumer.txt", "w");
	if (fp == NULL)
		exit(-1);

	mq = mq_open(QUEUE_NAME, O_RDONLY );
	CHECK((mqd_t) - 1 != mq);
	mq_getattr(mq, &attr);

	/* allocating array for consumed values */
	recvd_vals = malloc (max_vals * sizeof(int));
	if (recvd_vals == NULL) {
		printf("recvd_vals malloc error\n");
		exit(1);
	}


	for (int i = 0; i < max_vals; i++)
	{
		mq_receive(mq, buffer, MAX_SIZE, NULL);	//receiving message from que, converting it to an int and storing in the array
		int received = atoi(buffer);
		recvd_vals[i] = received;
		attr.mq_curmsgs = attr.mq_curmsgs - 1;
	}

	printf("Consumer consumed:\n");
	for (int i = 0; i < max_vals; i++) {
		printf("%d\n", recvd_vals[i]);
		fprintf(fp, "%d \n", recvd_vals[i]);
	}

	fclose(fp);
    free(recvd_vals);

	pthread_exit(NULL);
}
