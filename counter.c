#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<limits.h>
#include<getopt.h>

#include<sys/wait.h>
#define __USE_POSIX199309
#include<time.h>

void print_usage();
void alrm_hdlr(int useless);
void err_msg(char * msg);

/* External data */
extern char * optarg;
extern int optind;      /* Index to first non-arg parameter */

/* More globals */
timer_t timer;
struct itimerspec t;

/* Options */
bool OPT_H = false;
bool OPT_S = false;
bool OPT_M = false;
bool OPT_U = false;
bool OPT_F = false;

int count;
int final;

void print_usage()
{
    fprintf(stderr, "Usage: counter (-t <sec> || -m <ms> || -u <us>) -f <final>\n");
    fprintf(stderr, "\t-h Print usage\n");
    fprintf(stderr, "\t-s <sec> Specify time interval in seconds\n");
    fprintf(stderr, "\t-m <ms> Specify time interval in milliseconds\n");
    fprintf(stderr, "\t-u <us> Specify time interval in microseconds\n");
    fprintf(stderr, "\t-f <final> Final count\n");
}

int main(int argc, char * argv[])
{
    /* Timer setup */
    timer_create(CLOCK_MONOTONIC, NULL, &timer);
    t.it_value.tv_sec = 1;
    t.it_value.tv_nsec = 0;
    t.it_interval.tv_sec = 0;
    t.it_interval.tv_nsec = 0;

    /* Argument parsing */
    char opt;
    char * strerr = NULL;
    long arg;
    while((opt = getopt(argc, argv, "+hs:m:u:f:")) != -1)
    {
        switch(opt)
        {
            case 's':
            case 'm':
            case 'u':
                if(OPT_S || OPT_M || OPT_U)
                    err_msg("-s -m -u mutally exclusive\nPlease specify only one\n\n");
                if(opt == 's')
                    OPT_S = true;
                else if(opt == 'm')
                    OPT_M = true;
                else
                    OPT_U = true;
                arg = strtol(optarg, &strerr, 10);
                if(arg > INT_MAX || arg < 0 || strerr[0] != 0)
                {
                    if(opt == 's')
                        err_msg("Unable to parse -s argument correctly, should be number of seconds\n\n");
                    else if(opt == 'm')
                        err_msg("Unable to parse -m argument correctly, should be number of milliseconds\n\n");
                    else
                        err_msg("Unable to parse -u argument correctly, should be number of microseconds\n\n");
                }
                if(opt == 'm')
                {
                    t.it_value.tv_nsec = (arg % 1000) * 1000000;
                    t.it_value.tv_sec = arg / 1000; /* truncates per Section 2.5 of K&R 2nd ed */
                }
                else if(opt == 'u')
                {
                    t.it_value.tv_nsec = (arg % 1000000) * 1000;
                    t.it_value.tv_sec = arg / 1000000;
                }
                else
                    t.it_value.tv_sec = arg;
                optarg = NULL;
                break;
            case 'f':
                if(OPT_F)
                    err_msg("-f specified two or more times\n\n");
                OPT_F = true;
                arg = strtol(optarg, &strerr, 10);
                if(strerr[0] != 0)
                    err_msg("Unable to parse -f argument correctly, should be final count\n\n");
                if(arg < 0)
                    err_msg("Final count in -f argument is negative.\n\n");
                final = arg;
                optarg = NULL;
                break;
            case 'h':
            default:
                print_usage();
                return 0;
        }
    }

    // Call the handler to set up the signal handling and post to the sem for the first time
    alrm_hdlr(0);

    while(1)
        wait(NULL);
}

void alrm_hdlr(int __attribute__((unused)) useless)
{
    signal(SIGALRM, &alrm_hdlr);

    fprintf(stderr, "%d\n", count);
    count++;
    if(count == final)
        exit(0);

    timer_settime(timer, 0, &t, NULL);
    return;
}

void err_msg(char * msg)
{
    fprintf(stderr, msg);
    print_usage();
    exit(-1);
}
