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
#include <setjmp.h>
#include <signal.h>


#define CLOCKKEY 0x3574
#define SEMAPHOREKEY 0x1470
#define MESSAGEKEY 0x3596
#define PCBKEY 0x1596
#define DESCRIPTORKEY 0x0752

MemoryClock *memoryClock;
Semaphore *semaphore;
MessageQ mesq;
ResourceDiscriptor *resources;
ProcessCtrlBlk *pcbTable;

int clocKID;
int pcbID;
int semaphoreID;
int descriptorID;
int msqID;

jmp_buf buf;

MemoryClock *connectToClock(char *);

void connectToMessageQueue(char *);

ProcessCtrlBlk *connectPcb(int, char *);

void connectSemaphore(char *);

void connectDescriptor(char *);

long getTimeInNanoseconds();

void generateRequests(int);

void addTime(long);

void getResources(int);

int checkRequestsSatisfied(int);

void killHandler(int );

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


    /* Signal Handling */
    signal(SIGINT, killHandler);
    signal(SIGTSTP, killHandler);

    if (setjmp(buf) == 1) {
        snprintf(errorArr, 200, "\n\n%s: Interrupt signal was sent, cleaning up memory segments ", errorString);
        perror(errorArr);

        /* Detaching shared memory */
        shmdt(pcbTable);
        shmdt(memoryClock);
        shmdt(semaphore);
        shmdt(resources);

        exit(EXIT_SUCCESS);
    }

    /*Connect to shared memory*/
    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);
    connectPcb(totalPcbs, errorString);
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
    sem_wait(&(semaphore->mutex));

    /*Generate initial resource requests*/
    generateRequests(tableLocation);


    /*Generate initial resources that the process will recieve*/
    getResources(tableLocation);

    addTime(1000);

    sem_post(&(semaphore->mutex));

    usleep(20000);

    while (flag) {
        usleep(2000);
        /*send message on mutex that we want
 *          * to enter the critical section*/
        sem_wait(&(semaphore->mutex));


        /*Check time to see if we are ready to do resource activities*/
        currentTime = getTimeInNanoseconds();


        if (timeForResourceActivity <= currentTime) {

            /*Check if process should terminate*/
            if (currentTime >= nextCheckTermination) {

                /*Generate random number between 0 - 100*/
                terminationProbability = (rand() % 101);

                /*Process will terminate if random number is
 *                  * less than equal to 20 making process terminate
 *                                   * 20% of the time*/
                if (terminationProbability <= 30) {
                    resources->terminating[tableLocation] = 1;
                }

                nextCheckTermination = currentTime + terminationChecktime;
            }


            /*If process decided not to terminate request resources
 *              * or allocate them*/
            if (resources->terminating[tableLocation] == 0) {
                /*If process granted a request for resources allow it
 *                  * to grab more resources and */
                if (resources->allocate[tableLocation] == 1) {
                    resources->allocate[tableLocation] = 0;
                    getResources(tableLocation);

                    int check = checkRequestsSatisfied(tableLocation);

                    if (check == 0) {
                        resources->terminating[tableLocation] = 1;
                    }
                }

                    /*If a process is in a deadlock and told to release it's resources*/
                else if (resources->release[tableLocation] == 1) {
                    resources->terminating[tableLocation] = 1;
                }
                    /*Otherwise request the rest of the resouces this process requires*/
                else {
                    resources->request[tableLocation] = 1;
                    sem_post(&(semaphore->mutex));

                    /*wait to recieve message from parent when granted to get more resources*/
                    if (msgrcv(msqID, &mesq, sizeof(mesq), getpid(), MSG_NOERROR) == -1) {
                        snprintf(errorArr, 200, "\n%s User failed to receive message", errorString);
                        perror(errorArr);
                        exit(1);
                    }
                }

                timeForResourceActivity = resourceActivityTime + currentTime;
            }
        }

        /*Leave the critical section*/
        sem_post(&(semaphore->mutex));

        if (resources->terminating[tableLocation] == 1) {
            flag = false;
        }

        addTime(10000);
    }


    /*Clear shared memory*/
    shmdt(pcbTable);
    shmdt(memoryClock);
    shmdt(semaphore);
    shmdt(resources);
    return 0;
}

int checkRequestsSatisfied(int tableLocation) {
    /*Check if all resource grants have been made
 *       * if so the process has terminated*/
    int k;
    int resourceCheck = 0;
    for (k = 0; k < numResources; k++) {
        resourceCheck += resources->requestMatrix[tableLocation][k];
    }

    return resourceCheck;
}

void generateRequests(int tableLocation) {
    int i;
    for (i = 0; i < numResources; i++) {

        /*Get a random number of resources
 *          * only allow a process to grab up to 10
 *                   * instances of a single resource otherwise
 *                            * grab a random number of what is left*/
        if (resources->resourceVector[i] > 10) {
            resources->requestMatrix[tableLocation][i] = (rand() % 11);
        } else {
            resources->requestMatrix[tableLocation][i] = (rand() % resources->resourceVector[i] + 1);
        }
    }
}

void getResources(int tableLocation) {

    int tempVector[numResources] = {0};

  /*  printf("\n\nuser process: %d current requests: \n", getpid());
 *      int h;
 *          for (h = 0; h < numResources; h++) {
 *                  printf(" %d ", resources->requestMatrix[tableLocation][h]);
 *                      }
 *
 *                          printf("\n\n userProcess: %d current holdings: \n", getpid());
 *                              for (h = 0; h < numResources; h++) {
 *                                      printf(" %d ", resources->allocationMatrix[tableLocation][h]);
 *                                          }
 *
 *
 *                                              printf("\nuser process: Resources that can be allocated for %d\n", getpid());
 *                                                  int y;
 *                                                      for (y = 0; y < numResources; y++) {
 *                                                              printf(" %d ", resources->allocationVector[y]);
 *                                                                  }*/



    int i;
    for (i = 0; i < numResources; i++) {

        /*skip resources that are not requested*/
        if (resources->requestMatrix[tableLocation][i] != 0) {

            /*Request resources based on how much is left, if we are requesting more than what is left then
 *              * receive random amount from whats left otherwise recieve random amount of full request*/
            if (resources->requestMatrix[tableLocation][i] > resources->allocationVector[i]) {
                tempVector[i] = (rand() % (resources->allocationVector[i] + 1));
                resources->allocationMatrix[tableLocation][i] += tempVector [i];
            } else {
                tempVector[i] = (rand() % resources->requestMatrix[tableLocation][i] + 1);
                resources->allocationMatrix[tableLocation][i] += tempVector[i];
            }

            /*Remove nonshareable resources from allocation vector to show they are no longer available,
 *              * if sharable then provide the resource to the process update processes request matrix
 *                           * to what it will still need*/
            if (resources->sharableResources[i] != 1) {
                resources->allocationVector[i] -= tempVector[i];
                /*Remove resources that have been allocated from this processes requests*/
                resources->requestMatrix[tableLocation][i] -= tempVector[i];
            } else {

                resources->requestMatrix[tableLocation][i] -= tempVector[i];

            }
        }

        addTime(1000);
    }

  /*  printf("\nsharable resources \n");
 *      for (y = 0; y < numResources; y++) {
 *              printf(" %d ", resources->sharableResources[y]);
 *                  }
 *                      printf("\n\nuser process allocations after reciving resources %d \n", getpid());
 *                          for (y = 0; y < numResources; y++) {
 *                                  printf(" %d ", resources->allocationMatrix[tableLocation][y]);
 *                                      }
 *                                          printf("\nAllocation vector after for %d\n", getpid());
 *                                              for (y = 0; y < numResources; y++) {
 *                                                      printf(" %d ", resources->allocationVector[y]);
 *                                                          }
 *                                                              printf("\nresources still left to request for %d\n", getpid());
 *                                                                  for (y = 0; y < numResources; y++) {
 *                                                                          printf(" %d ", resources->requestMatrix[tableLocation][y]);
 *                                                                              }*/

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

void killHandler(int dummy) { longjmp(buf, 1); }
