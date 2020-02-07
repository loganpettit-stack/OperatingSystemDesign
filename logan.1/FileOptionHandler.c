#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

void getPermissions(int, char[]);
char getFileType(int);
int getByteString(long);

char* getOptions(char* path, char optionsArray[], int argCounter) {

    struct passwd *getpwuid(), *pw_ptr;
    struct group *getgrgid(), *grp_ptr;
    char* byteString = "";
    char permissions[10];
    char fileType;
    int i = 0;
    struct stat sb;

    lstat(path, &sb);


    for (i = 0; i < argCounter; i++) {
        char opt = optionsArray[i];

        switch (opt) {
            case 'p' :
                getPermissions(sb.st_mode, permissions);
                printf("%-11s", permissions);
                break;

            case 't' :
                fileType = getFileType(sb.st_mode);
                printf("%-2c", fileType);
                break;

            case 'i' :
                printf("%-2d", sb.st_nlink);
                break;

            case 'u' :
                pw_ptr = getpwuid(sb.st_uid);
                if(pw_ptr == NULL)
                    printf("%-5d", sb.st_uid);
                else
                    printf("%-10s", pw_ptr->pw_name);

                break;

            case 'g':
                grp_ptr = getgrgid(sb.st_gid);
                if(grp_ptr == NULL)
                    printf("%-5d", sb.st_gid);
                else
                    printf("%-10s", grp_ptr->gr_name);

                break;

            case 's' :
                /*byteString = getByteString((long) sb.st_size);*/

                break;
        }
    }
}

void getPermissions(int fileMode, char permissions[]){

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

char getFileType(int mode){
    char fileType;

    switch (mode & S_IFMT) {
        case S_IFBLK:
            fileType = 'b';
            break;
        case S_IFCHR:
            fileType = 'c';
            break;
        case S_IFDIR:
            fileType = 'd';
            break;
        case S_IFLNK:
            fileType = 'l';
            break;
        case S_IFREG:
            fileType = '-';
            break;
        case S_IFSOCK:
            fileType = 's';
            break;
        case S_IFIFO:
            fileType = 'p';
            break;
        default:
            fileType = '?';
            break;
    }

    return fileType;
}


