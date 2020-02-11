
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include "FileOptionHandler.h"

#define SIZE 1000

struct queue {
    char* DirectoryNames[SIZE];
    int front;
    int rear;
};


/*Function to create queue structure*/
struct queue* createQueue() {
    struct queue* q = malloc(sizeof(struct queue));
    q->front = -1;
    q->rear = -1;
    return q;
}


bool checkLinkFlag(char [], int);
struct queue* createQueue();
void enqueue(struct queue* q, char *);
char* dequeue(struct queue* q);
int isEmpty(struct queue* q);


int isEmpty(struct queue* q) {
    if(q->rear == -1)
        return 1;
    else
        return 0;
}

/*Function to queue directory names into the queue*/
void enqueue(struct queue* q, char* dirName){
    if(q->rear == SIZE-1)
        printf("\nQueue is Full!!");
    else {
        if(q->front == -1)
            q->front = 0;
        q->rear++;
        q->DirectoryNames[q->rear] = dirName;

    }
}

/*Function to remove directory names from the queue*/
char* dequeue(struct queue* q){
    char* item;
    if(isEmpty(q)){
        item = " ";
    }
    else{
        item = q->DirectoryNames[q->front];
        q->front++;
        if(q->front > q->rear){
            q->front = q->rear = -1;
        }
    }

    return item;
}


void BFS(char* StartDirName, char options[], int argCounter, char* error){

    char* str = "";
    char path[1024];
    char* next = " ";
    struct queue* q = createQueue();
    struct stat sb;
    bool linkFlag = false;
    char* dirError;
    char* statError;
    int status;

    /*Check for symbolic link on file*/
    linkFlag = checkLinkFlag(options, argCounter);

    /*queue the starting directory name*/
    enqueue(q, StartDirName);

    /*Loop until queue is empty,
 *      * directories should be only thing in queue*/
    while(!isEmpty(q)){

        next = dequeue(q);

        DIR* d;

        /*Pointer for directory entry*/
        struct dirent *Entry;

        /*opendir returns a pointer of DIR type*/
        d = opendir(next);

        if(d == NULL){
            dirError = strcat(error, "Could not open current directory");
            perror(dirError);
            exit(EXIT_FAILURE);
        }

        /*Read all files or directories within current directory*/
        while((Entry = readdir(d)) != NULL){

            if ((strcmp(Entry->d_name, ".") == 0) || (strcmp(Entry->d_name, "..") == 0)) {
                continue;
            }

            snprintf(path, sizeof(path) - 1, "%s/%s", next, Entry->d_name);

            /*IF -L option given follow symbolic links
 *              * using stat, lstat returns information on
 *              * the link to the file stat returns information
 *              * on the actual file being linked to therefore it
 *              * follows the link*/
            /*need to add error handling here */
            if (linkFlag)
               status = stat(path, &sb);
            else
               status = lstat(path, &sb);

            if(status != 0){
                statError = strcat(error, "stat/lstat execution error: ");
                perror(statError);
                exit(EXIT_FAILURE);
            }

            /*POSIX way of checking file information*/
            switch (sb.st_mode & S_IFMT) {

                case S_IFDIR :
                    getOptions(path, options, argCounter);
                    printf("%s\n", path);

                    str = strdup(path);
                    enqueue(q, str);
                    break;

                case S_IFREG :
                    getOptions(path, options, argCounter);
                    printf("%s\n", path);
                    break;

                case S_IFLNK :
                    getOptions(path, options, argCounter);
                    printf("%s\n", path);
                    break;

                default:
                    getOptions(path, options, argCounter);
                    printf("%s\n", path);
                    break;
            }

        }
        closedir(d);
    }
    free(q);
}

bool checkLinkFlag(char options[], int counter){
    int i;
    bool flag = false;

    for(i = 0; i < counter; i++){
        if(options[i] == 'L'){
            flag = true;
        }
    }

    return flag;
}
