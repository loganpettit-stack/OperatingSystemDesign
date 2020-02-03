#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "TreeTraversal.h"

void display_help_message(char *);

int main(int argc, char *argv[]) {

    char* indent = "4";
    char c;

    char dirname[20];
    printf("Provide directory name");
    scanf("%s", dirname);
    BFS(dirname);


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

            case 'I' :
                /*Check if following option is a digit
 *                  * for changing the indention of files*/
                if(isdigit(*optarg)){
                    indent = optarg;

                    BFS(argv[3]);
                }
                else {
                    perror("bt: Error: option [-I n] must have a corresponding integer value following it represented as n");
                }


            }

    printf("Hello, World!\n");
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
