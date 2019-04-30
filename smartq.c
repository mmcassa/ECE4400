#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Struct defintion
struct Queue {
    struct Queue    *next;
    char            ip[18];
    int             packet; // 0-89 where 0-29 is low 30-59 is med and 60-89 is high (WHEN INITIALLY ADDED TO Q)
    clock_t         entry_time,sent_time;
};

int main(int argc, char *argv[]) {
    struct Queue low,med,high;
    
    low = NULL;
    med = NULL;
    high = NULL;
    int fd1[2];
    int fd2[2];

    unsigned long int i = 0;

    // Set up pipes 4 queue
    if (pipe(fd1)==-1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 
    if (pipe(fd2)==-1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 

    pid = fork();

    // Add2q
    if (pid == 0) {
        FILE *fpt;
        if (argc != 2) {
            printf("Usage: lab3 [Read File]\n");
            exit(0);
        }
        fpt = fopen(argv[1],"rb");
        if (fpt == NULL) {
            printf("Unable to open %s for read.\nExiting now.\n",argv[1]);
            exit(0);
        }
        fseek(fp, 0L, SEEK_END);
        unsigned long int f_size = (unsigned long int) ftell(fp);
        rewind(fp);
        // Read bytes from a file as input data 
        while(i < f_size) {
            
            i++;
        }
        fclose(fpt);
    // Send2recip
    } else {
        while(i < 1000000000) {
            
            i++;
        }
    }

    return 0;
} 