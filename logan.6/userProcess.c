#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/msg.h>
#include <setjmp.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/ipc.h>
#include "structures.h"

MemoryClock *memoryClock;
Semaphore *semaphore;
MESQ mesq;


int clocKID;
int semaphoreID;
int msqID;

void connectToClock(char*);

void connectToMessageQueue(char *);

void connectSemaphore(char *);

int main(int argc, char *argv[]) {

    char *exe = strdup(argv[0]);
    char *executable = strdup(argv[0]);
    char *errorString = strcat(exe, ": Error: ");
    int tableLocation = atoi(argv[1]);

    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);

    printf("user: process %d started  with location %d at time %ld:%ld\n",
            getpid(), tableLocation, memoryClock->seconds, memoryClock->nanoseconds);


    /*Clear shared memory*/
    shmdt(memoryClock);
    shmdt(semaphore);
}

/*Function to connect to shared memory and error check*/
void connectToClock(char *error) {
    char errorArr[200];

    /*Get shared memory Id using shmget*/
    clocKID = shmget(CLOCK_KEY, sizeof(memoryClock), 0777 | IPC_CREAT);

    /*Error check shmget*/
    if (clocKID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space for clock", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Attach to shared memory using shmat*/
    memoryClock = shmat(clocKID, NULL, 0);

    /*Error check shmat*/
    if (memoryClock == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %d Failed to attach to shared memory clock", error, getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}

/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {
    char errorArr[200];

    if ((msqID = msgget(MSG_KEY, 0777 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s Pid: %d Failed to connect to message queue", error, getpid());
        perror(errorArr);
        exit(1);
    }
}

void connectSemaphore(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((semaphoreID = shmget(SEM_KEY, sizeof(Semaphore), 0644 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for semaphore", error,
                 (long) getpid());
        perror(error);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if ((semaphore = (Semaphore *) shmat(semaphoreID, (void *) 0, 0)) == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for semaphore", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}
