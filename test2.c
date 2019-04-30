#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h> 

#define MAX_TIME 10
#define ARIV_TIME .00001 // 10 microsecond

// Struct defintion
struct Queue {
    struct Queue    *next;
    char            ip[18];
    int             packet; // 0-89 where 0-29 is low 30-59 is med and 60-89 is high (WHEN INITIALLY ADDED TO Q)
    clock_t         entry_time,sent_time;
};

// The three queues to be passed between pipes
struct Priority {
    struct Queue *low,*med,*high;
    unsigned int max_q; // Maximum number of items in any of the queues
};

// Function defns 
void add2queue(char *argv[],int *fd1,int *fd2,int*fd3,int *fd4);



/* Remove top of queue */
void removeHead(struct Queue *q) {
    struct Queue *t = q->next;
    free(q);
    *q =*t;
}

/* Init Priority Queue */
void init_priority(struct Priority *p) {
    p->low = NULL;
    p->med = NULL;
    p->high = NULL;
    p->max_q = 20;
}

/* Queue Manager removes nodes that have been sitting for too long */
void qManage(char *argv[],int *fd1,int *fd2,int *fd3,int *fd4) {
    struct Priority *pq;
    pq =  (struct Priority *) calloc(1,sizeof(struct Priority));
    // Initialize Priority Queue and write the pointer to pipe 4
    init_priority(pq);
    int pid2 = fork();
    if (pid2 == 0) {
        add2queue(argv,fd1,fd2,fd3,fd4);
        return;
    } 
    printf("Max packets in each Queue: %d\n",pq->max_q);
    write(fd4[1],pq,sizeof(struct Priority *));
    printf("written\n");
    // Start Management
    while(1) {
        
    }


}

/* Add2queue */
void add2queue(char *argv[],int *fd1,int *fd2,int*fd3,int *fd4) {
    FILE *fpt;
    char IPs[][18] = {"10.1.152.111","10.1.152.123","10.1.152.253","00.0.000.000"};
    int i;
    char init4[100];
    fpt = fopen(argv[1],"rb");
    if (fpt == NULL) {
        printf("Unable to open %s for read.\nExiting now.\n",argv[1]);
        exit(0);
    }
    fseek(fpt, 0L, SEEK_END);
    unsigned long int f_size = (unsigned long int) ftell(fpt);
    rewind(fpt);
    printf("Number of packets to be sent: %li\n",f_size);

    

    // Copy pointer from pipe
    sleep(1);
    struct Priority *pq;
    pq =  (struct Priority *) calloc(1,sizeof(struct Priority));
    read(fd4[0],pq,sizeof(struct Priority ));
    printf("read\n");
    printf("Confirm max queue: %d\n",pq->max_q);

    // Read bytes from a file as input data 
    while(i < f_size) {
        

        i++;
    }


    fclose(fpt);
}

/* Send2recpient */

//wait for alleged packet send time
void send2recp(int *fd1, int *fd2, int *fd3) {
    int i, highq = 0, lowq = 0, medq = 0; 
	struct Priority *pq; 
	pq = (struct Priority *) calloc(1,sizeof(struct Priority));
	read(fd4[0], pq, sizeof(struct Priority));
    /*
		Read from pipe4 (fd4) the Priority structure
		** see qManage/add2q for example
	*/
	read(fd1[0],done,sizeof(int));
	struct Queue tempq;
    while(1) {
		read(fd1[0],done,sizeof(int));
		if (pq.high != NULL){
			tempq = pq->high;
			pq.high = pq->high->next; 
			if (tempq->packet > 59)	highq++;
			else if (60 >= tempq->packet > 29) medq++;
			else if (0 <= tempq->packet < 30) lowq ++; 
			free(tempq);
		}
		else if (pq.med != NULL){
			tempq = pq->med; 
			pq.med = pq->med->next;
			if (tempq->packet > 59)	highq++;
			else if (60 >= tempq->packet > 29) medq++;
			else if (0 <= tempq->packet < 30) lowq ++; 
			free(tempq);
		}
		else if (pq.low != NULL){
			tempq = pq->low;
			pq.low = pq->low->next; 
			if (tempq->packet > 59)	highq++;
			else if (60 >= tempq->packet > 29) medq++;
			else if (0 <= tempq->packet < 30) lowq ++; 
			free(tempq);
		}
		else if (done == 1){
			break;
		}
		i++;

		/*

			using priority structure, check if any of the queues have packets (in order)

			if found, remove the packet from queue by saving it in a local temp and then 				set queue = queue->next where queue is high, med, low in the Priority structure
 
			increment counters for high, med, low depending on packet "sent"

			
		*/


		// Check if more in queue
    }
}

int main(int argc, char *argv[]) {
    
    char mesg[] = "This is the initial message.";
    char init[] = "This is the initial message.";
    int slen = strlen(mesg);
    int fd1[2]; // Top of Queue
    int fd2[2]; // Next index (supplied by send2recp)
    int fd3[2]; // Curr index (supplied by qManager)
    int fd4[2]; // full Priority q
    int hold = 0;
    int j,pid,pid2;
    unsigned long int i = 0;
    printf("\n");
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
    if (pipe(fd3)==-1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 

    pid = fork();
    
    // Add2q
    if (pid == 0) {

        send2recp(fd1,fd2,fd3);
        
    // Send2recip
    } else {
        if (pipe(fd4)==-1) 
        { 
            fprintf(stderr, "Pipe Failed" ); 
            return 1; 
        }
        pid2 = 1;//fork();
        if (pid2 == 0) {
            add2queue(argv,fd1,fd2,fd3,fd4);
        } else {
            qManage(argv,fd1,fd2,fd3,fd4);
        }
        
    }

    return 0;
} 
