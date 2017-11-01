/* Taylor Clark
CS 4760
Assignment #4
Operating System Simulator: Process Scheduling

I am submitting this project with full knowledge that it does not fully meet the requirements set out in the assignment sheet. What I have included in each of these files however, is commented out code and pseudo-code that I believe would have worked had I more time and know-how to implement them. 
*/

#include <stdbool.h>

/* SEMAPHORE */
#define SEM_NAME "/semaName"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define MUTEX 1

/* ALPHA AND BETA */
#define ALPHA 3
#define BETA 5 

/* SHARED ME KEYS */
#define KEYA 1001
#define KEYB 888
#define KEYC 777

/* CONTROL BLOCK STRUCT */
struct Control_Block {
	pid_t pid;
	int starts;
	int startns;
	int ends;
	int endns;
	int cpuTimeUsed;
	int totalTimeInSys;
	double timeSinceLastBurst;
};

/* A MOMENT OF THE CLOCK */
struct clock_moment{
	int sec;
	int nansec;
};

/* SCHEDULER INFO */
struct scheduler{
	pid_t pid;
	int quantum;
	int done;
};

/* QUEUES */
#define MAX 10
struct Control_Block queue0[MAX];
struct Control_Block queue1[MAX];
struct Control_Block queue2[MAX];
int f0 = 0, f1 = 0, f2 = 0;			// fronts of queues 1 - 3
int r0 = -1, r1 = -1, r2 = -1;		// rears of queues 1 - 3
int ic0 = 0, ic1 = 0, ic2 = 0;		// itemcounts of queues 1 - 3

bool isEmpty(int w) {
   switch(w){
		case 0: { return ic0 == 0; }
		case 1: { return ic1 == 0; }
		case 2: { return ic2 == 0; }
	}
}

bool isFull(int w) {
   switch(w){
		case 0: { return ic0 == MAX; }
		case 1: { return ic1 == MAX; }
		case 2: { return ic2 == MAX; }
	}
}

int size(int w) {
   switch(w){
		case 0: { return ic0; }
		case 1: { return ic1; }
		case 2: { return ic2; }
	}
}  

void insert(struct Control_Block b, int w) {
	if(!isFull(w)) {
		switch(w) {
			case 0: {
				if(r0 == MAX-1) { r0 = -1; }
				queue0[++r0] = b;
				ic0++;
			}
			case 1: {
				if(r1 == MAX-1) { r1 = -1; }       
				queue1[++r1] = b;
				ic1++;
			}
			case 2: {
				if(r2 == MAX-1) { r2 = -1; }       
				queue2[++r2] = b;
				ic2++;
			}
		}
	}
}

void removeB(int w) {
	switch(w){
		case 0: {
			if(f0 == MAX) { f0 = 0; }
			ic0--;
		}
		case 1: {
			if(f1 == MAX) { f1 = 0; }
			ic1--;
		}
		case 2: {
			if(f2 == MAX) { f2 = 0; }
			ic2--;
		}
	}	
}