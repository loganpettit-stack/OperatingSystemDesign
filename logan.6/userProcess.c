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
Frame frame;

int clocKID;
int semaphoreID;
int msqID;

void connectToClock(char *);

void connectToMessageQueue(char *);

void connectSemaphore(char *);

void addTime(long);

int main(int argc, char *argv[]) {

    char errorArr[200];
    char *exe = strdup(argv[0]);
    char *executable = strdup(argv[0]);
    char *errorString = strcat(exe, ": Error: ");
    int tableLocation = atoi(argv[1]);
    int dirtyBitNum = atoi(argv[2]);
    int requestPage = 0;
    int requestAddress = 0;
    int requestType;
    int requestPropability;
    int terminationPropability;
    int requestCount = 0;
    double weightArr[PT_SIZE];
    double weight = 1;
    double randomValue = 0;
    int index = 0;
    double temp = 0;
    double lastValue = 0;

    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);

    srand(time(0) * getpid());

    /*printf("user: process %d started  with location %d at time %ld:%ld\n",
 *            getpid(), tableLocation, memoryClock->seconds, memoryClock->nanoseconds);*/


    /*Set up initial weights in array
 *     int j;
 *         for(j = 0; j < PT_SIZE; j++){
 *                 printf("weight: %f\n", weight);
 *                         weightArr[j] = weight;
 *                                 weight /= (j + 1);
 *                                     }
 *                                         */
    while (true) {

        terminationPropability = (rand() % 100 + 1);
        if(requestCount > 1000) {
            if(terminationPropability > 70){

                /*printf("user process %d trying to terminate\n", getpid());*/

                /*Tell master we are ready to terminate*/
                mesq.mesq_terminate = 1;
                mesq.mesq_type = getppid();
                if(msgsnd(msqID, &mesq, sizeof(mesq), IPC_NOWAIT) == -1){
                    snprintf(errorArr, 200, "\n%s Failed to send message to OSS.\n", errorString);
                    perror(errorArr);
                    exit(1);
                }

                break;
            }
        }


        /*Generate random number between 0 and 32 for and address to request*/
        if(dirtyBitNum == 1) {
            /*Set up initial weights in array*/
            int j;
            weightArr[0] = 1;
            for(j = 1; j < PT_SIZE; j++){
                weight = 1;
                weight /= (j + 1);
                weightArr[j] = weight;
            }

            for(j = 1; j < PT_SIZE;j++){
                temp = weightArr[j - 1];
                weightArr[j] = temp + weightArr[j];
            }
            
            lastValue = weightArr[PT_SIZE - 1];
            randomValue = (lastValue - 0) * (rand() / (double)RAND_MAX) + 0;

            for(j = 0; j < PT_SIZE; j++){
                if(weightArr[j] >= randomValue){
                    index = j;
                    break;
                }
            }

            requestPage = index;
            requestAddress = j * 1024 + (rand() % 1023);

        } else {
            requestPage = (rand() % 32);
            requestAddress = (rand() % 32000);
        }

        /*Generate if its a read or write*/
        requestPropability = (rand() % 100);
        if (requestPropability < 70) {
            requestType = READ;
        } else {
            requestType = WRITE;
        }

        /*Set generated process information*/
        mesq.mesq_pctLocation = tableLocation;
        mesq.mesq_pageReference = requestPage;
        mesq.mesq_memoryAddress = requestAddress;
        mesq.mesq_requestType = requestType;
        mesq.mesq_type = getppid();
        mesq.mesq_pid = getpid();
        mesq.mesq_sentSeconds = memoryClock->seconds;
        mesq.mesq_sentNS = memoryClock->nanoseconds;

        /*printf("user process %d sending message to request memory space\n", getpid());*/

        if(msgsnd(msqID, &mesq, sizeof(mesq), IPC_NOWAIT) == -1){
            snprintf(errorArr, 200, "\n%s Failed to send message to OSS.\n", errorString);
            perror(errorArr);
            exit(1);
        }

        if(msgrcv(msqID, &mesq, sizeof(mesq), getpid(), MSG_NOERROR) == -1){
            snprintf(errorArr, 200, "\n%s Failed to recieve message to OSS.\n", errorString);
            perror(errorArr);
            exit(1);
        }

        requestCount++;

        /*Enter critical resource and add time to clock*/
        sem_wait(&(semaphore->mutex));
        addTime(10);
        sem_post(&(semaphore->mutex));

    }


    /*Clear shared memory*/
    shmdt(memoryClock);
    shmdt(semaphore);

    return 0;
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

void addTime(long nanoseconds) {
    nanoseconds += memoryClock->nanoseconds;
    if (nanoseconds > 1000000000) {
        memoryClock->seconds += 1;
        nanoseconds -= 1000000000;
        memoryClock->nanoseconds = nanoseconds;
    } else {
        memoryClock->nanoseconds += nanoseconds;
    }
}

