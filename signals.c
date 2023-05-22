#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "signals.h"

typedef void (*SignalHandler)(int);

volatile int foreground_flag = 0;     // for toggling foreground-only mode


void ignoreSIGINT(int signal) {
    //char* message = "Caught SIGINT, sleeping for 00 seconds\n";
    //write(STDOUT_FILENO, message, 39);
}

void repromptSIGINT(int signal) {
    // Clear stdin
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}

    fflush(stdout);
}

void exitSIGINT(int signal) {
    exit(0);
}

void handleSIGINT(SignalHandler sigHandler) {
    struct sigaction sa;
    sa.sa_handler = sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

void ignoreSIGTSTP(int signal) {

}

void toggleForeground(int signal) {
    char message[100];
    if (foreground_flag == 0) { 
        int length = snprintf(message, sizeof(message), "Entering foreground-only mode (& is now ignored)\n");
        write(STDOUT_FILENO, message, length);
        
        foreground_flag = 1;
        return;
    }
    if (foreground_flag == 1) {
        int length = snprintf(message, sizeof(message), "Exiting foreground-only mode\n");
        write(STDOUT_FILENO, message, length);

        foreground_flag = 0;
        return;
    }
    
}


void handleSIGCHLD() {
    int status;
    pid_t childPid;

    while ((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            char message[100];
            int length = snprintf(message, sizeof(message), "Background process with PID %d exited normally with status %d\n", childPid, WEXITSTATUS(status));
            write(STDOUT_FILENO, message, length);
        }
        else if (WIFSIGNALED(status)) {
            char message[100];
            int length = snprintf(message, sizeof(message), "Background process with PID %d terminated by signal %d\n", childPid, WTERMSIG(status));
            write(STDOUT_FILENO, message, length);
        }
    }
}