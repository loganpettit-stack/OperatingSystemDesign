#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include "Structures.h"

#define CLOCKKEY 0x3574
#define SEMAPHOREKEY 0x1470
#define MESSAGEKEY 0x3596
#define PCBKEY 0x1596
#define DESCRIPTORKEY 0x0752

MemoryClock *memoryClock;
Semaphore *semaphore;
MessageQ *mesq;
ResourceDiscriptor *resources;
ProcessCtrlBlk *pcbTable;

int clocKID;
int pcbID;
int semaphoreID;
int descriptorID;
int msqID;

MemoryClock *connectToClock(char *);
void connectToMessageQueue(char *);
ProcessCtrlBlk *connectPcb(int, char *);
void connectSemaphore(char *);
void connectDescriptor(char *);


int main(int argc, char *argv[]) {

    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];

    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);
    connectPcb(numPCB, errorString);
    connectDescriptor(errorString);



    /*Clear shared memory*/
    shmctl(clocKID, IPC_RMID, NULL);
    shmctl(pcbID, IPC_RMID, NULL);
    shmctl(semaphoreID, IPC_RMID, NULL);
    shmctl(descriptorID, IPC_RMID, NULL);
    msgctl(msqID, IPC_RMID, NULL);
    return 0;
}

/*Function to connect to shared memory and error check*/
MemoryClock *connectToClock(char *error) {
    char errorArr[200];
    long *sharedMemoryPtr;

    /*Get shared memory Id using shmget*/
    clocKID = shmget(CLOCKKEY, sizeof(memoryClock), 0777 | IPC_CREAT);

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

    return memoryClock;
}

/*Connect to a portion of shared memory for the process control blocks, create enough
 *  * space to hold 18 process control blocks*/
ProcessCtrlBlk *connectPcb(int totalpcbs, char *error) {
    char errorArr[200];

    pcbID = shmget(PCBKEY, totalpcbs * sizeof(ProcessCtrlBlk), 0777 | IPC_CREAT);
    if (pcbID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space for process control block",
                 errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to attach to shared memory process control block", errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return pcbTable;
}

/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {
    char errorArr[200];

    if ((msqID = msgget(MESSAGEKEY, 0777 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s Pid: %d Failed to connect to message queue", error, getpid());
        perror(errorArr);
        exit(1);
    }
}

void connectSemaphore(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((semaphoreID = shmget(SEMAPHOREKEY, sizeof(Semaphore), 0644 | IPC_CREAT)) == -1) {
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

void connectDescriptor(char *error){
    char errorArr[200];

    /* Getting the semaphore ID */
    if((descriptorID = shmget(DESCRIPTORKEY, sizeof(ResourceDiscriptor), 0644 | IPC_CREAT)) == -1){
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for resource descriptor", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if((resources = (ResourceDiscriptor*)shmat(descriptorID, (void *)0, 0)) == (void *)-1){
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for resource descriptor", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}

