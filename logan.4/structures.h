
#ifndef SCHEDULER_STRUCTURES_H
#define SCHEDULER_STRUCTURES_H

typedef struct {
    int front, rear, size;
    unsigned capacity;
    int* array;
} Queue;

typedef struct {
    long seconds;
    long nanoseconds;
} MemoryClock;

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

    /*Scheduling data*/
    int readyState;
    int suspendedState;
    int blockedState;
    int quantum;
    int PCBtableLocation;
    int priority;
    int job;
    int cpuTimeUsed;
    int burstTime;
    int jobFinished;
    int jobType;
    int timeToUnblock;
    int processClass;
    long blockedTime;
    long seconds;
    long nanoseconds;
    long waitTime;
    long timeWaitedToLaunch;

} ProcessCtrlBlk;


#endif //SCHEDULER_STRUCTURES_H

