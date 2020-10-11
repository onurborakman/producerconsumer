/*
*   Onur Akman
*   CST-221
*   10/11/2020
*   Michael Landreth
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdbool.h>
//Added pthread library in order to use the threads
#include <pthread.h>
// Constants
int MAX = 100;
int WAKEUP = SIGUSR1;
int SLEEP = SIGUSR2;

// The Child PID if the Parent else the Parent PID if the Child
pid_t otherPid;

// A Signal Set
sigset_t sigSet;

// Shared Circular Buffer
struct CIRCULAR_BUFFER{
    int count;          // Number of items in the buffer
    int read;          // Next slot to read in the buffer
    int write;          // Next slot to write in the buffer
    int buffer[100];
};
struct CIRCULAR_BUFFER *buffer = NULL;

/****************************************************************************************************/

// This method will put the current Process to sleep forever until it is awoken by the WAKEUP signal
void *sleepAndWait(){
    int nSig;
    printf("Sleeping...........\n");
    //Sleep until notified to wake up using the sigwait() method
    sigwait(&sigSet, &nSig);

    printf("Awoken\n");
    //Signal back to let the process know that the signal has been received and you may continue
    kill(otherPid, WAKEUP);
}

// This method will signal the Other Process to WAKEUP
void *wakeupOther(){
  int nSig;
	//Signal Other Process to wakeup using the kill() method
  kill(otherPid, WAKEUP);
  //Wait for the SIGNAL RECEIVED signal in order to continue
  sigwait(&sigSet, &nSig);
}

// Gets a value from the shared buffer
void get(struct CIRCULAR_BUFFER* buffer){
    // Get a value from the Circular Buffer and adjust where to read from next
    int read = buffer->buffer[buffer->read];
    // Print
    printf("--------------Consuming: %i\n", read);
    // Adjust where to read from next
    buffer->read = buffer->read + 1;
}

// Puts a value in the shared buffer
void put(struct CIRCULAR_BUFFER* buffer, int i){
    // Write to the next available position in the Circular Buffer and adjust where to write next
    buffer->buffer[buffer->write] = i;
    // Print
    printf("%i\n", i);
    // Increment the count
    buffer->count = buffer->count + 1;
    // Adjust where to write next
    buffer->write = buffer->write + 1;
}
/****************************************************************************************************/

/**
 * Logic to run to the Consumer Process
 **/
void consumer(){
    // Set Signal Set to watch for WAKEUP signal
    sigemptyset(&sigSet);
    sigaddset(&sigSet, WAKEUP);
    sigprocmask(SIG_BLOCK, &sigSet, NULL);

    // Run the Consumer Logic
    printf("Running Consumer Process.....\n");

    // Implement Consumer Logic (see page 129 in book)
    // Start producing from 1 to 100
    int i = 1;

    // Loop until MAX value
    while(i <= MAX){
      // If the buffer array is empty then sleep and wait
      if(buffer->count == 0){
        pthread_t a;
        pthread_create(&a, 0, sleepAndWait, 0);
        pthread_join(a, 0);
      }
      // Get value from the buffer array
      get(buffer);
      // Increment the i integer in order to move forward with while
      ++i;
    }
    // Exit cleanly from the Consumer Process
    _exit(1);
}

/**
 * Logic to run to the Producer Process
 **/
void producer(){
  sigemptyset(&sigSet);
  sigaddset(&sigSet, WAKEUP);
  sigprocmask(SIG_BLOCK, &sigSet, NULL);
    // For while loop in order to produce right amount of integers
    int i = 1;

    // Run the Producer Logic
    printf("Running Producer Process.....\n");

    // Implement Producer Logic (see page 129 in book)
    // Produce until MAX is reached
    while(i <= MAX){
        // If buffer array is full then go to sleep
        if(buffer->count == MAX){
            printf("Producer has reached the maximum capacity...\n");
            printf("Producer is getting ready to go to sleep...\n");

            pthread_t a;
            pthread_create(&a, 0, sleepAndWait, 0);
            pthread_join(a, 0);
        }
        printf("Producing: ");
        // Place the integer value from buffer to the screen
        put(buffer ,i);
        // Wake up the consumer if the production starts
        if(buffer->count == 1){
            pthread_t a;
            pthread_create(&a, 0, wakeupOther, 0);
            pthread_join(a, 0);
        }

        // Increment the value in order to end the while loop on point
        ++i;
    }

    // Exit cleanly from the Consumer Process
    _exit(1);
}

/**
 * Main application entry point to demonstrate forking off a child process that will run concurrently with this process.
 *
 * @return 1 if error or 0 if OK returned to code the caller.
 */
int main(int argc, char* argv[]){
    pid_t  pid;

    // Create shared memory for the Circular Buffer to be shared between the Parent and Child  Processes
    buffer = (struct CIRCULAR_BUFFER*)mmap(0,sizeof(buffer), PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    buffer->count = 0;
    buffer->read = 0;
    buffer->write = 0;

    // Use fork()
    pid = fork();


    if (pid == -1){
        // Error: If fork() returns -1 then an error happened (for example, number of processes reached the limit).
        printf("Can't fork, error %d\n", errno);
        exit(EXIT_FAILURE);
    }
    // OK: If fork() returns non zero then the parent process is running else child process is running
    if (pid == 0){
        // Run Producer Process logic as a Child Process
        otherPid = getppid();
        producer();

    }else{
        // Run Consumer Process logic as a Parent Process
        otherPid = pid;
        consumer();
  }

    // Return OK
    return 0;
}
