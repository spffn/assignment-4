/* ALPHA AND BETA */
#define ALPHA 3
#define BETA 5 

/* SHARED ME KEYS */
#define KEYA 1001
#define KEYB 888

/* CONTROL BLOCK STRUCT */
struct Control_Block {
	pid_t pid;
	int cpuTimeUsed;
	int totalTimeInSys;
	int timeSinceLastBurst;
	int quantum;
	int doWhat;
};