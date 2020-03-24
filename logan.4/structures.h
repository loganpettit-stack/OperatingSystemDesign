
#ifndef SCHEDULER_STRUCTURES_H
#define SCHEDULER_STRUCTURES_H

typedef struct {
    int front, rear, size;
    unsigned capacity;
    int* array;
} Queue;

typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
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

    /*Scheduling
 *     int priority;
 *         int pctLocation;
 *             unsigned int timeCreatedSecs;
 *                 unsigned int timeCreatedNS;
 *                     unsigned int dispatchTime;
 *                         unsigned int lastWaitTimeSecs;
 *                             unsigned int lastWaitTimeNS;
 *                                 unsigned int totalWaitTimeSecs;
 *                                     unsigned int totalWaitTimeNS;
 *                                         unsigned int lastTimeAccessedSecs;
 *                                             unsigned int lastTimeAccessedNS;
 *                                                 unsigned int cpuTimeUsed;
 *                                                     unsigned int systemTimeUsedSecs;
 *                                                         unsigned int systemTimeUsedNS;
 *                                                             unsigned int lastBurstDuration;
 *                                                                 int quantum;
 *                                                                     int task;
 *                                                                         int ready;
 *                                                                             int suspend;
 *                                                                                 int blocked;
 *                                                                                     unsigned int timeToUnblockSecs;
 *                                                                                         unsigned int timeToUnblockNS;
 *                                                                                             int queueLooper;
 *                                                                                                 int completed;*/
} ProcessCtrlBlk;


#endif //SCHEDULER_STRUCTURES_H

