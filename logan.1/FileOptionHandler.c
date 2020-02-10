
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void getPermissions(int, char[]);
char getFileType(int);
void showFileSize(long);

char* getOptions(char* path, char optionsArray[], int argCounter) {


    char newTime[50];
    char*  time;
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
                showFileSize((long) sb.st_size);
                break;

            case 'd' :
                time = ctime(&sb.st_mtime);
                int j = 0;
                while(time[j] != '\0'){
                    if(time[j] == '\n'){
                        time[j] = ' ';
                    }
                    j += 1;
                }

                printf(" %s ", time);
                break;

            case 'l' :
                fileType = getFileType(sb.st_mode);
                printf("%-2c", fileType);

                getPermissions(sb.st_mode, permissions);
                printf("%-11s", permissions);

                printf("%-2d", sb.st_nlink);

                pw_ptr = getpwuid(sb.st_uid);
                if(pw_ptr == NULL)
                    printf("%-5d", sb.st_uid);
                else
                    printf("%-10s", pw_ptr->pw_name);

                grp_ptr = getgrgid(sb.st_gid);
                if(grp_ptr == NULL)
                    printf("%-5d", sb.st_gid);
                else
                    printf("%-10s", grp_ptr->gr_name);

                showFileSize((long) sb.st_size);

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

void showFileSize(long size){

    if(size < 1024){
        printf("%-3ld %-3s ", size, "B");
    }
    else if (size > 1024 && size < 1048576){
        size /= 1024;
        printf("%-3ld %-3s ", size, "K");
    }
    else if (size > 1048576 && size < 1073741824){
        size /= 1024;
        size /= 1024;
        printf("%-3ld %-3s ", size, "M");
    }
    else if (size > 1073741824){
        size /= 1024;
        size /= 1024;
        size /= 1024;
        printf("%-3ld %-3s ", size, "G");
    }
}
