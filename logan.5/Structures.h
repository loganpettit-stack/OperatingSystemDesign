
#ifndef DEADLOCKDETECTION_STRUCTURES_H
#define DEADLOCKDETECTION_STRUCTURES_H

#include <semaphore.h>

#define totalPcbs 18
#define numResources 20
#define B 5000000

typedef struct {
    long seconds;
    long nanoseconds;
} MemoryClock;

typedef struct {
    int shmInt;
    sem_t mutex;
} Semaphore;

typedef struct {
    int front, rear, size;
    unsigned capacity;
    int* array;
} Queue;

typedef struct {
    long mesq_type;
    char mesq_text[100];
} MessageQ;

typedef struct {
    /*Identification*/
    int pid;
    int ppid;
    int uId;
    int gId;

} ProcessCtrlBlk;

typedef struct {
    /* Information about the resources */
    int requestMatrix[totalPcbs][numResources]; // Resources each process needs to complete
    int allocationMatrix[totalPcbs][numResources]; // The currently allocated resources to the processes
    int needMatrix[totalPcbs][numResources]; // what each process will need to be able to complete
    int resourceVector[numResources]; // Total resources in the system
    int allocationVector[numResources]; // Current resources avalible to be allocated
    int sharableResources[numResources]; // If 1, then that resource is sharable

    int request[totalPcbs];
    int allocate[totalPcbs];
    int release[totalPcbs];
    int terminating[totalPcbs];
    int suspended[totalPcbs];
    int timesChecked[totalPcbs];

} ResourceDiscriptor;

#endif //DEADLOCKDETECTION_STRUCTURES_H


