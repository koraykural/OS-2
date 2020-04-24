#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include<sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

void process_function(int interval_start, int interval_end, int *pipe_endings);
bool isPrime(int number);

int main(int argc, char* argv[])
{
    // Check argument count
    if(argc != 5) {
        printf("4 arguments requiered, returning\n");
        return EXIT_FAILURE;
    }

    printf("Master: Started.\n");

    // Get arguments
    char *ptr;
    int interval_start = strtol(argv[1], &ptr, 10);
    int interval_end = strtol(argv[2], &ptr, 10);
    int num_processes = strtol(argv[3], &ptr, 10);
    //int num_threads = strtol(argv[4], &ptr, 10);

    // Create pipe for each child process
    int **fds = NULL;
    fds = malloc(sizeof(int) * num_processes);
    for (int i = 0; i < num_processes; i++)
    {
        fds[i] = malloc(sizeof(int) * 2);
        pipe(fds[i]);
    }

    // Calculate interval lenght of processes
    int interval_steps = (interval_end - interval_start) / num_processes;

    // Store pids of child processes
    int *child_processes = malloc(sizeof(int) * num_processes);
    
    // Create child processes
    int pid = 1;
    for(int i = 0; i < num_processes; i++)
    {
        pid = fork();

        // Fork error
        if(pid < 0) {
            printf("Fork failed, returning\n");
            return EXIT_FAILURE;
        }

        // Child process
        else if(pid == 0) {
            int subinterval_start = interval_start + (interval_steps * i) + i;
            int subinterval_end = interval_start + (interval_steps * (i+1)) + i;
            printf("Slave %d Started. Interval %d-%d\n",i,subinterval_start,subinterval_end);
            process_function(subinterval_start, subinterval_end, fds[i]);
            printf("Slave %d: Done\n",i);
            exit(0);
            break;
        }

        // Main process
        else {
            child_processes[i] = pid;
        }
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0) ;

    // Print results
    printf("Master: Done. Prime numbers are: ");
    for (int i = 0; i < num_processes; i++)
    {
        // Close writing end of pipe
        close(fds[i][1]);

        // Read until the end
        int read_value;
        while (read(fds[i][0], &read_value, sizeof(int)) > 0) {
            printf("%d, ", read_value);
        }

        // Close reading end of pipe
        close(fds[i][0]);
    }
    printf("\n");

    // Free the memory and terminate program
    free(child_processes);
    for (int i = 0; i < num_processes; i++)
    {
        free(fds[i]);
    }
    return EXIT_SUCCESS;
}

void process_function(int interval_start, int interval_end, int *pipe_endings)
{
    // Close the reading end of the pipe
    close(pipe_endings[0]);
    for (int i = interval_start; i <= interval_end; i++)
    {
        if(isPrime(i)) {
            write(pipe_endings[1], &i, sizeof(int));
        }
    }
    close(pipe_endings[1]);
    return;
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