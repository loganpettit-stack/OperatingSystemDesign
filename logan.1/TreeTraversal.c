
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define SIZE 50


struct queue {
    char* DirectoryNames[SIZE];
    int front;
    int rear;
};


struct queue* createQueue();
void enqueue(struct queue* q, char *);
char* dequeue(struct queue* q);
void display(struct queue* q);
int isEmpty(struct queue* q);
void printQueue(struct queue* q);

/*Function to create queue structure*/
struct queue* createQueue() {
    struct queue* q = malloc(sizeof(struct queue));
    q->front = -1;
    q->rear = -1;
    return q;
}


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


void BFS(char* StartDirName){
    
    char* str = " ";
    char path[1024];
    char* next = " ";
    struct queue* q = createQueue();
    struct stat sb;

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
            printf("Could not open current directory");
            return;
        }

        /*Read all files or directories within current directory*/
        while((Entry = readdir(d)) != NULL){

            if ((strcmp(Entry->d_name, ".") == 0) || (strcmp(Entry->d_name, "..") == 0)) {
                continue;
            }

            snprintf(path, sizeof(path) - 1, "%s/%s", next, Entry->d_name);

	    lstat(path, &sb);

            switch (sb.st_mode & S_IFMT) {

                case S_IFDIR :
                    printf("%s\n", path);
                    str = strdup(path);
                    enqueue(q, str);
                    break;

                case S_IFREG :
                    printf("%s\n", path);
                    break;

            }

        }

        closedir(d);
    }
}
