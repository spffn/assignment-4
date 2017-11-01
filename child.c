/* Taylor Clark
CS 4760
Assignment #4
Operating System Simulator: Process Scheduling

I am submitting this project with full knowledge that it does not fully meet the requirements set out in the assignment sheet. What I have included in each of these files however, is commented out code and pseudo-code that I believe would have worked had I more time and know-how to implement them. 
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "conb.h"

int main(int argc, char *argv[]){
	
	pid_t pid = (long)getpid();		// process id
	
	/* SEMAPHORE INFO */
	sem_t *semaphore = sem_open(SEM_NAME, O_RDWR);
	if (semaphore == SEM_FAILED) {
		printf("%ld: ", pid);
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }
	/* SEMAPHORE INFO END */
	
	/* VARIOUS VARIABLES */
	srand(time(NULL));
	int doWhat;						// generated randomly between 0 and 3 to pick
									// what the process "did" while running
									// 0 = terminate
									// 1 = terminated at time quantum
									// 2 = "wait" for r.s seconds
									// 3 = preempted after using p% of time quantum
									/* RANDOMLY GENERATED NUMBERS */
	int r, s, p;					// r = [0 ,5]						
									// s = [0, 1000]
									// p = [1, 99]								
	
	int m = atoi(argv[1]);
	int which = atoi(argv[2]);
									
	int run = rand() % 1;
	int amo;
	
	/* SHARED MEMORY */
	int shmidA, shmidB, shmidC;
	
	// locate the segments
	if ((shmidA = shmget(KEYA, 50, 0666)) < 0) {
        perror("child shmget A failed");
        exit(1);
    }
	else if ((shmidB = shmget(KEYB, m * sizeof(struct Control_Block), 0666)) < 0) {
        perror("child shmget B failed");
        exit(1);
    }
	else if ((shmidC = shmget(KEYC, 50, 0666)) < 0) {
        perror("child shmget C failed");
        exit(1);
    }
	
	// attach to our data space
	int *clock;
	struct Control_Block *b;
	struct scheduler *sched;
	
	if ((clock = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("child shmat A failed");
        exit(1);
    }
	else if ((b = shmat(shmidB, NULL, 0)) == (char *) -1) {
        perror("child shmat B failed.");
        exit(1);
    }
	else if ((sched = shmat(shmidC, NULL, 0)) == (char *) -1) {
        perror("child shmat C failed.");
        exit(1);
    }
	/* SHARED MEMORY END*/
	
	b[which].pid = getpid();		// set process id immediately
	int stop = 0, sl = 0;
	
	// enter while loop
	while(stop == 0){
		// if its the processes turn to "run", generate a random value to decide
		// "how" it runs
		if(sched->pid == b[which].pid && sched->done == 0){
			// if run = 0, then only use a portion of its quantum
			// update the amount of cpu time used to that
			if(run == 0) {
				amo = rand() % sched->quantum;
				b[which].cpuTimeUsed += amo;
			}
			// else it ran for its whole quantum, so add it to cpu time used
			else{
				b[which].cpuTimeUsed += sched->quantum;
			}
			// time is up, so stop the running, and signal in the semaphore that
			// a new process can be scheduled
			stop = 1;
			
			// try to send a message to oss, marking itself as done
			while(sl == 0){
				if(sem_trywait(semaphore) != 0){ continue; }
				sched->done = 1;
				sem_post(semaphore);
				sl = 1;
			}
		}
	}
	
	(b+which)->ends = clock[0];
	(b+which)->endns = clock[1];
	(b+which)->totalTimeInSys = clock[0] + ((double)clock[1]/1000000000);
	
	return 0;
}