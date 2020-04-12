#ifndef DEADLOCKDETECTION_STRUCTURES_H
#define DEADLOCKDETECTION_STRUCTURES_H

#include <semaphore.h>

#define numPCB 18
#define numResources 20

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
    int request_matrix[numPCB][numResources]; // Resources each process needs to complete
    int allocation_matrix[numPCB][numResources]; // The currently allocated resources to the processes
    int need_matrix[numPCB][numResources]; // what each process will need to be able to complete
    int resource_vector[numResources]; // Total resources in the system
    int allocation_vector[numResources]; // Current resources avalible to be allocated
    int sharable_resources[numResources]; // If 1, then that resource is sharable

    int request[numPCB];
    int allocate[numPCB];
    int release[numPCB];
    int terminating[numPCB];
    int suspended[numPCB];
    int timesChecked[numPCB];

} ResourceDiscriptor;

#endif //DEADLOCKDETECTION_STRUCTURES_H


