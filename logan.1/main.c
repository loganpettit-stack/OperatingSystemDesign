#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "TreeTraversal.h"

#define SIZE 10

void displayUsage();

int main(int argc, char *argv[]) {

    int argCounter = 0;
    char options[SIZE];
    char c;
    char* executable = strdup(argv[0]);
    char* error = strcat(executable, ": Error: ");
    char* optionNumError;
    char* optionError;
    char* duplicateError;
    char* optionlError;

    if(argc > 1) {

        /*Use getopt to parse command arguements, h: means
 *          * h require an extra argument*/
        while ((c = getopt(argc, argv, "h:Ldgipstul")) != -1)
            switch (c) {

                /*Help option*/
                case 'h' :
                    printf("\nHelp Option Selected [-h], this program will preform a breadth first search of specified\n"
                           " directory if no directory is given on the command line the the current directory is used.\n ");
                    displayUsage();
                    exit(EXIT_SUCCESS);

                    /*Follow symbolic links if any*/
                case 'L' :
                    options[argCounter] = 'L';
                    argCounter += 1;
                    break;

                    /*Print information on file type*/
                case 't' :
                    options[argCounter] = 't';
                    argCounter += 1;
                    break;

                    /*Print permission bits*/
                case 'p' :
                    options[argCounter] = 'p';
                    argCounter += 1;
                    break;

                    /*print number of links to files in i node table*/
                case 'i' :
                    options[argCounter] = 'i';
                    argCounter += 1;
                    break;

                    /*print uid associated with file*/
                case 'u' :
                    options[argCounter] = 'u';
                    argCounter += 1;
                    break;

                    /*print gid associated with file*/
                case 'g' :
                    options[argCounter] = 'g';
                    argCounter += 1;
                    break;

                    /*print size of file*/
                case 's' :
                    options[argCounter] = 's';
                    argCounter += 1;
                    break;

                    /*show time of last modification*/
                case 'd' :
                    options[argCounter] = 'd';
                    argCounter += 1;
                    break;

                    /*print information on file as if tpiugs is entered*/
                case 'l' :
                    options[argCounter] = 'l';
                    argCounter += 1;
                    break;

                default :
                    optionError = strcat(error, "Incorrect option choice available format and options: \n");
                    fprintf(stderr, "%s\n", optionError);
                    displayUsage();
                    exit(EXIT_FAILURE);
            }
    }

    if(argCounter > 8){
        optionNumError = strcat(error, "Maximum number of options exceeded available options and formqts are: \n");
        fprintf(stderr, "%s\n", optionNumError);
        displayUsage();
        exit(EXIT_FAILURE);
    }

    /*check for duplicate options and options
 * combined with l */
    char argChecker;
    int k;
    for(k = 0; k < argCounter; k++){
        argChecker = options[k];

        if(argChecker == 'l' && argCounter > 1){
            optionlError = strcat(error, "Option l cannot be combined with any other options, format is: \n");
            fprintf(stderr, "%s\n", optionlError);
            displayUsage();
            exit(EXIT_FAILURE);
        }

        int m;
        for(m = k + 1; m < argCounter; m++){
            if(options[m] == argChecker){
                duplicateError = strcat(error, "Duplicate options entered options are:\n");
                fprintf(stderr, "%s\n", duplicateError);
                displayUsage();
                exit(EXIT_FAILURE);
            }
        }
    }
    char* dirName = argv[optind];

    if(dirName == NULL){
        dirName = ".";
    }

    BFS(dirName, options, argCounter, error);

    return 0;
}

void displayUsage(){
    fprintf(stderr, "bt [-h] [-L -d -g -i -p -s -t -u | -l] [dirname]\n\n");
    fprintf(stderr, "[-h]   : Print a help message and exit.\n"
                    "[-L]   : Follow symbolic links, if any.  Default will be to not follow symbolic links\n"
                    "[-d]   : Show the time of last modification.\n"
                    "[-g]   : Print the GID associated with the file.\n"
                    "[-i]   : Print the number of links to file in inode table.\n"
                    "[-p]   : Print permission bits as rwxrwxrwx.\n"
                    "[-s]   : Print the size of file in bytes.\n"
                    "[-t]   : Print information on file type.\n"
                    "[-u]   : Print the UID associated with the file.\n"
                    "[-l]   : This option will be used to print information on the file as if the options tpiugs are all specified.\n\n");
}
