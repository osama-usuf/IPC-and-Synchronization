#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
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
    srand(time(NULL));
    int shmid;
    key_t key;
    shm_struct * shm;
    int max_vals = 1000;
    int num_vals;
    int *gend_vals;

    FILE* fp;
    fp = fopen("producer.txt", "w");
    if (fp == NULL) exit(-1);

    /* We initialize the semaphore counter to 1 (INITIAL_VALUE) */
    sem_t *semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);

    if (semaphore == SEM_FAILED) {
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }
    else {
        /*allocating array for storing produced vals */
        gend_vals = malloc (max_vals * sizeof(int));
        if (gend_vals == NULL) {
            printf("recvd_vals malloc error\n");
            exit(1);
        }
    }


    /*
     * We'll name our shared memory segment
     * "5670".
     */
    key = 5670;

    /*
     * Create the segment.
     */

    if ((shmid = shmget(key, sizeof(shm_struct), IPC_CREAT | 0666)) < 0) {
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
     * Now put some things into the memory for the
     * other process to read.
     */

    /* inititialising shm que attributes */
    shm->counter = 0;
    shm->in = 0;
    shm->out = 0;
    num_vals = 0;
    printf("Producer generating:\n");
    while (num_vals < max_vals)
    {
        /* the producer waits for the consumer to consume the
        values from the shared que once buffer is full */
        if (shm->counter == BUFFERSIZE) {
            sem_wait(semaphore);
        }


        /* producing a random int and storing it onto the array */
        int random = rand() % 10;
        printf("%d\n", random);
        shm->buffer[shm->in] = random;  // next vlaue prduced
        shm->in = (shm->in + 1) % BUFFERSIZE;
        gend_vals[num_vals++] = random;
        shm->counter++;

    }

    //waiting until all vals have been consumed
    while (shm->counter != 0)
        sleep(1);

    // write the values produced
    for (int i = 0; i < max_vals; i++) {
        fprintf(fp, "%d \n", gend_vals[i]);
    }


    /* cleanup: detach and remove shm and unlink and close sem
    */
    sem_unlink(SEM_NAME);
    sem_close(semaphore);
    shmdt (shm);
    shmctl (shmid, IPC_RMID, NULL);
    fclose(fp);
    free(gend_vals);
    exit(0);
}

