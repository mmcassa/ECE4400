
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h> 
#include <sys/mman.h>
#include <time.h>
#include <math.h>

#define SMART_TIME .01   // Smart packet dropping period
#define QUEUE_TIME .1   // Packet bumping
#define ARIV_TIME .005 // 10 microsecond
#define MAX_QUEUE 50
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
};

// Struct for last queued packets
struct SmartPacket {
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

}

/*  */
struct Queue * createPacket(int val) {
    void * shmem = create_shared_memory(sizeof(struct Queue));
    struct Queue *q = (struct Queue *) shmem;
    q->packet = val;
    q->next = NULL;
    q->entry_time = clock();
    return q;
}

void append(struct Queue *q,struct Queue *a) {

}

/* Checks to see if a packet can be ignored (redundancy check) */
int checkSmart(struct SmartPacket *sp,int value) {
    int drop = 0;
    double cur; 
    cur = (double)(clock()-sp[value].entry_time)/CLOCKS_PER_SEC;
    if (cur < SMART_TIME) 
        drop = 0;
    return drop;
}

int numInQueue(struct Queue *q,int m) {
    struct Queue *t;
    t = q;
    int i=0;
    while(i<m) {
        printf("%d ",t->packet);
        if (t->packet != -1) {
            t = t->next;
            i++;
        }else
            break; 
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
    
    printf("Max packets in each Queue: %d\n",pq->max_q);
    // Start Management

    while(*share3 != 1) {
        if ((double)(clock()-pq->med->entry_time)/CLOCKS_PER_SEC > QUEUE_TIME) {
            printf("E");
        }
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
    double starT,stoP;
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
    printf("Confirm max queue: %d\n",pq->max_q);
    
    struct SmartPacket *smartQ;
    smartQ = (struct SmartPacket *) calloc(256,sizeof(struct SmartPacket));
    for(i=0;i<256;i++) {
        smartQ[i].entry_time = clock()-100000;

    }
    i = 0;
    // //read bytes from a file as input data 
    while( i < f_size) {
        
        fread(&byte,1,sizeof(char),fpt);
        bVal = (int) byte;
        //printf("%d\n",bVal);
        if(checkSmart(smartQ,bVal) == 0) {
            
            if (bVal < 30) {
                qt = pq->low;
                for(j=0;j<pq->max_q;j++) {
                    if (qt->packet == -1) {
                        qt->packet = bVal; 
                        qt->entry_time = clock();
                        j =pq->max_q+1;
                        break;
                    }else 
                        qt = qt->next;
                }
                //printf("\n");
                if (j == pq->max_q) {
                    lost[0]++;
                }
            } else if (bVal < 60) {
                qt = pq->med;
                for(j=0;j<pq->max_q;j++) {
                    if (qt->packet == -1) {
                        qt->packet = bVal;
                        qt->entry_time = clock();
                        j =0;
                        break;
                    }else 
                        qt = qt->next;
                }
                if (j == pq->max_q) {
                    lost[1]++;
                }
            } else {
                qt = pq->high;
                for(j=0;j<pq->max_q;j++) {
                    if (qt->packet == -1) {
                        qt->packet = bVal;//createPacket(bVal);
                        qt->entry_time = clock();
                        j =0;
                        break;
                    }else 
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
        if (i % 150 == 0 || i == 6) {
            //printf("low: %d, med %d, high %d\n",numInQueue(pq->low,pq->max_q),numInQueue(pq->med,pq->max_q),numInQueue(pq->high,pq->max_q));
            //printf("%f",((double)(clock()-start))/CLOCKS_PER_SEC);
        }

        sleep(ARIV_TIME);
    }
    i = 1;
    

    fclose(fpt);
    ////write(share1[1],&i,sizeof(int));
    
    *share1 = 1;
    printf("\nSmart Dropped packet Count: %ld\nLost packets (Low, Med, High): %ld %ld %ld\n",smartDrop,lost[0],lost[1],lost[2]);
    
}

/* Send2recpient */

//wait for alleged packet send time
void send2recp(int *share1, int *share2,int *share3, struct Priority *pq) {//int *fd2, int *fd3,int *fd4) {
    int *done,i=0, highq = 0, lowq = 0, medq = 0; 
	//struct Priority *pq; 
	//pq = (struct Priority *) calloc(1,sizeof(struct Priority));
	//sleep(.5);
    //read(fd4[0], pq, sizeof(struct Priority));
    /*
		//read from pipe4 (fd4) the Priority structure
		** see qManage/add2q for example
	*/
    //pq->max_q = 50;
    printf("Confirm max queue send: %d\n",pq->max_q);
    done = share1;
    //done = 0;   
	struct Queue *tempq;
	clock_t q_time;
	double low_time, high_time, med_time;
    low_time = high_time = med_time = 0;
    //sleep(1);
    while(*share1 != 1 || *done == 0) {

		if (pq->high->packet != -1){
            //printf("h");
			tempq = pq->high;
			pq->high = pq->high->next; 
			if (tempq->packet > 59)	{
				q_time = clock() - tempq->entry_time;
				high_time += (double)q_time;
				highq++;
			}
			else if (59 >= tempq->packet > 29){
				q_time = clock() - tempq->entry_time;
				med_time += (double)q_time;
				medq++;
			}
			else {
				q_time = clock() - tempq->entry_time;
				low_time += (double)q_time;
				lowq++;
			}
            tempq->packet = -1;
			//free(tempq);
		}
		else if (pq->med->packet != -1){
            //printf("m");
			tempq = pq->med; 
			pq->med = pq->med->next;
			if (tempq->packet > 59){
				q_time = clock() - tempq->entry_time;
				high_time += q_time;
				highq++;
			}
			else if (59 >= tempq->packet > 29){
				q_time = clock() - tempq->entry_time;
				med_time += (double)q_time;
				medq++;
			}
			else {
				q_time = clock() - tempq->entry_time;
				low_time += (double)q_time;
				lowq++;
			}
            tempq->packet = -1;
			//free(tempq);
		}
		else if (pq->low->packet != -1){
            //printf("l");
			tempq = pq->low;
			pq->low = pq->low->next; 
			if (tempq->packet > 59)	{
				q_time = clock() - tempq->entry_time;
				high_time += q_time;
				highq++;
			}
			else if (59 >= tempq->packet > 29){
				q_time = clock() - tempq->entry_time;
				med_time += (double)q_time;
				medq++;
			}
			else {
				q_time = clock() - tempq->entry_time;
				low_time += (double)q_time;
				lowq++;
			}
            tempq->packet = -1;
			//free(tempq);
		}
		else if (*share1 == 0){
			//break;
		} else {
            done = 0;
        }
		sleep(.005);
        
        if (i % 80) {
            //printf("%d",pq->low,pq->med,pq->high);
            //printf("low: %d, med %d, high %d\n",numInQueue(pq->low,pq->max_q),numInQueue(pq->med,pq->max_q),numInQueue(pq->high,pq->max_q));
        }
        i++;
    }
    
	double avg_ht = high_time/highq; 
	double avg_mt = med_time/medq;
	double avg_lt = low_time/lowq;
	printf("\nAverage time for high priority packets: %f\nAverage time for medium priority packets: %f\nAverage time for low priority packets: %f\n", avg_ht, avg_mt, avg_lt);
	printf("Number of high priority packets processed: %d\nNumber of medium priority packets: %d\nNumber of low priority packets processed: %d\n", highq, medq, lowq);
    *share3 = 1;
}

int main(int argc, char *argv[]) {
    
    char mesg[] = "This is the initial message.";
    char init[] = "This is the initial message.";
    int slen = strlen(mesg);
    //int share1[2]; // Done queuing
    int hold = 0;
    int j,k,pid,pid2;
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
    struct Queue *temp,*q;
    for(j=0;j<3;j++) {
        if (j == 0) {
            temp = pq->high = (struct Queue *) create_shared_memory(sizeof(struct Queue));
            pq->high->packet = -1;
        } else if (j == 1) {
            temp = pq->med = (struct Queue *) create_shared_memory(sizeof(struct Queue));
            pq->med->packet = -1;
        } else {
            temp = pq->low = (struct Queue *) create_shared_memory(sizeof(struct Queue));
            pq->low->packet = -1;
        }
        for(k=0;k<MAX_QUEUE-1;k++) {
            if (j==0) {
                pq->high->next = (struct Queue *) create_shared_memory(sizeof(struct Queue));
                pq->high = pq->high->next;
                pq->high->packet = -1;
                if (k== MAX_QUEUE-2)
                    pq->high->next = temp;
            } else if (j==1) {
                pq->med->next = (struct Queue *) create_shared_memory(sizeof(struct Queue));
                pq->med = pq->med->next;
                pq->med->packet = -1;
                if (k==MAX_QUEUE-2)
                    pq->med->next = temp;
            } else {
                pq->low->next = (struct Queue *) create_shared_memory(sizeof(struct Queue));
                pq->low = pq->low->next;
                
                pq->low->packet = -1;
                if (k== MAX_QUEUE-2)
                    pq->low->next = temp;
            }
            
            
        }
        

    }
    
    //init_priority(pq);
    pq->max_q = MAX_QUEUE;


    pid = fork();
    
    // Add2q
    if (pid == 0) {
        send2recp(share1,share2,share3,pq);
        
    // Send2recip
    } else {
        
        pid2 = fork();
        if (pid2 != 0) {
            add2queue(argv,share1,share2,share3,pq);
            wait(NULL);
        } else {
            qManage(argv,share1,share2,share3,pq);
            wait(NULL);
        }
        
    }


    return 0;
} 
