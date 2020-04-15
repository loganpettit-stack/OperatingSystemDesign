#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <stdbool.h>
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

long getTimeInNanoseconds();


int main(int argc, char *argv[]) {

    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];
    long resourceActivityTime;
    long processStartTimeNanos;
    long processEndTimeNanos;
    int tableLocation = atoi(argv[1]);
    bool flag = true;
    long currentTime;
    long timeForResourceActivity;
    long terminationChecktime;
    long nextCheckTermination;
    int terminationProbability;
    int resourceGrants = 0;

    /*Connect to shared memory*/
    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);
    connectPcb(numPCB, errorString);
    connectDescriptor(errorString);

    /*Get start time*/
    processStartTimeNanos = getTimeInNanoseconds();

    /*set one second threshold for process*/
    processEndTimeNanos = processStartTimeNanos + 1000000000;

    srand(getpid() * memoryClock->nanoseconds);

    /*generate time check in or out resources*/
    resourceActivityTime = (rand() % B);
    currentTime = getTimeInNanoseconds();
    timeForResourceActivity = resourceActivityTime + currentTime;


    /*Generating time to check when it should terminate*/
    terminationChecktime = (rand() % 250000000);
    nextCheckTermination = processEndTimeNanos;


    /*Process requests resources and populates
 *      * its request matrix*/
    int i;
    for(i = 0; i < numResources; i++){

        /*Get a random number of resources
 *          * only allow a process to grab up to 10
 *                   * instances of a single resource otherwise
 *                            * grab a random number of what is left*/
        if(resources->resourceVector[i] > 10){
            if(resources->resourceVector[i] > 10){
                resources->requestMatrix[tableLocation][i] = (rand () % 11);
            }
            else {
                resources->requestMatrix[tableLocation][i] = (rand() % resources->resourceVector[i] + 1);
            }
        }
    }

    /*Generate initial resources that the process will recieve*/
    for(i = 0; i < numResources; i++){

        /*skip resources that are not requested*/
        if(resources->requestMatrix[tableLocation][i] != 0){

            /*Request resources based on how much is left, if we are requesting more than what is left then
 *              * receive random amount from whats left otherwise recieve random amount of full request*/
            if(resources->requestMatrix[tableLocation][i] > resources->allocationVector[i]){
                resources->allocationMatrix[tableLocation][i] = (rand() % (resources->allocationVector[i] + 1));
            }
            else {
                resources->allocationMatrix[tableLocation][i] = (rand() % resources->requestMatrix[tableLocation][i] + 1);
            }

            /*Remove nonshareable resources from allocation vector, if sharable then provide the resource to
 *              * the process*/
            if(resources->sharableResources[i] != 1){
                resources->allocationVector[i] -= resources->allocationMatrix[tableLocation][i];
            } else {

                resources->allocationMatrix[tableLocation][i] = resources->requestMatrix[tableLocation][i];
            }
        }
    }




   while(flag){
        usleep(500);
        /*send message on mutex that we want
 *          * to enter the critical section*/
        sem_wait(&(semaphore->mutex));

        /*Check time to see if we are ready to do resource activities*/
        currentTime = getTimeInNanoseconds();
        if(timeForResourceActivity <= currentTime){

            if(currentTime >= nextCheckTermination){

                /*Generate random number between 0 - 100*/
                terminationProbability = (rand() % 101);

                /*Process will terminate if random number is
 *                  * less than equal to 20 making process terminate
 *                                   * 20% of the time*/
                if(terminationProbability <= 20){
                    resources->terminating[tableLocation] = 1;
                }

                nextCheckTermination = currentTime + terminationChecktime;
            }

            /*Process is not ready to terminate and
 *              * is granted resources*/
            else if(resources->allocate[tableLocation] == 1){
                resources->request[tableLocation] = 0;

                resourceGrants += 1;

                /*If the process is granted resources
 *                  * five times terminate*/
                if(resourceGrants >= 5){
                    resources->terminating[tableLocation] = 1;
                }
            }

            /*Process was terminated due to deadlock prevention*/
            else if(resources->release[tableLocation] == 1) {
                resources->terminating[tableLocation] = 1;
            }

            /*Process has not requested any resources yet*/
            else if (resources->request[tableLocation] == 0){
                resources->request[tableLocation] = 1;
            }

            /*Process is suspended and will wait for message to
 *              * wake up*/
            else if (resources->suspended[tableLocation] == 1){
                sem_post(&(semaphore->mutex));

                if(msgrcv(msqID, &mesq, sizeof(mesq), getpid(), MSG_NOERROR) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to receive message", errorString);
                    perror(errorArr);
                    exit(1);
                }

                continue;
            }
            /*Process requests resources, but they are have not
 *              * been granted*/
            else {

                resources->request[tableLocation] = 0;

                currentTime = getTimeInNanoseconds();
                timeForResourceActivity = currentTime + resourceActivityTime;

            }
        }

        /*Leave the critical section*/
        sem_post(&(semaphore->mutex));

        if(resources->terminating[tableLocation] == 1){
            flag = false;
        }
   }


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

void connectDescriptor(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((descriptorID = shmget(DESCRIPTORKEY, sizeof(ResourceDiscriptor), 0644 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200,
                 "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for resource descriptor", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if ((resources = (ResourceDiscriptor *) shmat(descriptorID, (void *) 0, 0)) == (void *) -1) {
        snprintf(errorArr, 200,
                 "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for resource descriptor", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}

/*Return time in nanoseconds*/
long getTimeInNanoseconds() {
    long secondsToNanoseconds = 0;
    long currentTimeInNanos;
    secondsToNanoseconds = memoryClock->seconds * 1000000000;
    currentTimeInNanos = secondsToNanoseconds + memoryClock->nanoseconds;

    return currentTimeInNanos;
}

