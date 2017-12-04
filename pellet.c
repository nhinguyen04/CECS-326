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

key_t key = 1234; // Key to be passed to shmget()
int shmflg; // Shmflg to be passed to shmget()
int shmid; // Return value from shmget()
char (*streamPtr)[ROWS][COLUMNS]; // 2D array pointer for stream. This will be used as smhget() size

#define SEM_LOCATION "/semaphore"
sem_t (*semaphore); 

const int MAX_THREADS = 20; // For pellets
const int MAX_INTERVAL = 3;
const int MIN_INTERVAL = 1;

static void *child(void*);
static int TOTAL_THREADS = 30;

// Functions
void GetSharedMemory();
void AttachSharedMemory();
void DetachSharedMemory();
void RemoveSharedMemory();

void OpenSemaphore();
void CloseSemaphore();

void OnInterrupt();
void OnTerminate();

int main(int argc, char *argv[]) {
    printf("PID %d Pellet started\n", getpid());

	// Signal handling
    signal(SIGINT, OnInterrupt); 
	signal(SIGTERM, OnTerminate); 
    
    // Attach shared memory
    GetSharedMemory();
    AttachSharedMemory();
	OpenSemaphore();
    
    // Initialize PRNG
    srand(time(NULL));
    
    // Create the thread array to make multiple pellets
    pthread_t threads[TOTAL_THREADS];
    
    //Create threads until the maximum is reached.
    int threadCounter = 0;
    for(int i = 0; i < TIME_LIMIT; i++) {
        if(threadCounter <= MAX_THREADS) {
            // Sleep for random intervals
            int sleepTime = rand() % MAX_INTERVAL + MIN_INTERVAL;
            sleep(sleepTime);
            
            // Create new pellet thread
            pthread_create(&threads[threadCounter], NULL, child, NULL);
        }
    }
    
    // Continue when the thread list is empty
    pthread_join(threads[TOTAL_THREADS-1], NULL);
    
    DetachSharedMemory();
    printf("PID %d Pellet exited\n", getpid());
    exit(0);
}

static void *child(void* ignored) {
    // Randomize pellet position
    int pelletRow;
	int pelletCol;
    do {
        pelletRow = rand()%8+1;
        pelletCol = rand()%8+1;
    }
    while((*streamPtr)[pelletRow][pelletCol] == 'o');
    
    bool eaten = false;
	bool leftStream = false;
    
    sem_wait(semaphore); // Lock
    printf("Pellet %d was dropped at [%d,%d]\n", (int)pthread_self(), pelletCol, pelletRow);
    sem_post(semaphore); // Unlock
	
    while(1) {
        (*streamPtr)[pelletCol][pelletRow] = 'o';
        
        // Move pellet downstream
        sleep(1);
        pelletCol++;
        
        // Check if pellet was eaten or left stream
        if(pelletCol == ROWS) {
            leftStream = true;
		}
        else if((*streamPtr)[pelletCol][pelletRow] == '^') {
            eaten = true;
		}
        
        // Fix symbol in last position
        if((*streamPtr)[pelletCol-1][pelletRow] != '^') {
            sem_wait(semaphore);
            (*streamPtr)[pelletCol-1][pelletRow] = '~';
            sem_post(semaphore);
        }
        
        if(eaten || leftStream) {
            break;
		}
    }
    
    // Write results to console
    sem_wait(semaphore);
    if(eaten) {
        printf("Pellet %d was eaten at column %d!\n", (int)pthread_self(), pelletRow);
	}
    else {
        printf("Pellet %d wasn't eaten and left stream at column %d\n", (int)pthread_self(), pelletRow);
	}
    sem_post(semaphore);
    
    printf("Pellet %d is exiting.\n", (int)pthread_self());
    return NULL;
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

// Create a semaphore from a location.
void OpenSemaphore() {
    if ((semaphore = sem_open(SEM_LOCATION, 0)) == SEM_FAILED ) {
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

// Handler for signal to interrupt.
void OnInterrupt() {
	wait(NULL); // Wait for all child processes to die
	CloseSemaphore();
	DetachSharedMemory();
	printf("PID %d Pellet killed from interrupt.\n", getpid());
	exit(0);
}

// Handler for signal to end.
void OnTerminate() {
	wait(NULL); // Wait for all child processes to die
	CloseSemaphore();
	DetachSharedMemory();
	printf("PID %d Pellet killed because program terminated.\n", getpid());
	exit(0);
}
