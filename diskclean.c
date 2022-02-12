#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

int parse_time(char *optarg)
{
    return 0;
}

void usage(int status)
{
    if (status != EXIT_SUCCESS) {
        fputs("Try 'diskclean --help' for more information.\n", stderr);
        exit(status);
    }
    fputs("\
Usage: diskclean [OPTION]... [DIRECTORY]...\n\
Remove old files within specified DIRECTORYs (the current directory by default).\n\
Recursively prompt to remove or not.\n\
\n\
  -y, --yes               assume \"yes\" as answer to all prompts and run non-interactively\n\
  -p, --print             print the file list and exit\n\
  -d, --keep-directory    do not delete the directory\n\
  -t, --time              remove file older than DD:hh:mm:ss, default to 7 days\n\
  -q, --quiet             run silently, can be only used with -y option\n\
  -h, --help              display this help and exit\n\n\
", stdout);
    exit(status);
}

int main(int argc, char *argv[])
{
    bool yes = false;
    bool print = false;
    bool keep_directory = false;
    int time = 7 * 24 * 60 * 60;  // default 1 week
    bool quiet = false;
    bool help = false;
    
    static struct option long_options[] = {
        {"yes", no_argument, NULL, 'y'},
        {"print", no_argument, NULL, 'p'},
        {"keep-directory", no_argument, NULL, 'd'},
        {"time", required_argument, NULL, 't'},
        {"quiet", no_argument, NULL, 'q'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };
    
    /* Parse command line options. */

    int c;
    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "ypdt:qh",
                        long_options, &option_index);
        if (c == -1)
            break;
        
        switch (c) {
            case 'y':
                yes = true;
                break;
            case 'p':
                print = true;
                break;
            case 'd':
                keep_directory = true;
                break;
            case 't':
                time = parse_time(optarg);
                break;
            case 'q':
                quiet = true;
                break;
            case 'h':
                usage(EXIT_SUCCESS);
            default:
                usage(EXIT_FAILURE);
        }
    }
    if (optind < argc) {

    }
    exit(EXIT_SUCCESS);
}