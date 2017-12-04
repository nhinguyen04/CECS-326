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
int fishColumn = ROWS/2; 

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

void OpenSemaphore();
void CloseSemaphore();

void OnInterrupt();
void OnTerminate();
void MoveFish(int);

int main(int argc, char *argv[]) {
	
	// Start fish with pid
	printf("PID %d Fish started \n", getpid());
	
	signal(SIGINT, OnInterrupt); // For handling interrupt signal 
	signal(SIGTERM, OnTerminate); // For handling terminate signal
	
	// Attach shared memory
	GetSharedMemory();
	AttachSharedMemory();
	
	while(1) {
		// Iterate through stream and find rows with pellets
		for(int row = (ROWS-2); row >= 0; row--) {
            // Locate any pellets on the specific row
            int currentRowPellets[COLUMNS] = {0};
            for (int col = 0; col < COLUMNS; col++) {
                if((*streamPtr)[row][col] == 'o') {
                    currentRowPellets[col] = 1;
                }
            }
            
            // Don't move if pellet is coming at fish
            if(row == (ROWS-2) && currentRowPellets[fishColumn] == 1) {
                sleep(1);
                break;
            }
            else {
                // Find closest pellet for later.
                int closestPellet = -1;
                for(int col = 0; col < COLUMNS; col++) {
                    if(currentRowPellets[col] == 1) {
                        closestPellet = col;
                        break;
                    }
                }
                
                // If the fish can reach the pellet
                if(closestPellet != -1) {
                    // Move left if pellet is left
                    if(closestPellet > fishColumn) {
                        sem_wait(semaphore);
                        MoveFish(1);
                        sem_post(semaphore);
                    }
                    
                    // Move right if pellet is right
                    else {
                        sem_wait(semaphore);
                        MoveFish(-1);
                        sem_post(semaphore);
                    }

                    sleep(1);
                    break;
                }
            }
        }
    }
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

// Get a semaphore from a location.
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

// Move fish left if direction is negative and right if direction is positive.
void MoveFish(int direction) {
    // Must not forget to update last position.
    (*streamPtr)[ROWS-1][fishColumn] = '~';
    fishColumn += direction;
    (*streamPtr)[ROWS-1][fishColumn] = '^';
}

// Handler for signal to interrupt.
void OnInterrupt() {
	CloseSemaphore();
	DetachSharedMemory();
	printf("PID %d Fish killed from interrupt.\n", getpid());
	exit(0);
}

// Handler for signal to end.
void OnTerminate() {
	CloseSemaphore();
	DetachSharedMemory();
	printf("PID %d Fish killed because program terminated.\n", getpid());
	exit(0);
}
