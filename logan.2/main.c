#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

void displayUsage();

int main(int argc, char* argv[]) {
    int c;
    char* executable = strdup(argv[0]);
    char* error = strcat(executable, ": Error: ");
    char* Nerror;
    char* Serror;
    char* Berror;
    char* Ierror;


    if(argc > 1) {

        /*Use getopt to parse command arguements, : means option
 *          * requires second argument :: means second arguement is
 *                   * optional*/
        while ((c = getopt(argc, argv, "hn:s:b:i:o:")) != -1)
            switch (c) {

                /*Help option*/
                case 'h' :
                    printf("\nHelp Option Selected [-h]\n\n ");
                    displayUsage();
                    exit(EXIT_SUCCESS);


                /*indicate the max total of child process oss will
 *                  * create*/
                case 'n':
                    if(!isdigit(*optarg)){
                        Nerror = strcat(error, " [-n] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Nerror);
                        displayUsage();
                        exit(EXIT_FAILURE);

                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*indicate the number of children allowed to exist in the
 *                  * system at the same time*/
                case 's' :
                    if(!isdigit(*optarg)){
                        Serror = strcat(error, " [-s] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Serror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*start of the sequence of numbers to bes tested for primality*/
                case 'b':
                    if(!isdigit(*optarg)){
                        Berror = strcat(error, " [-b] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Berror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*increment between numbers that we test*/
                case 'i':
                    if(!isdigit(*optarg)){
                        Ierror = strcat(error, " [-i] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Ierror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*-o filename output file */
                case 'o':
                    printf("o selected\n");

                    break;
            }
    }

}

void displayUsage(){
    fprintf(stderr, "This program will be executed ..\n");
    fprintf(stderr, "-n x Indicate the maximum total of child processes oss will ever create. (Default 4)\n"
                    "-s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n"
                    "-b B Start of the sequence of numbers to be tested for primality\n"
                    "-i I Increment between numbers that we test\n"
                    "-o filename Output file.\n");

}
