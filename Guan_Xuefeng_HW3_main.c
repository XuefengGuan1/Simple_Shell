#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

// Define BUFFER_SIZE to 163 as required
#define BUFFER_SIZE 163

// Make a reasonable max number of arguments can be entered
#define MAX_ARG_LENGTH 10

/*Define Delimiter for strtok as a space and |, as if we want to change this later
we could easily change here. Don't need to find in the function */
#define DELIM_SPACE " "
#define DELIM_PIPE "|"

void execution(char **);
void runPipe(char **);

int main(int argc, char *argv[])
{
    // Create a prefix "> " if nothing entered from user
    char *prompt = "> ";
    if (argc > 1)
    {
        prompt = argv[1];
    }

    // Create a buffer to store the inputs from user
    char buffer[BUFFER_SIZE];

    /*Loop over and over until later user inputs exit as their command,
    otherwise the loop will not stop*/
    while (1)
    {
        // Prompt user to enter commands
        fprintf(stdout, "%s ", prompt);

        /*
        1. Store user input into a buffer with max byte length of 163
        2. Ensure that when EOF is reached, which would return NULL,
        the program would break and exit
         */
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        /*Because how fgets work, we replace the new line character with
        a null terminator */
        if (buffer[strlen(buffer) - 1] == '\n')
        {
            buffer[strlen(buffer) - 1] = '\0';
        }

        /*If user didn't input anything and just pressed enter,
        then the string would be "\n\0", but then I replaced \n with \0.
        Thus "\n\0" will become "\0" with a length of 0. When user
        enters nothing, prompt the user and fetch a new line
        */
        if (strlen(buffer) == 0)
        {
            fprintf(stderr, "Enter something please\n");
            continue;
        }

        // Tokenize the strings in buffer with a 2D array

        // Create a 2D char array to store multiple strings
        char *args[MAX_ARG_LENGTH];
        int index = 0;

        /* Run strtok with buffer as the first argument once, and then
        in while loop use NULL as the first argument to make sure strtok
        will keep finding the next token in buffer*/

        // Check if the following buffer contains "|" or not
        args[index] = strtok(buffer, DELIM_PIPE);

        // Check if user inputed exit, then exit the program
        if (strcmp(args[index], "exit") == 0)
        {
            break;
        }

        // Will not stop until NULL pointer hit
        while (args[index] != NULL)
        {
            index++;
            /*Truncate the arguments after the 9th argument,
            and also save the 10th one for the NULL pointer */
            if (index >= MAX_ARG_LENGTH - 1)
            {
                break;
            }
            args[index] = strtok(NULL, DELIM_PIPE);
        }

        // Insert a NULL pointer at the end of the 2D array
        args[index] = NULL;

        /*
         If index > 1, than that means "|"" is detected in buffer,
         Thus the command entered is a pipe command, so do runPipe();
        */
        if (index > 1)
        {
            runPipe(args);
        }
        else
        {
            /*
                If index = 0, then it means that no "|" detected in buffer,
                Thus the command entered is not a pipe command.
            */
            // Don't run pipe

            // Reset the index, and remove all excessive spaces in buffer
            index = 0;
            args[index] = strtok(buffer, DELIM_SPACE);
            while (args[index] != NULL)
            {
                index++;
                /*Truncate the arguments after the 9th argument,
                and also save the 10th one for the NULL pointer */
                if (index >= MAX_ARG_LENGTH - 1)
                {
                    break;
                }
                args[index] = strtok(NULL, DELIM_SPACE);
            }
            // NULL terminate the args;
            args[index] = NULL;
            execution(args);
        }
    }
    return 0;
}

void execution(char **args)
{
    // Set up for fork
    pid_t pid;
    int childExitStatus;

    // Start the fork process
    pid = fork();

    if (pid < 0)
    {
        // If fork() returns a value smaller than 0, then fork failed.
        // Exit the program with error code
        fprintf(stderr, "fork creation failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // If pid = 0, then child process
        // execvp should loop through the 2D array, and run each argument
        execvp(args[0], args);

        // Make sure to exit the program with error code when execvp fails
        // If evecvp succeed, then anything below this shouldn't run
        fprintf(stderr, "execvp failed\n");
        exit(1);
    }
    else
    {
        // If pid > 0, then parent Process
        // Gets the child process exit code then print it out
        // Pass in childExitStatus's address
        wait(&childExitStatus);
        fprintf(stdout, "Child %d, exited with %d\n", pid, WEXITSTATUS(childExitStatus));
    }
}

void runPipe(char **args)
{
    // First remove all the spaces for each string
    // Create a 3D array to store the commands
    /* A command like {"   cat commands.txt "},{"wc "}, {"   wc   "}
        after this process should be come{"cat", "commands.txt"},{"wc"},{"wc"}
    */
    char *commands[MAX_ARG_LENGTH][MAX_ARG_LENGTH];

    // i is for commands[i][][] and args[i], they share this value
    int i = 0;
    // j is for commands[][j][]
    int j = 0;

    // Remove excessive spaces in the 2D array, similar process like before
    for (i = 0; args[i] != NULL; i++)
    {
        commands[i][j] = strtok(args[i], DELIM_SPACE);
        while (commands[i][j] != NULL)
        {
            j++;
            commands[i][j] = strtok(NULL, DELIM_SPACE);
        }
        commands[i][j] = NULL;
        j = 0;
    }
    commands[i][0] = NULL;

    /*
      There are i commands, and i - 1 pipes, because a pipe connects two
      commands together.
    */
    int pipeNeeded = i - 1;
    int numOfCommands = i;

    // Create an array to store all the file descriptors for pipes
    // Each pipe has 2 file descriptors, one for in and one for out
    int pipefd[2 * pipeNeeded];

    // Create pipes by using a for loop
    for (int i = 0; i < pipeNeeded; i++)
    {
        /*
          Use pointer manipulation here. pipefd is a pointer points to
          pipefd[0], pipefd + 1 equals to pipefd[1], pipefd + 2
          equals to pipefd[2]
        */
        if (pipe(pipefd + (2 * i)) == -1)
        {
            // If pipe creation fails, then exit the program
            fprintf(stderr, "Pipe creation failed");
            exit(1);
        }
    }

    // Variables for fork process.
    pid_t pid;
    int status;
    pid_t pidIds[numOfCommands];

    for (i = 0; i < numOfCommands; i++)
    {
        // Create a fork in each iteration
        pid = fork();

        if (pid < 0)
        {
            // Fork failed, and return error
            fprintf(stderr, "fork creation failed");
            exit(1);
        }
        else if (pid == 0)
        {
            // Child process
            /*
             If it's not the first command, set the input to the
             previous pipe's read end
            */
            if (i != 0)
            {
                if (dup2(pipefd[(i - 1) * 2], 0) == -1)
                {
                    // Check dup2 status
                    fprintf(stderr, "dup2 creation failed for the head\n");
                    exit(1);
                }
            }

            /*
             If not the last command, then set the output to
             the tail of the current pipe
            */
            if (i != numOfCommands - 1)
            {
                if (dup2(pipefd[i * 2 + 1], 1) == -1)
                {
                    // Check dup2 status
                    fprintf(stderr, "dup2 creation failed for the tail\n");
                    exit(1);
                }
            }

            // Close all pipe file descriptors in the child process
            for (int j = 0; j < 2 * pipeNeeded; j++)
            {
                close(pipefd[j]);
            }

            // Execute the command
            execvp(commands[i][0], commands[i]);

            // If execvp success, then this line shouldn't read
            // If fails, then return error code
            fprintf(stderr, "execvp failed");
            exit(1);
        }
        else
        {
            // Parent process
            // Store all the pid values of each children
            pidIds[i] = pid;
        }
    }

    // closes all pipe file descriptors to ensure no leaks
    for (int i = 0; i < 2 * pipeNeeded; i++)
    {
        close(pipefd[i]);
    }

    // Wait for child process to finish
    for (int i = 0; i < numOfCommands; i++)
    {
        wait(&status);
        // Print the Child ID
    }
    for (int i = 0; i < numOfCommands; i++)
    {
        fprintf(stdout, "Childs %d", pidIds[i]);
        if (i != numOfCommands - 1)
        {
            fprintf(stdout, "\n");
        }
    }

    // Print the ending status
    fprintf(stdout, ", exited with %d\n", WEXITSTATUS(status));
}
