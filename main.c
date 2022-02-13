#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <fts.h>
#include <time.h>
#include "diskclean.h"

static void dc_option_init (struct dc_options *x)
{
    x->yes = false;
    x->print = false;
    x->keep_directory = false;
    x->time = 7 * 24 * 60 * 60; // default 1 week
    x->quiet = false;
}

time_t parse_time(char *optarg)
{
    int d, h, m, s;
    if (sscanf(optarg, "%02d:%02d:%02d:%02d", &d, &h, &m, &s) != 4) {
        fputs("Time format must be given by DD:hh:mm:ss\n", stderr);
        exit(EXIT_FAILURE);
    }
    return d*24*60*60 + h*60*60 + m*60 + s;
}

void usage(int status)
{
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, "Try 'diskclean --help' for more information.\n");
        exit(status);
    }
    fputs("\
Usage: diskclean [OPTION]... [DIRECTORY]...\n\
Remove old files within specified DIRECTORYs.\n\
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
    struct dc_options opt;
    time_t t;
    time(&t);
    dc_option_init(&opt);

    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "ypdt:qh",
                        long_options, &option_index);
        if (c == -1)
            break;
        
        switch (c) {
            case 'y':
                opt.yes = true;
                break;
            case 'p':
                opt.print = true;
                break;
            case 'd':
                opt.keep_directory = true;
                break;
            case 't':
                opt.time = t - parse_time(optarg);
                break;
            case 'q':
                opt.quiet = true;
                break;
            case 'h':
                usage(EXIT_SUCCESS);
            default:
                usage(EXIT_FAILURE);
        }
    }
    if (opt.quiet && !opt.yes) {
        fprintf(stderr, "%s: -q option can be only used with -y option\n", argv[0]);
        usage(EXIT_FAILURE);
    }
    if (argc <= optind) {
        fprintf(stderr, "%s: Missing operand\n", argv[0]);
        usage(EXIT_FAILURE);
    }
    else {
        dc(argv + optind, &opt);
    }
    exit(EXIT_SUCCESS);
}