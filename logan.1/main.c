#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "TreeTraversal.h"

#define SIZE 10

void display_help_message(char *);

int main(int argc, char *argv[]) {

    int argCounter = 0;
    char options[SIZE];
    char c;

    if(argc < 2){
        display_help_message(argv[0]);
    }

    /*Use getopt to parse command arguements, hI: means
 *      * h and I require an extra argument*/
    while((c = getopt(argc, argv, "hI:Ldgipstul")) != -1)
        switch (c) {

            /*Help option*/
            case 'h' :
                display_help_message(argv[0]);
                break;

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

                break;

        }

    char* dirName = argv[optind];

    if(dirName == NULL){
        dirName = ".";
    }

    char cleanedArray[argCounter + 1];
    int i = 0;
    for(i; i < argCounter; i++){
        cleanedArray[i] = options[i];
    }

    BFS(dirName, cleanedArray, argCounter);


    return 0;
}

void display_help_message(char *executable){
    fprintf(stderr, "\nHelp Option Selected [-h]: The following is the proper format for executing "
                    "the file:\n\n%s", executable);
    fprintf(stderr, " [-h] [-I n] [-L -d -g -i -p -s -t -u | -l] [dirname]\n\n");
    fprintf(stderr, "[-h]   : Print a help message and exit.\n"
                    "[-I n] : Change indentation to 'n' spaces for each level.\n"
                    "[-L]   : Follow symbolic links, if any.  Default will be to not follow symbolic links\n"
                    "[-d]   : Show the time of last modification.\n"
                    "[-g]   : Print the GID associated with the file.\n"
                    "[-i]   : Print the number of links to file in inode table.\n"
                    "[-p]   : Print permission bits as rwxrwxrwx.\n"
                    "[-s]   : Print the size of file in bytes.\n"
                    "[-t]   : Print information on file type.\n"
                    "[-u]   : Print the UID associated with the file.\n"
                    "[-l]   : This option will be used to print information on the file as if the options tpiugs are all specified.\n\n");

    exit(EXIT_SUCCESS);
}
