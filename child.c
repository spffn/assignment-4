/* Taylor Clark
CS 4760
Assignment #4
Operating System Simulator: Process Scheduling
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
#include "conb.h"

int main(int argc, char *argv[]){
	
	pid_t pid = (long)getpid();		// process id
	
	/* VARIOUS VARIABLES */
	srand(time(NULL));
	int doWhat;						// generated randomly between 0 and 3 to indicate
									// what the process "did" while running
									// 0 = terminate
									// 1 = terminated at time quantum
									// 2 = "wait" for r.s seconds
									// 3 = preempted after using p% of time quantum
	int r, s, p;					/* RANDOMLY GENERATED NUMBERS */
									// r = [0 ,5]
									// s = [0, 1000]
									// p = [1, 99]
	int which = atoi(argv[1]);
									
	int run = rand() % 1;
	int amo;
	
	/* SHARED MEMORY */
	int shmidA, shmidB;
	
	// locate the segments
	if ((shmidA = shmget(KEYA, 50, 0666)) < 0) {
        perror("child shmget A failed");
        exit(1);
    }
	else if ((shmidB = shmget(KEYB, sizeof(struct Control_Block), 0666)) < 0) {
        perror("child shmget B failed");
        exit(1);
    }
	
	// attach to our data space
	int *clock;
	struct Control_Block *b;
	if ((clock = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("child shmat A failed");
        exit(1);
    }
	else if ((b = shmat(shmidB, NULL, 0)) == (char *) -1) {
        perror("child shmat B failed.");
        exit(1);
    }
	/* SHARED MEMORY END*/
	
	(b+which)->pid = getpid();
	printf("Child PID: %ld\n", (b+which)->pid);
	printf("\tCPU Used: %i \n\tTime in Sys: %i\n",(b+which)->cpuTimeUsed, (b+which)->totalTimeInSys);
	
	return 0;
}