/* Taylor Clark
CS 4760
Assignment #4
Operating System Simulator: Process Scheduling

I am submitting this project with full knowledge that it does not fully meet the requirements set out in the assignment sheet. What I have included in each of these files however, is commented out code and pseudo-code that I believe would have worked had I more time and know-how to implement them. 
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
#include <semaphore.h>
#include "conb.h"

char errstr[50];
FILE *f;
int shmidA, shmidB, shmidC;
sem_t *semaphore;
struct Control_Block *b;
int make;

void sig_handler(int);
void clean_up(void);

int main(int argc, char *argv[]){
	
	srand(time(NULL));
	
	/* SEMAPHORE INFO */
	// init semaphore
	semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, SEM_PERMS, MUTEX);
	if (semaphore == SEM_FAILED) {
		printf("Master: ");
        perror("sem_open(3) error");
		clean_up();
        exit(EXIT_FAILURE);
    }
	
	// close it because we dont use the semaphore in the parent and we want it to
	// autodie once all processes using it have ended
	if (sem_close(semaphore) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
	/* SEMAPHORE INFO END */
	
	/* BIT VECTOR */
	make = 10;
	int useVector[10];
	int i;
	for (i = 0; i < make; i++){
		useVector[i] = 0;
	}
	
	int q0wait, q1wait, q2wait;		// avg wait time in queues
	int q0q, q1q, q2q;				// queue quantums
	
	/* SETTING UP SIGNAL HANDLING */
	signal(SIGINT, sig_handler);
	
	/* VARIOUS VARIABLES */
	// 1,000,000,000 ns = 1 seconds
	int overhead = 0;				// used to simulate overhead in sys in ns
									// random gen of [0, 1000]					
	int simTimeEnd = 10;				// when to end the simulation
	struct clock_moment last;		// last time a process was spawned
	
	pid_t pid, cpid;
	char fname[] = "log.out";		// file name
	
    time_t start;					// the timer information
	/* VARIOUS VARIABLES END */
	
	
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
	struct scheduler *sched;
	
	// create segments to hold all the info from file
	if ((shmidA = shmget(KEYA, 50, IPC_CREAT | 0666)) < 0) {
        perror("Master shmgetA failed.");
		clean_up();
        exit(1);
    }
	else if ((shmidB = shmget(KEYB, (sizeof(struct Control_Block)*make), IPC_CREAT | 0666)) < 0) {
        perror("Master shmgetB failed.");
		clean_up();
        exit(1);
    }
	else if ((shmidC = shmget(KEYC, 50, IPC_CREAT | 0666)) < 0) {
        perror("Master shmgetC failed.");
		clean_up();
        exit(1);
    }
	
	// attach segments to data space
	if ((clock = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
		clean_up();
        exit(1);
    }
	else if ((b = shmat(shmidB, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
		clean_up();
        exit(1);
    }
	else if ((sched = shmat(shmidC, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
		clean_up();
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
		clean_up();
		exit(1);
	}

	/* FORK PROCESSES */
	for(i = 0; i < make; i++){
		pid = fork();
		if (pid < 0) {
			perror(errstr); 
			printf("Fork failed!\n");
			clean_up();
			exit(1);
		}
		if (pid == 0){
			b[i].cpuTimeUsed = 0;
			b[i].totalTimeInSys = 0;
			b[i].timeSinceLastBurst = 0;
			b[i].starts = 0;
			b[i].startns = 0;
			
			// exec the child
			char n[5];
			sprintf(n, "%i", make);
			char w[5];
			sprintf(w, "%i", i);
			execlp("user", "user", n, w, NULL);
			perror(errstr); 
			printf("execl() failed!\n");
			clean_up();
			exit(1);
		}
		useVector[i] = 1;
		insert(b[i], 0);
	}
	
	// set current sim clock time of last child spawned
	last.sec = clock[0];
	last.nansec = clock[1];
		
	// calculate end time
	start = time(NULL);
	printf("Starting scheduler...\n");
	fprintf(f, "Master: Starting clock loop at %s", ctime(&start));
	fprintf(f, "\n-------------------------\n\n");
	int q, k;
	int quant = 200; 
	
	/* WHILE LOOP */
	sleep(3);
	
	k = 0;
	//schedule first process
	fprintf(f, "Dispatching process %ld at %i.%i.\n", sched->pid, clock[0], clock[1]);
	sched->pid = b[k].pid;
	sched->quantum = quant;
	sched->done = 0;
	
    while (clock[0] < simTimeEnd) {  
	
		// check if the scheduled process is done yet
		// if it is, remove it from position in queue and clear use vector
		if(sched->done == 1){
			fprintf(f, "Process %ld has completed at %i.%i.\n", sched->pid, clock[0], clock[1]);
			for(q = 0; q < make; q++){ 
				if(b[q].pid == sched->pid) { 
					useVector[q] = 0;
					fprintf(f, "Child PID: %ld\n", (b+q)->pid);
					fprintf(f, "\tStart Time (Sim): %i.%i s \n\tEnd Time (Sim): %i.%i s\n",(b+q)->starts, (b+q)->startns, (b+q)->ends, (b+q)->endns);
					fprintf(f, "\tCPU Time Used: %i ns \n\tTime in Sys: ~%d seconds\n",(b+q)->cpuTimeUsed, (b+q)->totalTimeInSys);
					k++;
					q = 100;
				}
			}
			
			// schedule a new child
			if(k > make) { k = 0; }
			if(useVector[k] != 0){
				sched->pid = b[k].pid;
				sched->quantum = quant;
				sched->done = 0;
				fprintf(f, "Dispatching process %ld at %i.%i.\n", sched->pid, clock[0], clock[1]);
			}		
		}
		
		// when to spawn a new process (seconds)
		int when = rand() % 2;		
		
		// increment the clock (ns)
		overhead = rand() % 1001;
		clock[1] += overhead;
		if(clock[1] - 1000000000 > 0){
			clock[1] -= 1000000000; 
			clock[0] += 1;
		}
		
		// check to see if the appropriate amount of time has passed before
		// spawning a new child
		if(clock[0] > last.sec + when){
			// if yes, check the useVector for an open space
			for(i = 0; i < make; i++){
				if(useVector[i] == 0){
					fprintf(f, "(!!) Opening at %i. Spawning new child. \n", i);
					pid = fork();
					// spawn new kid
					if (pid == 0){
						b[i].cpuTimeUsed = 0;
						b[i].totalTimeInSys = 0;
						b[i].timeSinceLastBurst = 0;
						b[i].starts = 0;
						b[i].startns = 0;
						b[i].ends = 0;
						b[i].endns = 0;
				
						// set current sim clock time of last child spawned
						last.sec = clock[0];
						last.nansec = clock[1];
						
						// exec the child
						char n[5];
						sprintf(n, "%i", make);
						char w[5];
						sprintf(w, "%i", i);
						execlp("user", "user", n, w, NULL);
						perror(errstr); 
						printf("execl() failed!\n");
						clean_up();
						exit(1);
					}
					useVector[i] = 1;
				}
			}
		}
		
	}
	
	fprintf(f, "Master: Time's up!\n");
	wait(NULL);
	fprintf(f, "\n-------------------------\n\n");
	start = time(NULL);
	fprintf(f, "Master: Ending clock loop at %s", ctime(&start));
	fprintf(f, "Master: Simulated time ending at: %i seconds, %i nanoseconds.\n", clock[0], clock[1]);
	printf("Finished! Please see output file for details.\n");
	
	/* CLEAN UP */ 
	clean_up();

    return 0;
}

/* CLEAN UP */ 
// release all shared memory, malloc'd memory, semaphores and close files.
void clean_up(){
	printf("Master: Cleaning up now.\n");
	
	int i;
	for(i = 0; i < make; i++){
		fprintf(f, "Master: Killing process %ld.\n", b[i].pid);
		kill(b[i].pid, SIGTERM);
	}
	
	shmctl(shmidA, IPC_RMID, NULL);
	shmctl(shmidB, IPC_RMID, NULL);
	shmctl(shmidC, IPC_RMID, NULL);
	if (sem_unlink(SEM_NAME) < 0){
		perror("sem_unlink(3) failed");
	}
	sem_close(semaphore);
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
