
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h> 
#include <sys/mman.h>
#include <time.h>

#define SMART_TIME 1   // Smart packet dropping period
#define QUEUE_TIME 10   // Packet bumping
#define ARIV_TIME .0001 // 10 microsecond
#define MAX_QUEUE 100
// Struct defintion
struct Queue {
    struct Queue    *next;
    char            ip[18];
    unsigned int    packet; // 0-89 where 0-29 is low 30-59 is med and 60-89 is high (WHEN INITIALLY ADDED TO Q)
    clock_t         entry_time;
};

// The three queues to be passed between pipes
struct Priority {
    struct Queue *low,*med,*high;
    unsigned int max_q; // Maximum number of items in any of the queues
    //unsigned int count[3]; // 0 is low, 1 is medium, 2 is high
};

// Struct for last queued packets
struct SmartPacket {
    int         packet;
    clock_t     entry_time;
};

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, 0, 0);
}

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
    p->max_q = MAX_QUEUE;
    //p->count[0] = 0;
    //p->count[1] = 0;
    //p->count[2] = 0;

}

/*  */
struct Queue * createPacket(int val) {
    void *shmem = create_shared_memory(sizeof(struct Queue));
    struct Queue *q = (struct Queue *) shmem;
    q->packet = val;
    q->next = NULL;
    q->entry_time = clock();
    return q;
}

/* Checks to see if a packet can be ignored (redundancy check) */
int checkSmart(struct SmartPacket *sp,int value) {
    int drop = 0;
    struct SmartPacket *t = sp;
    double cur;
    while(t != NULL) {
        // If the packets are saying the same thing
        if (t->packet == value) {
            cur = (double)(clock()-sp->entry_time)/CLOCKS_PER_SEC;
            if (cur < SMART_TIME) 
                drop = 1;
        }
    }
    return drop;
}

int numInQueue(struct Queue *q) {
    struct Queue *t = q;
    int i=0;
    while(t != NULL) {
        i++;
        t = t->next;
    }
    return i;   
}

/* Queue Manager increases priority of nodes that have sat for too long */
void qManage(char *argv[],int *fd1,int *fd2,int *fd3,int *fd4) {
    struct Priority *pq;
    int done = 0;
    double dur,future;
    clock_t start = clock();
    future = (double)(clock()-start)/CLOCKS_PER_SEC+.2;
    // Initialize Priority Queue and write the pointer to pipe 4
    void *vp = create_shared_memory(sizeof(struct Priority));
    pq = (struct Priority *) vp;
    init_priority(pq);
    
    write(fd4[1],pq,sizeof(struct Priority ));
    printf("Max packets in each Queue: %d\n",pq->max_q);
    
    read(fd3[0],&done,sizeof(int));
    // Start Management
    while(done != 1) {
        
        if((double)((clock()-start)/CLOCKS_PER_SEC) > future) {
            printf("low: %d, med %d, high %d\n",numInQueue(pq->low),numInQueue(pq->med),numInQueue(pq->high));
            future = (double)(clock()-start)/CLOCKS_PER_SEC +.5;
        }
        read(fd3[0],&done,sizeof(int));
    }
    printf("exit manage\n");

}

/* Add2queue */
void add2queue(char *argv[],int *fd1,int *fd2,int*fd3,int *fd4) {
    FILE *fpt;
    char IPs[][18] = {"10.1.152.111","10.1.152.123","10.1.152.253","00.0.000.000"};
    int i,j,bVal;
    unsigned long int smartDrop = 0;
    unsigned long int lost[] = {0,0,0};
    char init4[100],byte;
    struct Queue *qt; // temp packet
    i = 0;
    write(fd1[1],&i,sizeof(int));

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
    printf("Confirm max queue: %d\n",pq->max_q);
    struct SmartPacket *smartQ;
    
    // Read bytes from a file as input data 
    while(i < f_size) {
        fread(&byte,1,1,fpt);
        if(checkSmart(smartQ,(int)byte) == 0) {
            //bVal = atoi(byte);
            bVal = (unsigned int) byte;
            if (bVal < 30) {
                qt = pq->low;
                for(j=0;j<pq->max_q;j++) {
                    if (qt == NULL) {
                        pq->low = createPacket(bVal);
                        break;
                    }else if (qt->next == NULL) {
                        qt->next = createPacket(bVal);
                    }
                }
            } else if (bVal < 60) {
                qt = pq->med;
                for(j=0;j<pq->max_q;j++) {
                    if (qt == NULL) {
                        pq->med = createPacket(bVal);
                        break;
                    } else if (qt->next == NULL) {
                        qt->next = createPacket(bVal);
                    }
                }
            } else {
                qt = pq->high;
                for(j=0;j<pq->max_q;j++) {
                    if (qt == NULL) {
                        pq->high = createPacket(bVal);
                        break;
                    } else if (qt->next == NULL) {
                        qt->next = createPacket(bVal);
                    }
                }
            } 
            // Add to smart q
            smartQ = (struct SmartPacket *) calloc(1,sizeof(struct SmartPacket));
            smartQ->packet = (int) byte;
            smartQ->entry_time = clock();
        } else {
            smartDrop++;
        }

        i++;
        sleep(ARIV_TIME);
    }
    i = 1;
    

    fclose(fpt);
    write(fd1[1],&i,sizeof(int));
    printf("\nSmart Dropped packet Count: %ld\n",smartDrop);
    
}

/* Send2recpient */

//wait for alleged packet send time
void send2recp(int *fd1, int *fd2, int *fd3,int *fd4) {
    int done,i, highq = 0, lowq = 0, medq = 0; 
	struct Priority *pq; 
	pq = (struct Priority *) calloc(1,sizeof(struct Priority));
	read(fd4[0], pq, sizeof(struct Priority));
    /*
		Read from pipe4 (fd4) the Priority structure
		** see qManage/add2q for example
	*/
	read(fd1[0],&done,sizeof(int));
	struct Queue *tempq;
	clock_t q_time;
	double low_time, high_time, med_time;
    while(1) {
		read(fd1[0],&done,sizeof(int));
		if (pq->high != NULL){
			tempq = pq->high;
			pq->high = pq->high->next; 
			if (tempq->packet > 59)	{
				q_time = clock() - tempq->entry_time;
				high_time += (double)q_time;
				highq++;
			}
			else if (60 >= tempq->packet > 29){
				q_time = clock() - tempq->entry_time;
				med_time += (double)q_time;
				medq++;
			}
			else {
				q_time = clock() - tempq->entry_time;
				low_time += (double)q_time;
				lowq++;
			}
			free(tempq);
		}
		else if (pq->med != NULL){
			tempq = pq->med; 
			pq->med = pq->med->next;
			if (tempq->packet > 59){
				q_time = clock() - tempq->entry_time;
				high_time += q_time;
				highq++;
			}
			else if (60 >= tempq->packet > 29){
				q_time = clock() - tempq->entry_time;
				med_time += (double)q_time;
				medq++;
			}
			else if (0 <= tempq->packet < 30){
				q_time = clock() - tempq->entry_time;
				low_time += (double)q_time;
				lowq++;
			}
			free(tempq);
		}
		else if (pq->low != NULL){
			tempq = pq->low;
			pq->low = pq->low->next; 
			if (tempq->packet > 59)	{
				q_time = clock() - tempq->entry_time;
				high_time += q_time;
				highq++;
			}
			else if (60 >= tempq->packet > 29){
				q_time = clock() - tempq->entry_time;
				med_time += (double)q_time;
				medq++;
			}
			else if (0 <= tempq->packet < 30){
				q_time = clock() - tempq->entry_time;
				low_time += (double)q_time;
				lowq++;
			}
			free(tempq);
		}
		else if (done == 1){
			break;
		}
		i++;
		sleep(.005);

		/*
			using priority structure, check if any of the queues have packets (in order)

			if found, remove the packet from queue by saving it in a local temp and then 				set queue = queue->next where queue is high, med, low in the Priority structure
 
			increment counters for high, med, low depending on packet "sent"
		*/

		// Check if more in queue
    }
	double avg_ht = high_time/highq; 
	double avg_mt = med_time/medq;
	double avg_lt = low_time/lowq;
	printf("\nAverage time for high priority packets: %f\nAverage time for medium priority packets: %f\nAverage time for low priority packets: %f\n", avg_ht, avg_mt, avg_lt);
	printf("Number of high priority packets processed: %d\nNumber of medium priority packets: %d\nNumber of low priority packets processed: %d\n", highq, medq, lowq);
}

int main(int argc, char *argv[]) {
    
    char mesg[] = "This is the initial message.";
    char init[] = "This is the initial message.";
    int slen = strlen(mesg);
    int fd1[2]; // Done queuing
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


    // Create Shared memory
    //void* shmem = create_shared_memory(sizeof(struct Queue)*3*MAX_QUEUE);

    pid = fork();
    
    // Add2q
    if (pid == 0) {

        send2recp(fd1,fd2,fd3,fd4);
        
    // Send2recip
    } else {
        if (pipe(fd4)==-1) 
        { 
            fprintf(stderr, "Pipe Failed" ); 
            return 1; 
        }
        pid2 = fork();
        if (pid2 == 0) {
            add2queue(argv,fd1,fd2,fd3,fd4);
        } else {
            qManage(argv,fd1,fd2,fd3,fd4);
        }
        
    }

    return 0;
} 
