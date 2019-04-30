#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <math.h>
//#include <curses.h>

// Struct defintion
struct Queue {
    struct Queue    *next;
    char            ip[18];
    int             packet;
    clock_t         entry_time,sent_time;
} queue;

int main(int argc, char *argv[]) {
    struct Queue low,med,high;
    FILE *fpt;
    clock_t start = clock();
    double cur = (double)(clock()-start)/CLOCKS_PER_SEC;
    int pid = fork();

    if(pid == 0) {
        while(cur < 1.0) {
            cur = (double)(clock()-start)/CLOCKS_PER_SEC;
            printf("%f\n",cur);
        }

    }
    else
    {
        wait(NULL);
    }
    
    //printf("%ld %d\n",clock()-start,pid);

    return 0;
}