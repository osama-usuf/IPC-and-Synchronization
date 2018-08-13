#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#define BUFFERSIZE 10
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) //Sem permissions
#define INITIAL_VALUE 0 //Initial value to be assigned to the sem
#define BUFFERSIZE 10
#define SEM_NAME "/Sem" // This is the name of the named semaphore


//struct for facilitating que buffering operations
typedef struct {
	int counter;
	int in;
	int out;
	int buffer[BUFFERSIZE];
} shm_struct;


int main(int argc, char* argv[])
{

	/* declaring and initialising attributes, openinng file in r mode */
	int shmid;
	key_t key;
	shm_struct *shm;
	int max_vals = 1000;
	int num_vals;
	int *recvd_vals;

	FILE* fp;
	fp = fopen("consumer.txt", "w");
	if (fp == NULL)
		exit(-1);


	/* We open the existing semaphore */

	sem_t *semaphore = sem_open(SEM_NAME, O_RDWR);

	if (semaphore == SEM_FAILED) {
		perror("sem_open(3) error");
		exit(EXIT_FAILURE);
	}

	else {
		/*allocating array for storing consumed vals */
		recvd_vals = malloc (max_vals * sizeof(int));
		if (recvd_vals == NULL) {
			printf("recvd_vals malloc error\n");
			exit(1);
		}
	}
	/*
	 * We need to get the segment named
	 * "5670", created by the producer.
	 */
	key = 5670;
	/*
	 * Locate the segment.
	 */
	if ((shmid = shmget(key, sizeof(shm_struct), 0666)) < 0) {
		perror("shmget");
		exit(1);
	}
	/*
	 * Now we attach the segment to our data space.
	 */
	if ((shm = shmat(shmid, NULL, 0)) == (shm_struct *) - 1) {
		perror("shmat");
		exit(1);
	}
	/*
	 * Now read what the produced puts in the struct in shm.
	 */
	num_vals = 0;
	while (num_vals != max_vals)
	{
		/*the consumer wakes up the producer if queue is empty and waits itself until the buffer is full */
		if (shm->counter == 0) {
			sem_post(semaphore);
			while (shm->counter != BUFFERSIZE) {}
		}

		/*reading a value from the que, and storing it in the array*/

		int val_consumed = shm->buffer[shm->out];
		shm->out = (shm->out + 1) % BUFFERSIZE;
		shm->counter--;
		recvd_vals[num_vals++] = val_consumed;

	}


	// display the values read
	printf("consumer received:\n");
	for (int i = 0; i < max_vals; i++) {
		printf("%d\n", recvd_vals[i]);
		fprintf(fp, "%d \n", recvd_vals[i]);
	}
	shm->counter = 0;
	/*
	 * cleanup: detach  shm and unlink sem
	 */
	fclose(fp);
	shmdt (shm);
	sem_unlink(SEM_NAME);
	sem_close(semaphore);
    free(recvd_vals);
	exit(0);
}
