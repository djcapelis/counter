#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<limits.h>
#include<getopt.h>
#include<semaphore.h>
#include<errno.h>

#include<sys/wait.h>
#define __USE_POSIX199309
#include<time.h>

#include<string.h>

/* Function definitions */
void print_usage();
void alrm_hdlr(int useless);
void err_msg(char * msg);

/* External data */
extern char * optarg;
extern int optind;      /* Index to first non-arg parameter */

/* Globals */
timer_t timer;
struct itimerspec t;
int count;
int final;
unsigned long long memsize;

/* Options */
bool OPT_H = false;
bool OPT_S = false;
bool OPT_M = false;
bool OPT_U = false;
bool OPT_F = false;
bool MEMOPTS = false;

/* Error handler define */
#define chk_err(cond) do { if(cond) { goto err; } } while(0)

void print_usage()
{
    fprintf(stderr, "Usage: counter <options>\n");
    fprintf(stderr, "\t-h Print usage\n");
    fprintf(stderr, "\t-s <sec> Specify time interval in seconds\n");
    fprintf(stderr, "\t-m <ms> Specify time interval in milliseconds\n");
    fprintf(stderr, "\t-u <us> Specify time interval in microseconds\n");
    fprintf(stderr, "\t-G <GBs> Number of gigabytes of memory to allocate\n");
    fprintf(stderr, "\t-M <MBs> Number of megabytes of memory to allocate\n");
    fprintf(stderr, "\t-K <KBs> Nunber of kilobytes of memory to allocate\n");
    fprintf(stderr, "\t-f <final> Final count\n");
}

int main(int argc, char * argv[])
{
    /* Timer setup */
    chk_err(timer_create(CLOCK_MONOTONIC, NULL, &timer) == -1);
    t.it_value.tv_sec = 1;
    t.it_value.tv_nsec = 0;
    t.it_interval.tv_sec = 0;
    t.it_interval.tv_nsec = 0;

    /* Argument parsing */
    char opt;
    char * strerr = NULL;
    long arg;
    while((opt = getopt(argc, argv, "+hs:m:u:f:G:M:K:")) != -1)
    {
        switch(opt)
        {
            case 'G':
            case 'M':
            case 'K':
                if(MEMOPTS)
                    err_msg("Only one -G, -M or -K switch is allowed\n\n");
                MEMOPTS = true;
                arg = strtoll(optarg, &strerr, 10);
                if(arg < 0 || strerr[0] != 0)
                    err_msg("Please enter a valid positive integer for the amount of memory to alloc\n\n");
                memsize = arg;
                if(opt == 'M')
                    memsize *= 1024;
                if(opt == 'G')
                    memsize *= 1024 * 1024;
                optarg = NULL;
                break;
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

    char * mem;
    if(MEMOPTS)
    {
        mem = calloc(memsize, 1024);
        if(mem == NULL)
        {
            fprintf(stderr, "Unable to allocate %lld bytes of memory.  Perhaps try a smaller size?\n", memsize * 1024);
            exit(EXIT_FAILURE);
        }
        memset(mem, 'U', memsize*1024);
    }

    // Call the handler to set up the signal handling and start counting
    alrm_hdlr(0);

    sem_t sem;
    chk_err(sem_init(&sem, 0, 0) == -1);
    while(1)
       sem_wait(&sem); // Wait forever.

err:
    if(errno != 0)
        perror("counter");
    exit(EXIT_FAILURE);
}

void alrm_hdlr(int __attribute__((unused)) useless)
{
    chk_err(signal(SIGALRM, &alrm_hdlr) == SIG_ERR);

    fprintf(stderr, "%d\n", count);
    count++;
    if(count == final)
        exit(EXIT_SUCCESS);

    chk_err(timer_settime(timer, 0, &t, NULL) == -1);
    return;

err:
    if(errno != 0)
        perror("Error in counter signal handler");
    exit(EXIT_FAILURE);
}

void err_msg(char * msg)
{
    fprintf(stderr, msg);
    print_usage();
    exit(EXIT_FAILURE);
}
