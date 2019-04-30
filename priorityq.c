#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Struct defintion
struct Queue {
    struct Queue    *next;
    char            ip[18];
    int             packet;
    clock_t         entry_time,sent_time;
};

int main(int argc, char *argv[]) {
    struct Queue low,med,high;
    FILE *fpt;
    low = NULL;
    med = NULL;
    high = NULL;
    
    

    return 0;
}