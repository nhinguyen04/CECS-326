// Nhi Nguyen
// Cecs326 Concurrent Processing and Shared Memory

#define _XOPEN_SOURCE 700 // POSIX functions version 700
#include <stdlib.h> // Standard library
#include <time.h> // Random 
#include <unistd.h> // Fork 
#include <signal.h> // Signal 
#include <stdbool.h> // Boolean 
#include <stdio.h> // I/O 
#include <sys/types.h> // To use pid_t and getpid()
#include <sys/ipc.h> // For interprocess communication
#include <sys/shm.h> // To use shared memory
#include <sys/wait.h> // To use wait declaration
#include <sys/sem.h> // To use emaphore members
#include <semaphore.h> // For semaphores
#include <pthread.h> // Threads
#include <errno.h> // For errors
#include <fcntl.h>

// Variables
#define ROWS 10
#define COLUMNS 10
#define TIME_LIMIT 30

pid_t fishPID;
pid_t pelletPID;

key_t key = 1234; // Key to be passed to shmget()
int shmflg; // Shmflg to be passed to shmget()
int shmid; // Return value from shmget()
char (*streamPtr)[ROWS][COLUMNS]; // 2D array pointer for stream. This will be used as smhget() size

#define SEM_LOCATION "/semaphore"
sem_t (*semaphore); 

// Functions
void GetSharedMemory();
void AttachSharedMemory();
void DetachSharedMemory();
void RemoveSharedMemory();

void CreateSemaphore();
void CloseSemaphore();
void UnlinkSemaphore();

void OnTimeOut();
void OnInterrupt();
void OnTerminate();
void DisplayStream();
void CreateStream();

int main(int argc, char *argv[]) {
	printf("PID %d Swim Mill started\n", getpid());
	
	signal(SIGINT, OnInterrupt); // For handling interrupt signal 
	signal(SIGTERM, OnTerminate); // For handling terminate signal
	signal(SIGALRM, OnTimeOut); // For running out of time
	alarm(30); // Set max time to 30 seconds
	
	GetSharedMemory();
	AttachSharedMemory();
	CreateStream();
	CreateSemaphore();
	
	// Create child process
	fishPID = fork();
	if (fishPID == 0) {
		// Child fish process
		execv("Fish", argv);
		exit(0);
	}
	else if (fishPID > 0) {
		// Child pellet process
		pelletPID = fork();
		if (pelletPID == 0) {
			execv("Pellet", argv);
			exit(0);
		}
		else {
			for (int s = TIME_LIMIT; s >= 0; s--) {
				printf("%d seconds remaining \n", s);
				sleep(1);
				DisplayStream();
			}
			OnTerminate();
		}
	}
	else {
		// Fork failed
		perror("Fork failed");
		exit(3);
	}
	
	return 0;
}

void OnTimeOut() {
	raise(SIGINT); // Raise a signal
}


// Create shared memory segment. Exit if error.
void GetSharedMemory() {
	if ((shmid = shmget(key, sizeof(streamPtr), IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		exit(1);
	}
}

// Attach shared memory segment to data space. Exit if error.
void AttachSharedMemory() {
	if ((streamPtr = shmat(shmid, NULL, 0)) == (void *) - 1) {
		perror("shmat");
		exit(1);
	}
}

// Remove memory pointer from the allocated memory.
void DetachSharedMemory() {
    if (shmdt(streamPtr) == -1) {
        perror("shmdt");
        exit(1);
    }
}

// Remove the allocated memory from shared memory.
void RemoveSharedMemory() {
    if(shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        exit(1);
    }
}

// Create a semaphore from a location if it doesn't already exist.
void CreateSemaphore() {
    if ((semaphore = sem_open(SEM_LOCATION, O_CREAT, 0644, 1)) == SEM_FAILED ) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

// Close a semaphore and deny access.
void CloseSemaphore() {
    if (sem_close(semaphore) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }
}

// Remove semaphore from memory. 
void UnlinkSemaphore() {
	if (sem_unlink(SEM_LOCATION) == -1) {
		perror("sem_unlink");
		exit(EXIT_FAILURE);
	}
}

// Display stream to console. 
void DisplayStream() {
	sem_wait(semaphore);
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLUMNS; c++) {
			printf("%c", (*streamPtr)[r][c]);
		}
		printf("\n");
	}
	printf("\n");
	sem_post(semaphore);
}

// Initialize stream.
void CreateStream() {
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLUMNS; c++) {
			(*streamPtr)[r][c] = '~';
		}
	}
	// Place fish
	(*streamPtr)[ROWS-1][COLUMNS/2] = '^';
}

void OnInterrupt() {
	// Kill fish
	if (kill(fishPID, SIGINT) == -1) {
		printf("PID %d Fish couldn't be killed. \n", fishPID);
	}
	
	// Kill pellet
	if (kill(pelletPID, SIGINT) == -1) {
		printf("PID %d Pellet couldn't be killed. \n", pelletPID);
	}
	
	wait(NULL);
	
	CloseSemaphore();
	UnlinkSemaphore();
	DetachSharedMemory();
	RemoveSharedMemory();
	
	// Print interrupt
	printf("PID: %d Program interrupted.\n", getpid());
	
	exit(0);
}

void OnTerminate() {
	// Kill fish
	if (kill(fishPID, SIGINT) == -1) {
		printf("PID %d Fish couldn't be killed. \n", fishPID);
	}
	
	// Kill pellet
	if (kill(pelletPID, SIGINT) == -1) {
		printf("PID %d Pellet couldn't be killed. \n", pelletPID);
	}
	
	wait(NULL);
	
	CloseSemaphore();
	UnlinkSemaphore();
	DetachSharedMemory();
	RemoveSharedMemory();
	
	// Print interrupt
	printf("PID: %d Program terminated.\n", getpid());
	
	exit(0);
}
