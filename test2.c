
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
#define ARIV_TIME .05 // 10 microsecond
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
    struct SmartPacket *next;
    int         packet;
    clock_t     entry_time;
};

void* create_shared_memory(size_t size) {
  // Our memory buffer will be //readable and writable:
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
void add2queue(char *argv[],int *share1, int *share2,int *share3, struct Priority *pq);


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
    struct Queue *q = (struct Queue *) create_shared_memory(sizeof(struct Queue));
    q->packet = val;
    q->next = NULL;
    q->entry_time = clock();
    return q;
}

/* Checks to see if a packet can be ignored (redundancy check) */
int checkSmart(struct SmartPacket *sp,int value) {
    int drop = 0;
    struct SmartPacket *t;
    t = sp;
    double cur;
    while(t != NULL) {
        // If the packets are saying the same thing
        if (t->packet == value) {
            cur = (double)(clock()-t->entry_time)/CLOCKS_PER_SEC;
            if (cur < SMART_TIME) 
                drop = 1;
                break;
        }
        t = t->next;
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
void qManage(char *argv[],int *share1, int *share2,int *share3, struct Priority *pq) {//int *fd2,int *fd3,int *fd4) {
    //struct Priority *pq;
    int done = 0;
    double dur,future;
    clock_t start = clock();
    future = (double)(clock()-start)/CLOCKS_PER_SEC+1;
    // Initialize Priority Queue and //write the pointer to pipe 4
    
    
    ////write(fd4[1],pq,sizeof(struct Priority ));
    printf("Max packets in each Queue: %d\n",pq->max_q);
    ////write(fd4[1],pq,sizeof(struct Priority ));
    ////read(fd3[0],&done,sizeof(int));
    // Start Management

    while(*share3 != 1) {
        //printf("%f",((double)(clock()-start))/CLOCKS_PER_SEC);
        if(((double)(clock()-start))/CLOCKS_PER_SEC > future) {
            //printf("low: %d, med %d, high %d\n",numInQueue(pq->low),numInQueue(pq->med),numInQueue(pq->high));
            future = (double)(clock()-start)/CLOCKS_PER_SEC +1;
        }
        ////read(fd3[0],&done,sizeof(int));
    }
    printf("exit manage\n");

}

/* Add2queue */
void add2queue(char *argv[],int *share1, int *share2,int *share3, struct Priority *pq) {
    FILE *fpt;
    char IPs[][18] = {"10.1.152.111","10.1.152.123","10.1.152.253","00.0.000.000"};
    int j,bVal;
    unsigned long int smartDrop = 0,i;
    unsigned long int lost[] = {0,0,0};
    char init4[100],byte;

    double future;
    clock_t start = clock();
    future = (double)(clock()-start)/CLOCKS_PER_SEC+.05;

    struct Queue *qt; // temp packet
    i = 0;
    //write(share1[1],&i,sizeof(int));

    fpt = fopen(argv[1],"rb");
    if (fpt == NULL) {
        printf("Unable to open %s for //read.\nExiting now.\n",argv[1]);
        exit(0);
    }
    fseek(fpt, 0L, SEEK_END);
    unsigned long int f_size = (unsigned long int) ftell(fpt);
    rewind(fpt);
    printf("Number of packets to be sent: %li\n",f_size);

    

    // Copy pointer from pipe

    printf("Confirm max queue: %d\n",pq->max_q);
    
    struct SmartPacket *smartQ;
    smartQ = NULL;
    i = 0;
    printf("%li %li\n",i,f_size);
    // //read bytes from a file as input data 
    while( i < f_size) {

        //printf("%li\n",i);
        fread(&byte,1,sizeof(char),fpt);
        if(checkSmart(smartQ,0) == 0) {
            bVal = (int) byte;
            if (bVal < 30) {
                qt = pq->low;
                for(j=0;j<pq->max_q;j++) {
                    if (qt == NULL) {
                        pq->low = createPacket(bVal);
                        j =0;
                        break;
                    }else if (qt->next == NULL) {
                        qt->next = createPacket(bVal);
                        j =0;
                        break;
                    }
                    qt = qt->next;
                }
                if (j == pq->max_q) {
                    lost[0]++;
                }
            } else if (bVal < 60) {
                qt = pq->med;
                for(j=0;j<pq->max_q;j++) {
                    if (qt == NULL) {
                        pq->med = createPacket(bVal);
                        j =0;
                        break;
                    } else if (qt->next == NULL) {
                        qt->next = createPacket(bVal);
                        j =0;
                        break;                        
                    }
                    qt = qt->next;
                }
                if (j == pq->max_q) {
                    lost[1]++;
                }
            } else {
                qt = pq->high;
                for(j=0;j<pq->max_q;j++) {
                    if (qt == NULL) {
                        pq->high = createPacket(bVal);
                        break;
                    } else if (qt->next == NULL) {
                        qt->next = createPacket(bVal);
                        break;
                    }
                    qt = qt->next;
                }
                if (j == pq->max_q) {
                    lost[2]++;
                }
            } 
            // Add to smart q
            //smartQ = (struct SmartPacket *) calloc(1,sizeof(struct SmartPacket));
            //smartQ->packet = i;
            //smartQ->next = NULL;
            //smartQ->entry_time = clock();
        } else {
            smartDrop++;
        }

        i++;
        if (i % 80 == 0 || i == 6) {
            printf("low: %d, med %d, high %d\n",numInQueue(pq->low),numInQueue(pq->med),numInQueue(pq->high));
        }
        if(((double)(clock()-start))/CLOCKS_PER_SEC > future) {
            printf("low: %d, med %d, high %d\n",numInQueue(pq->low),numInQueue(pq->med),numInQueue(pq->high));
            future = (double)(clock()-start)/CLOCKS_PER_SEC +.05;
        }
        sleep(ARIV_TIME);
    }
    i = 1;
    

    fclose(fpt);
    ////write(share1[1],&i,sizeof(int));
    *share1 = 1;
    printf("\nSmart Dropped packet Count: %ld\n",smartDrop);
    printf("Lost packets (Low, Med, High): %ld %ld %ld\n",lost[0],lost[1],lost[2]);
    
}

/* Send2recpient */

//wait for alleged packet send time
void send2recp(int *share1, int *share2,int *share3, struct Priority *pq) {//int *fd2, int *fd3,int *fd4) {
    int *done,i, highq = 0, lowq = 0, medq = 0; 
	//struct Priority *pq; 
	//pq = (struct Priority *) calloc(1,sizeof(struct Priority));
	sleep(.5);
    //read(fd4[0], pq, sizeof(struct Priority));
    /*
		//read from pipe4 (fd4) the Priority structure
		** see qManage/add2q for example
	*/
    pq->max_q = 50;
    printf("Confirm max queue send: %d\n",pq->max_q);
    done = share1;
    printf("%d",*done);
	struct Queue *tempq;
	clock_t q_time;
	double low_time, high_time, med_time;
    while(1) {
		////read(share1[0],&done,sizeof(int));
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
		else if (*done == 1){
			break;
		}
		i++;
		sleep(.005);
    }
    *share3 = 1;
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
    //int share1[2]; // Done queuing
    int hold = 0;
    int j,pid,pid2;
    unsigned long int i = 0;
    printf("\n");
    // Set up pipes 4 queue


    // Create Shared memory
    //void* shmem = create_shared_memory(sizeof(struct Queue)*3*MAX_QUEUE);
    int *share1;
    int *share2;
    int *share3;
    share1 = (int *) create_shared_memory(sizeof(int));
    share2 = (int *) create_shared_memory(sizeof(int));
    share3 = (int *) create_shared_memory(sizeof(int));

    *share1 = 0;
    *share2 = 0;
    *share3 = 0;

    struct Priority *pq;
    void *vp = create_shared_memory(sizeof(struct Priority));
    pq = (struct Priority *) vp;
    init_priority(pq);

    pid = fork();
    
    // Add2q
    if (pid == 0) {

        send2recp(share1,share2,share3,pq);//fd2,fd3,fd4);
        
    // Send2recip
    } else {
        
        pid2 = fork();
        if (pid2 == 0) {
            add2queue(argv,share1,share2,share3,pq);//fd2,fd3,fd4);
        } else {
            qManage(argv,share1,share2,share3,pq);//,fd2,fd3,fd4);
        }
        
    }

    return 0;
} 
