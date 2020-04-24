/*
* Bekir Koray Kural
* 150170053
* Development environment: Ubuntu 18.04
* Compile and link: gcc -std=c99 -lpthread main.c - main
* To compile: gcc -std=c99 -D_SVID_SOURCE -D_GNU_SOURCE -lpthread main.c -o main
* To run: ./main interval_min interval_max np nt
* Example: ./main 1 200 2 2
*/
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int id;
    int interval_start;
    int interval_end;
    int* shm;
    int num_children;
} ProcessArguments;

typedef struct {
    int process_id;
    int id;
    int interval_start;
    int interval_end;
} ThreadArguments;

void process_function(ProcessArguments *args);
void* thread_function(void *args);
bool isPrime(int number);

int main(int argc, char* argv[])
{
    // Check argument count
    if(argc != 5) {
        printf("4 Arguments requiered, returning\n");
        return EXIT_FAILURE;
    }

    printf("Master: Started.\n");

    // Get Arguments
    char *ptr;
    int interval_start = strtol(argv[1], &ptr, 10);
    int interval_end = strtol(argv[2], &ptr, 10);
    int num_processes = strtol(argv[3], &ptr, 10);
    int num_threads = strtol(argv[4], &ptr, 10);

    // Calculate interval length of each process
    int interval_steps = (interval_end - interval_start) / num_processes;

    // Create shared memory for each child process
    int *shmids = malloc(sizeof(int) * num_processes);
    for (int i = 0; i < num_processes; i++)
    {
        key_t key = ftok("main.c",i);
        int size = sizeof(int) * interval_steps / 2;
        shmids[i] = shmget(key, size, 0666|IPC_CREAT); 
    }
    
    // Create child processes
    for(int i = 0; i < num_processes; i++)
    {
        int pid = fork();

        // Fork error
        if(pid < 0) {
            printf("Fork failed, returning\n");
            return EXIT_FAILURE;
        }

        // Child process
        else if(pid == 0) {
            // Calculate interval of process
            int subinterval_start = interval_start + (interval_steps * i) + i;
            int subinterval_end = interval_start + (interval_steps * (i+1)) + i;

            // Pointer to shared memory of this process
            int *ptr = (int*) shmat(shmids[i],(void*)0,0);

            // Create Arguments for process function
            ProcessArguments *args = malloc(sizeof(ProcessArguments));
            args->interval_start = subinterval_start;
            args->interval_end = subinterval_end;
            args->num_children = num_threads;
            args->shm = ptr;
            args->id = i;

            process_function(args);

            // Detach from shared memory and free Arguments
            free(args);
            shmdt(ptr);

            // Exit child process
            exit(0);
            break;
        }

        // Main process does nothing here
        else continue;
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0) ;

    // Print results
    printf("Master: Done. Prime numbers are: ");
    for (int i = 0; i < num_processes; i++)
    {
        // Pointer to shared memory of i'th process
        int *ptr = (int*) shmat(shmids[i],(void*)0,0);

        // Print values until the end
        while (*ptr != -1) {
            printf("%d, ", *ptr);
            ptr++;
        }

        // Detach and destroy shared memory
        shmdt(ptr);
        shmctl(shmids[i],IPC_RMID,NULL); 
    }
    printf("\n");

    return EXIT_SUCCESS;
}




void process_function(ProcessArguments* param)
{
    printf("Slave %d Started. Interval %d-%d\n",param->id,param->interval_start,param->interval_end);
    
    // Pointer to shared memory of this process
    int *ptr_shm = param->shm;

    // Interval length of each thread
    int interval_steps = (param->interval_end - param->interval_start) / param->num_children;

    // Array of thread ids and thread_function arguments
    pthread_t ids[param->num_children];
    ThreadArguments *args[param->num_children];

    // Create threads
    for (int i = 0; i < param->num_children; i++)
    {
        // Calculate interval of thread
        int subinterval_start = param->interval_start + (interval_steps * i) + i;
        int subinterval_end = param->interval_start + (interval_steps * (i+1)) + i;

        // Create Arguments of thread function
        args[i] = malloc(sizeof(ThreadArguments));
        args[i]->id = i;
        args[i]->process_id = param->id;
        args[i]->interval_start = subinterval_start;
        args[i]->interval_end = subinterval_end;

        // Create thread and check error
        if(pthread_create(&ids[i], NULL, thread_function, args[i])) {
            free(args);
            printf("Error happened while creating thread\n");
            return;
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < param->num_children; i++)
    {
        // Thread will allocate memory for this
        void *primes;

        pthread_join(ids[i], &primes);

        // Write returned array to shared memory
        int *ptr_primes = (int*)primes;
        while(*ptr_primes != -1) {
            *ptr_shm = *ptr_primes;
            ptr_primes++;
            ptr_shm++;
        }

        // Free memory
        free(primes);
        free(args[i]);
    }
    

    // Add -1 to shared memory indicating end
    *ptr_shm = -1;
    
    printf("Slave %d: Done\n",param->id);
}




void* thread_function(void *param)
{
    // From void to struct
    ThreadArguments *args = (ThreadArguments*) param;

    printf("Thread %d.%d: searching in %d-%d\n", args->process_id, args->id, args->interval_start, args->interval_end);

    // Create int arrray
    int *return_value = malloc(sizeof(int) * (args->interval_end - args->interval_start));

    // Add prime numbers to array
    int *ptr = return_value;
    for (int i = args->interval_start; i < args->interval_end; i++)
    {
        if(isPrime(i)) {
            *ptr = i;
            ptr++;
        }
    }

    // Add -1 to array indicating end
    *ptr = -1;

    return (void *) return_value;
}




bool isPrime(int number)
{
    // https://www.knowprogram.com/c-programming/c-prime-number-using-function/
    int count = 0;

    for(int i=2; i<=number/2; i++)
    {
        if(number%i == 0)
        {
            count=1;
            break;
        }
    }

    if(number == 1) count = 1;

    if(count > 0)
        return false;
    else
        return true;
}