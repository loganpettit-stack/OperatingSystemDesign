#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

void getPermissions(int, char[]);

char* getOptions(char* path, char optionsArray[], int argCounter) {

    char fileInformation[1024];
    char permissions[10];
    int i = 0;
    struct stat sb;

    lstat(path, &sb);


    for (i; i < argCounter; i++) {
        char opt = optionsArray[i];

        switch (opt) {
            case 'p' :

                getPermissions(sb.st_mode, permissions);

                printf("%-12s", permissions);

                break;
        }
    }
}

void getPermissions(int fileMode, char permissions[]){

    strcpy(permissions, "---------");

    strcpy(permissions, "---------");

    /*User Permissions*/
    if(fileMode & S_IRUSR) permissions[0] = 'r';
    if(fileMode & S_IWUSR) permissions[1] = 'w';
    if(fileMode & S_IXUSR) permissions[2] = 'x';

    /*Group Permissions*/
    if(fileMode & S_IRGRP) permissions[3] = 'r';
    if(fileMode & S_IWGRP) permissions[4] = 'w';
    if(fileMode & S_IXGRP) permissions[5] = 'x';

    /*Other Permissions*/
    if(fileMode & S_IROTH) permissions[6] = 'r';
    if(fileMode & S_IWOTH) permissions[7] = 'w';
    if(fileMode & S_IXOTH) permissions[8] = 'x';
}
