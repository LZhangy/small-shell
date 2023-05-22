// -----------------
// LOUIS ZHANG'S SMALL SHELL
// -----------------

// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

// SHELL FILES
#include "cmdparse.h"
#include "signals.h"

// GLOBALS --------------------------------------------------------------
const int MAX_FILENAME_LENGTH = 256;
char* HOMEDIR;
int status;
// Global variables to store termination status for background processes
int terminationStatus; 
char terminationMessage[256];
// extern volatile int foreground_flag;


/*
* The exit command exits your shell. It takes no arguments. 
* When this command is run, your shell must kill any other processes or jobs that your shell has started 
* before it terminates itself.
*/
void exitShell() {
    kill(0, SIGTERM);
    exit(0);
}

/*
* The cd command changes the working directory of smallsh.
* By itself - with no arguments - it changes to the directory specified in the HOME environment variable.
* This command can also take one argument: the absolute or relative path of a directory to change to
*/
void cd(char* path) {
    if (path == NULL || strlen(path) <= 1) {
        // change to home directory if no argument provided
        chdir(HOMEDIR);
    }
    else {
        // change to specified directory
        path[strlen(path)] = '\0';  // remove newline character
        if (chdir(path) != 0) {
            printf("Error: could not change directory to %s\n", path);
        }
    }

}

/*
* Status prints out either the exit status or the terminating signal of the last foreground process run by your shell.
* If this command is run before any foreground command is run, then it should simply return the exit status 0.
* The three built-in shell commands do not count as foreground processes for the purposes of this built-in command - i.e., status should ignore built-in commands.
*/
void statusShell(int status) {
    if (WIFEXITED(status)) {
        printf("exit value %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)) {
        printf("terminated by signal %d - %s\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
}



/*
*   MAIN
*/
int main() {
    printf("\nWelcome to Louis Zhang's small shell!\n\n");
    // Init vars
    HOMEDIR = getenv("HOME");
    char* input = NULL;
    char output[2048];
    size_t len = 0;
    ssize_t nread;
    pid_t pid = getpid();

    struct Command cmd;
    initCommand(&cmd);

    status = 0;
    int redirected = 0;
    int background = 0;

    // HANDLE SIGNALS ----------------------------------------------------
    // Register handle_SIGINT as the signal handler
    signal(SIGINT, ignoreSIGINT);

    // MAIN SHELL LOOP ---------------------------------------------------
    while (1) {
        fflush(stdout); // Flush out the output buffers each time you print

        handleSIGCHLD(); // Print termination message of children, if any
        printf("\n");
        // Print current working directory + ":" as prompt
        getcwd(output, sizeof(output));
        printf("\n%s", output);   
        printf(": ");


        signal(SIGTSTP, toggleForeground);  // 8. For SIGTSTP: Toggle foreground-only mode

        // Read in user input
        nread = getline(&input, &len, stdin);
        if (nread == -1) {
            break;
        }

        // First check if the input is a comment and should be ignored
        if (input[0] == '#') {
            continue;
        }

        // Tokenize input string and store in cmd struct
        tokenizeInput(input, &cmd);

        // If no valid args, just reprompt!
        if (cmd.numArgs < 1) {
            continue;
        }

        // 8. Toggling foreground-only
        if (foreground_flag == 0) {
            background = runInBackground(&cmd);
        }
        // Delete & args if foreground-only mode
        if (foreground_flag == 1) {
            // Remember that the & arg has to be at the end to count
            if (strcmp(cmd.args[cmd.numArgs - 1], "&") == 0) {
                cmd.args[cmd.numArgs - 1] = NULL;  // Remove the "&" from the args
                cmd.numArgs--;
            }
        }

        /*
        * Code to execute commands goes here
        * NOTE that the small shell does not fork for any of the 3 built-in commands
        */
        
            // For the "exit" command
            if (strcmp(cmd.args[0], "exit") == 0) {
                freeCommand(&cmd);
                exitShell();
            }

            // For the "cd" command
            if (strcmp(cmd.args[0], "cd") == 0) {
                cd(cmd.args[1]);    // only takes 1 arg
                // freeCommand(&cmd);
                continue;
            }

            // For the "status" command
            if (strcmp(cmd.args[0], "status") == 0) {
                statusShell(status);
                //freeCommand(&cmd);
                continue;
            }
        

        /* ---------------------------------------------
        * ELSE - Fork and exec into the specified command
        */  
        // FIRST, spawn the child
        pid_t spawnPid = -5;
        status = -5;
        spawnPid = fork();
        
          
        if (spawnPid == -1) // If error while creating fork:
        {
            perror("Hull Breach! Error forking child\n");
            status = 1;
            exit(1);
        } 
        else if (spawnPid == 0) // CHILD PROCESS-----------------------------------------------------
        {
        // SETUP SIGNAL HANDLER LOGIC FOR CHILDREN
            // All children must ignore SIGTSTP
            signal(SIGTSTP, ignoreSIGTSTP);

            // If background process, ignore SIGINT
            if (background == 1) {
                signal(SIGINT, ignoreSIGINT);
            }
            // A foreground child process must terminate itself when it receives SIGINT
            else {
                handleSIGINT(exitSIGINT);
            }

        // REDIRECTION -----------------------------------------------------------------
            // AFTER THE FORK, redirect output if user entered '<' or '>' as valid args
            redirected = performRedirection(&cmd);
            
            // If this this is a background process:
            if (background == 1) {
                // If the user doesn't redirect the standard input for a background command, 
                // then standard input should be redirected to /dev/null
                if (redirected == 01) { // 01 = redirecting output only
                    redirectInputToDevNull();
                }
                // If the user doesn't redirect the standard output for a background command, 
                // then standard output should be redirected to /dev/null
                if (redirected == 10) { // 10 = redirecting input only
                    redirectOutputToDevNull();
                }
                if (redirected == 00) {
                    redirectInputToDevNull();
                    redirectOutputToDevNull();
                }
                // Otherwise, redirected must be 11, so both input & output are already user redirected
            }

            // IF there's any redirection:
            if (redirected != 0) {
                
                // Passes only the first argument to bash because I can't get removing args to work
                execlp(cmd.args[0], cmd.args[0], NULL); 
                perror("execlp error");     // only returns if an error occurred
                status = 1;
                exit(1);
            }
            
            else {
                execvp(cmd.args[0], cmd.args); // Execute the command with all arguments
                perror("execvp error");     // only returns if an error occurred
                status = 1;
                exit(1);
            }
            
            
        }
        else {      // PARENT PROCESS
            if (background == 1) {
                printf("Background process started with PID %d\n", spawnPid);  
            }
            else {
                // Wait for the child process to complete
                waitpid(spawnPid, &status, 0);

                // If a child foreground process is killed by a signal, 
                // the parent must immediately print out the number of the signal that killed its 
                // foreground child process before prompting the user for the next command.
                if (WIFSIGNALED(status)) {
                    char message[100];
                    int length = snprintf(message, sizeof(message), "terminated by signal %d\n", WTERMSIG(status));
                    write(STDOUT_FILENO, message, length);
                }
            }

        }
        
        
        freeCommand(&cmd);
    }
    free(input);
    
    return 0;
}