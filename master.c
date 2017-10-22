/* Taylor Clark
CS 4760
Assignment #4
Operating System Simulator: Process Scheduling
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <signal.h>
#include "conb.h"

char errstr[50];
FILE *f;
int shmidA, shmidB;

void sig_handler(int);
void clean_up(void);
void blockInit(struct Control_Block);

struct clock_moment{
	int sec;
	int nansec;
};

int main(int argc, char *argv[]){
	
	srand(time(NULL));
	
	/* BIT VECTOR */
	int make = 2;
	int useVector[2];
	int i;
	for (i = 0; i < make; i++){
		useVector[i] = 0;
	}
	
	/* SETTING UP SIGNAL HANDLING */
	signal(SIGINT, sig_handler);
	
	/* VARIOUS VARIABLES */
	// 1,000,000,000 ns = 1 seconds
	int overhead = 0;				// used to simulate overhead in sys in ns
									// random gen of [0, 1000]
									
	int simTimeEnd = 10;			// when to end the simulation
	
	struct clock_moment lastSpawn;	// last time a process was spawned
	lastSpawn.sec = 0;
	lastSpawn.nansec = 0;
	
	pid_t pid, cpid;
	char fname[] = "log.out";		// file name
	/* VARIOUS VARIABLES END */
	
	
	// the timer information
    time_t start;
	
	// for printing errors
	snprintf(errstr, sizeof errstr, "%s: Error: ", argv[0]);
	int c;
	opterr = 0;	

	/* COMMAND LINE PARSING */
	while ((c = getopt(argc, argv, ":hl:")) != -1) {
		switch (c) {
			// setting name of file of palindromes to read from	
			// by default it is palindromes.txt
			case 'l': {
				int result;
				char exten[] = ".out";
				char newname[200];
				strcpy(newname, optarg);
				strcat(newname, exten);
				result = rename(fname, newname);
				if(result == 0) {
					printf ("File renamed from %s to %s\n", fname, newname);
					strcpy(fname, newname);
				}
				else {
					perror(errstr);
					printf("Error renaming file.\n");
				}
				break;
			}
			// set the amount of simulated time to run for
			case 'o': {
				simTimeEnd = atoi(optarg);
				printf ("Setting simulated clock end time to: %i nanoseconds\n", simTimeEnd);
				break;
			}
			// show help
			case 'h': {
				printf("\n----------\n");
				printf("HELP LIST: \n");
				printf("-h: \n");
				printf("\tShow help, valid options and required arguments. \n");
				
				printf("-l: \n");
				printf("\t Sets the name of the log file to output to.\n");
				printf("\t Default is log.out.\n");
				printf("\tex: -l filename \n");
				
				printf("-o: \n");
				printf("\t Sets the simulated time to end at.\n");
				printf("\t Default is 2 simulated seconds.\n");
				printf("\tex: -o 10 \n");
				
				printf("-t: \n");
				printf("\tSets the amount of real time seconds to wait before terminating program. \n");
				printf("\tDefault is 20 seconds. Must be a number.\n");
				printf("\tex: -t 60 \n");
				
				printf("----------\n\n");
				break;
			}
			// if no argument is given, print an error and end.
			case ':': {
				perror(errstr);
				fprintf(stderr, "-%s requires an argument. \n", optopt);
				return 1;
			}
			// if an invalid option is caught, print that it is invalid and end
			case '?': {
				perror(errstr);
				fprintf(stderr, "Invalid option(s): %s \n", optopt);
				return 1;
			}
		}
	}
	/* COMMAND LINE PARSING END */
	
	
	/* SHARED MEMORY */
	// shared memory clock
	// [0] is seconds, [1] is nanoseconds
	int *clock;
	struct Control_Block *blocks;
	
	// create segment to hold all the info from file
	if ((shmidA = shmget(KEYA, 50, IPC_CREAT | 0666)) < 0) {
        perror("Master shmget failed.");
        exit(1);
    }
	else if ((shmidB = shmget(KEYB, (sizeof(struct Control_Block)*make), IPC_CREAT | 0666)) < 0) {
        perror("Master shmget failed.");
        exit(1);
    }
	
	// attach segment to data space
	if ((clock = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
        exit(1);
    }
	else if ((blocks = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
        exit(1);
    }
		
	// write to shared memory the intial clock settings
	// clear out shmMsg
	clock[0] = 0;			// seconds
	clock[1] = 0;			// nanoseconds
	/* SHARED MEMORY END*/

	
	// open file for writing to
	// will rewrite the file everytime
	f = fopen(fname, "w");
	if(f == NULL){
		perror(errstr);
		printf("Error opening file.\n");
		exit(1);
	}
	
	struct Control_Block b;
	/* FORK PROCESSES */
	for(i = 0; i < make; i++){
		// set up process control block and copy to shared mem
		blockInit(b);
		memcpy(&blocks[make], &b, sizeof(struct Control_Block));
		
		pid = fork();
		if (pid < 0) {
			perror(errstr); 
			printf("Fork failed!\n");
			exit(1);
		}
		if (pid == 0){
			// exec the child
			char n[5];
			sprintf(n, "%i", make);
			execlp("user", "user", n, NULL);
			perror(errstr); 
			printf("execl() failed!\n");
			exit(1);
		}
	}
		
	// calculate end time
	start = time(NULL);
	fprintf(f, "Master: Starting clock loop at %s", ctime(&start));
	fprintf(f, "\n-------------------------\n\n");
	
	/* WHILE LOOP */
    while (clock[0] < simTimeEnd) {  
		
		// when to spawn a new process (seconds)
		int when = rand() % 2;		
		
		// increment the clock (ns)
		overhead = rand() % 1001;
		clock[1] += overhead;
		if(clock[1] - 1000000000 > 0){
			clock[1] -= 1000000000; 
			clock[0] += 1;
		}
		
	}
	wait(NULL);
	fprintf(f, "\n-------------------------\n\n");
	start = time(NULL);
	printf("Master: Ending clock loop at %s", ctime(&start));
	printf("Master: Simulated time ending at: %i seconds, %i nanoseconds.\n", clock[0], clock[1]);
	
	for (i = 0; i < make; i++){
		printf("Child PID: %ld\n", (blocks+i)->pid);
		printf("\tCPU Used: %i \n\tTime in Sys: %i\n",(blocks+i)->cpuTimeUsed, (blocks+i)->totalTimeInSys);
	}
	
	/* CLEAN UP */ 
	clean_up();

    return 0;
}

/* CLEAN UP */ 
// release all shared memory, malloc'd memory, semaphores and close files.
void clean_up(){
	int i;
	printf("Master: Cleaning up now.\n");
	shmctl(shmidA, IPC_RMID, NULL);
	shmctl(shmidB, IPC_RMID, NULL);
	fclose(f);
}

/* SIGNAL HANDLER */
// catches SIGINT (Ctrl+C) and notifies user that it did.
// it then cleans up and ends the program.
void sig_handler(int signo){
	if (signo == SIGINT){
		printf("Master: Caught Ctrl-C.\n");
		clean_up();
		exit(0);
	}
}

void blockInit(struct Control_Block b){
	b.cpuTimeUsed = 0;
	b.totalTimeInSys = 0;
	b.timeSinceLastBurst = 0;
	b.quantum = 0;
	b.doWhat = 0;
}