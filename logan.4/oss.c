#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>
#include <setjmp.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include "structures.h"
#include <sys/msg.h>

#define PCBKEY 0x1596
#define MESSAGEKEY 0x1470
#define CLOCKKEY 0x3574

jmp_buf fileLinejmp;
jmp_buf ctrlCjmp;
jmp_buf timerjmp;

/*Structure Id's*/
int pcbID;
int msqID;
int clocKID;

int fileFlag = 0;
char *fileName = "output.dat";
int lineCount = 0;
long cycles = 0;
double totalCPUUtilization = 0;
int totalLaunches = 0;
double timeWaitedToLaunch = 0;
int totalBlockedProcesses = 0;
double blockedWaitTime = 0;
int runningProcesses = 0;
int totalProcesses = 0;
long nextLaunchTime;
long idleTime;

/*pointers to shared memory
 *  * structures*/
MessageQ msq;
MemoryClock *memoryClock;
ProcessCtrlBlk *pcbTable;

void connectToMessageQueue(char *);

MemoryClock *connectToClock(char *);

pid_t launchProcess(char *, int);

void ctrlCHandler();

void timerHandler();

unsigned int startTimer(int);

ProcessCtrlBlk *connectPcb(int, char *);

int findTableLocation(const int[], int);

Queue *createQueue(unsigned int);

void enqueue(Queue *, int);

int dequeue(Queue *);

void initializePCB(int);

void scheduleProcess(Queue *, Queue *, Queue *, Queue *, Queue *, Queue *, int[], char *, FILE *);

long getTimeInNanoseconds(long, long);

void addTime(long nanoseconds);

int isEmpty(Queue *);

void dispatchProcess(int, Queue *, Queue *, Queue *, Queue *, Queue *, Queue *, int[], char *, long, FILE *);

int main(int argc, char *argv[]) {

    const unsigned int maxTimeBetweenNewProcsSecs = 1;
    const unsigned int maxTimeBetweenNewProcsNS = 10000000;

    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];
    FILE *outputLog;
    long initializeScheduler = 7000000000;
    int seconds = 3;
    int randomNano;
    int bitVector[18] = {0};
    int openTableLocation;
    int flag = 1;
    int sec = 0;
    long nsec = 0;
    double avgwait = 0;
    int totalPCBs = 18;
    long currentTime = 0;
    long idleStart = 0;
    int process = 0;
    Queue *tempQueue = createQueue(18);
    Queue *queue0 = createQueue(18);
    Queue *queue1 = createQueue(18);
    Queue *queue2 = createQueue(18);
    Queue *queue3 = createQueue(18);
    Queue *blockedQueue = createQueue(18);

    outputLog = fopen(fileName, "w");

    /*Able to represent process ID*/
    pid_t pid = 0;

    /*Catch ctrl+c signal and handle with
 *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler */
    signal(SIGALRM, timerHandler);

    /*Jump back to main enviroment where Concurrentchildren are launched
 *     * but before the wait*/
    if (setjmp(ctrlCjmp) == 1) {

        /*Attempt to output statistics if it ran for long enough*/
        avgwait = blockedWaitTime / totalBlockedProcesses;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        } else {
            sec = 0;
            nsec = (long)avgwait;
        }
        fprintf(outputLog, "\n\nAverage time a process spent blocked: %d:%ld  \n", sec, nsec);

        avgwait = timeWaitedToLaunch / totalLaunches;
        if (avgwait > 1000000000) {
            sec = (long)avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        } else {
            sec = 0;
            nsec = (long)avgwait;
        }
        fprintf(outputLog, "Average time a process waited to be launched: %d:%ld\n", sec, nsec);

        avgwait = idleTime;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        }
        fprintf(outputLog, "CPU was idle with no processes for: %d:%ld \n", sec, nsec);
        fprintf(outputLog, "Average CPU utilization %f%% \n", totalCPUUtilization / cycles * 100);

        /*Empty all queues*/
        while(!isEmpty(queue0)){
            process = dequeue(queue0);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while (!isEmpty(queue1)) {
            process = dequeue(queue1);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(queue2)){
            process = dequeue(queue2);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(queue3)){
            process = dequeue(queue3);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(blockedQueue)){
            process = dequeue(blockedQueue);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }

        /*Detach clock from shared memory*/
        if (shmdt(memoryClock) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }


        /*Detach process control block shared memory*/
        if (shmdt(pcbTable) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear shared memory*/
        fclose(outputLog);
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }

    if (setjmp(fileLinejmp) == 1) {

        fprintf(stderr, "OSS: log file filled up with over 10000 lines, stopping program\n");

        /*Attempt to output statistics if it ran for long enough*/
        avgwait = blockedWaitTime / totalBlockedProcesses;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        } else {
            sec = 0;
            nsec = (long)avgwait;
        }
        fprintf(outputLog, "\n\nAverage time a process spent blocked: %d:%ld  \n", sec, nsec);

        avgwait = timeWaitedToLaunch / totalLaunches;
        if (avgwait > 1000000000) {
            sec = (long)avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        } else {
            sec = 0;
            nsec = (long)avgwait;
        }
        fprintf(outputLog, "Average time a process waited to be launched: %d:%ld\n", sec, nsec);

        avgwait = idleTime;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        }
        fprintf(outputLog, "CPU was idle with no processes for: %d:%ld \n", sec, nsec);
        fprintf(outputLog, "Average CPU utilization %f%% \n", totalCPUUtilization / cycles * 100);

        /*Empty all queues*/
        while(!isEmpty(queue0)){
            process = dequeue(queue0);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while (!isEmpty(queue1)) {
            process = dequeue(queue1);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(queue2)){
            process = dequeue(queue2);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(queue3)){
            process = dequeue(queue3);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(blockedQueue)){
            process = dequeue(blockedQueue);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }

        /*Detach clock from shared memory*/
        if (shmdt(memoryClock) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }


        /*Detach process control block shared memory*/
        if (shmdt(pcbTable) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear shared memory*/
        fclose(outputLog);
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }



    if (setjmp(timerjmp) == 1) {
        avgwait = blockedWaitTime / totalBlockedProcesses;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        } else {
            sec = 0;
            nsec = (long)avgwait;
        }
        fprintf(outputLog, "\n\nAverage time a process spent blocked: %d:%ld  \n", sec, nsec);

        avgwait = timeWaitedToLaunch / totalLaunches;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        } else {
            sec = 0;
            nsec = avgwait;
        }
        fprintf(outputLog, "Average time a process waited to be launched: %d:%ld\n", sec, nsec);

        avgwait = idleTime;
        if (avgwait > 1000000000) {
            sec = avgwait / 1000000000;
            nsec = (long) avgwait - (sec * 1000000000);
        }
        fprintf(outputLog, "CPU was idle with no processes for: %d:%ld \n", sec, nsec);
        fprintf(outputLog, "Average CPU utilization %f%% \n", totalCPUUtilization / cycles * 100);


        while(!isEmpty(queue0)){
            process = dequeue(queue0);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while (!isEmpty(queue1)) {
            process = dequeue(queue1);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(queue2)){
            process = dequeue(queue2);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(queue3)){
            process = dequeue(queue3);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }
        while(!isEmpty(blockedQueue)){
            process = dequeue(blockedQueue);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }

        /*Detach clock from shared memory*/
        if (shmdt(memoryClock) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Detach process control block shared memory*/
        if (shmdt(pcbTable) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear shared memory*/
        fclose(outputLog);
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }





    startTimer(seconds);

    /*Connect and create pcbtable in shared memory*/
    pcbTable = connectPcb(totalPCBs, errorString);

    /*Connect clock to shared memory*/
    connectToClock(errorString);

    /*Connect message queue*/
    connectToMessageQueue(errorString);

    memoryClock->seconds = 0;
    memoryClock->nanoseconds = 100;

    currentTime = (memoryClock->seconds, memoryClock->nanoseconds);
    /*get a starting idle time */
    if (isEmpty(queue0) && isEmpty(queue1) && isEmpty(queue2) && isEmpty(queue3) && isEmpty(blockedQueue)) {
        idleStart = currentTime;
    }

    /*Get next random time a process should launch in nanoseconds*/
    srand(time(0));
    randomNano = (rand() % maxTimeBetweenNewProcsNS + 1);
    nextLaunchTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds + randomNano);


    nextLaunchTime = 0;
    /*Launch processes until 100 is reached*/
    while (flag) {

        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);

        /* printf("Current time: %ld\n", currentTime);*/
        /*Only allow a certian number of concurrent processes*/
        if (runningProcesses <= 18 && currentTime > nextLaunchTime && totalProcesses < 100) {

            /*Start launching processes, find an open space in pcb table first*/
            if ((openTableLocation = findTableLocation(bitVector, totalPCBs)) != -1) {

                /*Launch process*/
                pid = launchProcess(errorString, openTableLocation);

                srand(getppid() * memoryClock->nanoseconds);

                int classProbability = (rand() % 100 + 1);
                /*Basically saying less than 10% of the time we will
 *                 * have a process be real-time class*/
                if (classProbability < 20) {
                    fprintf(outputLog,
                            "OSS: generated process with PID %d as real-time process and placed it in queue 0 at time %ld:%ld\n",
                            pcbTable[openTableLocation].pid, memoryClock->seconds, memoryClock->nanoseconds);
                    lineCount += 1;
                    pcbTable[openTableLocation].processClass = 0;
                } else {
                    fprintf(outputLog,
                            "OSS: generated process with PID %d as a user process and placed it in queue 1 at time %ld:%ld\n",
                            pcbTable[openTableLocation].pid, memoryClock->seconds, memoryClock->nanoseconds);
                    lineCount += 1;
                    pcbTable[openTableLocation].processClass = 1;
                }

                usleep(1000);

                pcbTable[openTableLocation].timeWaitedToLaunch = currentTime;
                runningProcesses += 1;
                totalProcesses += 1;

                /*Generate next process launch time*/
                srand(time(0));
                randomNano = (rand() % maxTimeBetweenNewProcsNS + 1);
                nextLaunchTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds + randomNano);

                /*printf("next launch time: %ld\n", nextLaunchTime);*/

                /*Queue all processes into the first queue*/
                enqueue(tempQueue, openTableLocation);

                /*Show that a process is occupying a space in
 *                  * the bitvector which will directly relate to the pcb table*/
                bitVector[openTableLocation] = 1;

                /* printf("\nOSS: generating process with PID: %d, Putting into Queue 1 at time: %ld:%ld totalproccesses: %d.\n",
 *                         pid, memoryClock->seconds, memoryClock->nanoseconds, totalProcesses);*/

                addTime(100);
            }
        }

        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);
        if (isEmpty(queue0) && isEmpty(queue1) && isEmpty(queue2) && isEmpty(queue3) && isEmpty(blockedQueue)) {
            idleTime += currentTime - idleStart;
            idleStart = currentTime;
        }

        /* printf("\n\n\n\n concurrent procs: %f util: %f  \n\n\n",concurrentProcesses, cpuUtilization);*/

        /*Let initial processes launch to start system up */
        if (currentTime > initializeScheduler) {
            scheduleProcess(tempQueue, queue0, queue1, queue2, queue3, blockedQueue, bitVector, errorString, outputLog);

            cycles += 1;
            double concurrentProcesses = runningProcesses;
            double totalprocs = 18;
            double cpuUtilization = concurrentProcesses / totalprocs;
            totalCPUUtilization += cpuUtilization;
        }

        if (runningProcesses == 0 && totalProcesses == 100) {
            flag = 0;
        }


        if(lineCount >= 10000){
            longjmp(fileLinejmp, 1);
        }
        addTime(100000);
    }


    fprintf(stderr, "Program exited after running %d processes data stored to %s\n\n", totalProcesses, fileName);


    /*detach structures from shared memory*/
    if (shmdt(memoryClock) == -1) {
        snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }


    if (shmdt(pcbTable) == -1) {
        snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Clear shared memory*/
    shmctl(clocKID, IPC_RMID, NULL);
    shmctl(pcbID, IPC_RMID, NULL);
    msgctl(msqID, IPC_RMID, NULL);


    avgwait = blockedWaitTime / totalBlockedProcesses;
    if (avgwait > 1000000000) {
        sec = avgwait / 1000000000;
        nsec = (long) avgwait - (sec * 1000000000);
    } else {
        sec = 0;
        nsec = (long)avgwait;
    }
    fprintf(outputLog, "\n\nAverage time a process spent blocked: %d:%ld  \n", sec, nsec);

    avgwait = timeWaitedToLaunch / totalLaunches;
    if (avgwait > 1000000000) {
        sec = avgwait / 1000000000;
        nsec = (long) avgwait - (sec * 1000000000);
    } else {
        sec = 0;
        nsec = (long)avgwait;
    }
    fprintf(outputLog, "Average time a process waited to be launched: %d:%ld\n", sec, nsec);

    avgwait = idleTime;
    if (avgwait > 1000000000) {
        sec = avgwait / 1000000000;
        nsec = (long) avgwait - (sec * 1000000000);
    }
    fprintf(outputLog, "CPU was idle with no processes for: %d:%ld \n", sec, nsec);
    fprintf(outputLog, "Average CPU utilization %f%% \n", totalCPUUtilization / cycles * 100);

    fclose(outputLog);

    return 0;
}

void
scheduleProcess(Queue *tq, Queue *q0, Queue *q1, Queue *q2, Queue *q3, Queue *bq, int bitVector[], char *errorString,
                FILE *outputLog) {
    long currentTime;
    long quantum = 1000000;
    int blockedprocess;
    int processToEvaluate;
    long q0Quantum = quantum;
    long q1Quantum = quantum;
    long q2Quantum = quantum / 2;
    long q3Quantum = quantum / 4;



    /*Remove processes from queue based on type and
 *      * distribute them to appropriate queue*/
    while (!isEmpty(tq)) {
        processToEvaluate = dequeue(tq);

        if (pcbTable[processToEvaluate].processClass == 0) {
            enqueue(q0, processToEvaluate);
        } else {
            enqueue(q1, processToEvaluate);
        }
    }

    /*Check if blocked queue times are up and move them
 *      * to the highest priortiy queue*/
    currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);
    int i = 0;
    while (i < bq->size) {
        blockedprocess = dequeue(bq);
        /*printf("Checking blocked process: %d\n", pcbTable[blockedprocess].pid);*/

        if (pcbTable[blockedprocess].blockedTime < currentTime) {
            if (pcbTable[blockedprocess].processClass == 0) {
                fprintf(outputLog, "OSS: Process %d has became unblocked and is being placed back into queue 0\n",
                        pcbTable[blockedprocess].pid);
                lineCount += 1;
                blockedWaitTime += currentTime - pcbTable[blockedprocess].waitTime;
                totalBlockedProcesses += 1;
                enqueue(q0, blockedprocess);
                addTime(100);
            } else {
                fprintf(outputLog, "OSS: Process %d has became unblocked and is being placed into queue 1\n",
                        pcbTable[blockedprocess].pid);
                lineCount += 1;
                blockedWaitTime += currentTime - pcbTable[blockedprocess].waitTime;
                totalBlockedProcesses += 1;
                enqueue(q1, blockedprocess);
                addTime(100);
            }

            i -= 1;
        }
        i += 1;
    }

    addTime(10000);

    /*Dispatch processes in queues starting with highest priority
 *      * until they are empty, Break out of loop to re launch a process if
 *           * concurrent processes falls under 10 finish to keep processes coming into the system*/
    while (!isEmpty(q0)) {
        dispatchProcess(0, q0, q0, q1, q2, q3, bq, bitVector, errorString, q0Quantum, outputLog);


        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);
        if (runningProcesses < 10 && nextLaunchTime < currentTime) {
            break;
        }
    }

    while (!isEmpty(q1)) {
        dispatchProcess(1, q1, q0, q1, q2, q3, bq, bitVector, errorString, q1Quantum, outputLog);

        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);
        if (runningProcesses < 5 && nextLaunchTime < currentTime) {
            break;
        }
    }

    while (!isEmpty(q2)) {
        dispatchProcess(2, q2, q0, q1, q2, q3, bq, bitVector, errorString, q2Quantum, outputLog);

        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);
        if (runningProcesses < 5 && nextLaunchTime < currentTime) {
            break;
        }
    }

    while (!isEmpty(q3)) {
        dispatchProcess(3, q3, q0, q1, q2, q3, bq, bitVector, errorString, q3Quantum, outputLog);

        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);
        if (runningProcesses < 5 && nextLaunchTime < currentTime) {
            break;
        }
    }


}

void dispatchProcess(int Qnum, Queue *dispatchQ, Queue *q0, Queue *q1, Queue *q2, Queue *q3, Queue *bq, int bitVector[],
                     char *errorString, long quantum, FILE *outputLog) {
    int statusCode = 0;
    char errorArr[200];
    char message[20];
    int pid;
    long currentTime;
    int processToLaunch;
    processToLaunch = dequeue(dispatchQ);
    pid = pcbTable[processToLaunch].pid;

    currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);

    if (pcbTable[processToLaunch].processClass == 0) {
        fprintf(outputLog, "OSS: Dispatching real-time process %d from queue %d at time %ld:%ld\n",
                 pid, Qnum, memoryClock->seconds, memoryClock->nanoseconds);
        lineCount += 1;
        timeWaitedToLaunch += currentTime - pcbTable[processToLaunch].timeWaitedToLaunch;
        pcbTable[processToLaunch].timeWaitedToLaunch = currentTime;
        totalLaunches += 1;
        addTime(100);
    } else {
        fprintf(outputLog, "OSS: Dispatching user process %d from queue %d at time %ld:%ld\n",
                 pid, Qnum, memoryClock->seconds, memoryClock->nanoseconds);
        lineCount += 1;
        timeWaitedToLaunch = currentTime - pcbTable[processToLaunch].timeWaitedToLaunch;
        pcbTable[processToLaunch].timeWaitedToLaunch = currentTime;
        totalLaunches += 1;
        addTime(100);
    }

    msq.mesq_type = (long) pid;

    /*Send time quantum to process and signal it to start doing work*/
    snprintf(message, 20, "%ld", quantum);
    strcpy(msq.mesq_text, message);
    if (msgsnd(msqID, &msq, sizeof(msq), MSG_NOERROR) == -1) {
        snprintf(errorArr, 200, "%s failed to send message in scheduler", errorString);
        perror(errorArr);
        exit(1);
    }


    /*When process sends back */
    if (msgrcv(msqID, &msq, sizeof(msq), getpid(), MSG_NOERROR) == -1) {
        snprintf(errorArr, 200, "%s failed to recieve message in scheduler", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }



    fprintf(outputLog, "OSS: %s\n", msq.mesq_text);
    lineCount += 1;



    /*if process job finished parent signals to kill the process*/
    if (pcbTable[processToLaunch].jobFinished == 1) {
        pid = wait(&statusCode);

        runningProcesses -= 1;
        bitVector[processToLaunch] = 0;

    } else if (pcbTable[processToLaunch].jobType == 1) {


        /*if a process runs for its full quantum requeue it
 *          * into the next lower queue or if it is a real-time
 *                   * process requeue to q0*/
        if (Qnum == 0) {
            fprintf(outputLog, "OSS: process with PID %d being requeued to queue 0\n", pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q0, processToLaunch);
        } else if (Qnum == 1) {
            fprintf(outputLog, "OSS: process with PID %d being requeued to queue 2\n", pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q2, processToLaunch);
        } else {
            fprintf(outputLog, "OSS: process with PID %d being requeued to queue 3\n", pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q3, processToLaunch);
        }


    } else if (pcbTable[processToLaunch].jobType == 2) {
        /*Process has been preempted and will go back to its same queue to be
 *          * relaunched and finish its full quantum at the same priority*/
        if (Qnum == 0) {
            fprintf(outputLog, "OSS: Process with PID %d was preempted and replaced back in queue 0\n",
                    pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q0, processToLaunch);
        } else if (Qnum == 1) {
            fprintf(outputLog, "OSS: Process with PID %d was preempted and replaced back in queue 1\n",
                    pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q1, processToLaunch);
        } else if (Qnum == 2) {
            fprintf(outputLog, "OSS: Process with PID %d was preempted and replaced back in queue 2\n",
                    pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q2, processToLaunch);
        } else if (Qnum == 3) {
            fprintf(outputLog, "OSS: Process with PID %d was preempted and replaced back in queue 3\n",
                    pcbTable[processToLaunch].pid);
            lineCount += 1;
            enqueue(q3, processToLaunch);
        }

    } else {
        /*The process has ran for a portion of its quantum and has become blocked
 *          * Place in the blocked queue until for a certain amount of time.*/
        fprintf(outputLog, "OSS: Process with PID %d was blocked and being placed in the blocked queue\n",
                pcbTable[processToLaunch].pid);
        enqueue(bq, processToLaunch);
        lineCount += 1;
    }

    addTime(10000);


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

/*Return time in nanoseconds*/
long getTimeInNanoseconds(long seconds, long nanoseconds) {
    long secondsToNanoseconds = 0;
    secondsToNanoseconds = seconds * 1000000000;
    nanoseconds += secondsToNanoseconds;

    return nanoseconds;
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


/*Function to find an open location in the bit vector which will correspond to the spot
 *  * in the pcb table*/
int findTableLocation(const int bitVector[], int totalPCBs) {
    int i;

    for (i = 0; i < totalPCBs; i++) {
        if (bitVector[i] == 0) {
            return i;
        }
    }

    return -1;
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


/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {
    char errorArr[200];

    if ((msqID = msgget(MESSAGEKEY, 0777 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s Pid: %d Failed to connect to message queue", error, getpid());
        perror(errorArr);
        exit(1);
    }
}

/*Function to launch child processes and return the corresponding pid's*/
pid_t launchProcess(char *error, int openTableLocation) {
    char errorArr[200];
    pid_t pid;

    char tableLocationString[20];

    snprintf(tableLocationString, 20, "%d", openTableLocation);


    /*Fork process and capture pid*/
    pid = fork();

    if (pid == -1) {
        snprintf(errorArr, 200, "\n\n%s Fork execution failure", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /*Set initial values of process control block of process*/
        initializePCB(openTableLocation);

        /*executes files, Name of file to be execute, list of args following,
 *          * must be terminated with null*/
        execl("./userProcess", "userProcess", tableLocationString, NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);

    } else {
        usleep(5000);
        return pid;
    }

}

void initializePCB(int tableLocation) {
    pcbTable[tableLocation].pid = getpid();
    pcbTable[tableLocation].ppid = getppid();
    pcbTable[tableLocation].uId = getuid();
    pcbTable[tableLocation].gId = getgid();

}


/**Implementation from geeksforgeeks.com**/
Queue *createQueue(unsigned capacity) {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (int *) malloc(queue->capacity * sizeof(int));
    return queue;
}

/*Queue is full when size becomes equal to the capacity*/
int isFull(Queue *queue) { return (queue->size == queue->capacity); }

/* Queue is empty when size is 0*/
int isEmpty(Queue *queue) { return (queue->size == 0); }

/* Function to add an item to the queue.
 *  * It changes rear and size*/
void enqueue(Queue *queue, int item) {
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;

    /* printf("%d enqueued to queue\n", pcbTable[item].pid);*/
}

/*Function to remove an item from queue.
 *  * It changes front and size*/
int dequeue(Queue *queue) {
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

/* Function to get front of queue*/
int front(Queue *queue) {
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}

/* Function to get rear of queue*/
int rear(Queue *queue) {
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
}


/*Timer function that sends alarm defined by GNU*/
unsigned int startTimer(int seconds) {
    struct itimerval old, new;
    new.it_interval.tv_usec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = 0;
    new.it_value.tv_sec = (long int) seconds;
    if (setitimer(ITIMER_REAL, &new, &old) < 0)
        return 0;
    else
        return old.it_value.tv_sec;
}

/*Function handles ctrl c signal and jumps to top
 *  * of program stack enviroment*/
void ctrlCHandler() {
    char errorArr[200];
    snprintf(errorArr, 200, "\n\nCTRL+C signal caught, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(ctrlCjmp, 1);
}

/*Function handles timer alarm signal and jumps
 *  * to top of program stack enviroment*/
void timerHandler() {
    char errorArr[200];
    snprintf(errorArr, 200, "\n\ntimer interrupt triggered, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(timerjmp, 1);
}
