#ifndef MEMORYMANAGEMENT_STRUCTURES_H
#define MEMORYMANAGEMENT_STRUCTURES_H

#include <sys/msg.h>
#include <semaphore.h>
#define CLOCK_KEY 0x3574
#define SEM_KEY 0x1470
#define MSG_KEY 0x0752
#define MAX_CONCURRENT_PROCS 18
#define MAX_PROCS 100
#define MEM_SIZE 256
#define PT_SIZE 32
#define READ 0
#define WRITE 1

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
    long mesq_pid;
    int mesq_pctLocation;
    int mesq_terminate;
    int mesq_pageReference;
    int mesq_memoryAddress;
    int mesq_requestType;
    long mesq_sentSeconds;
    long mesq_sentNS;
} MESQ;


typedef struct {
    long pid;
    int pageTable[PT_SIZE];
} ProcessInfo;

typedef struct {
    int occupiedBit;
    int referenceByte;
    int dirtyBit;
    long pid;
    int processPageReference;
} Frame;


#endif //MEMORYMANAGEMENT_STRUCTURES_H

