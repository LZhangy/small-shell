#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "cmdparse.h"

void initCommand(struct Command* cmd) {
    cmd->args = NULL;
    cmd->numArgs = 0;
}

char* expandDollarDollar(char* token) {
    char* newToken = NULL;
    char* pidStr = NULL;

    char* dollarDollar = strstr(token, "$$");
    if (dollarDollar != NULL) {
        int pid = getpid();
        pidStr = malloc(16 * sizeof(char));
        sprintf(pidStr, "%d", pid);

        size_t newTokenSize = strlen(token) + strlen(pidStr) - 1 + 1;
        newToken = malloc(newTokenSize);
        char* pos = strstr(token, "$$");
        int prefixLen = (int)(pos - token);
        strncpy(newToken, token, prefixLen);
        newToken[prefixLen] = '\0';  // Add null terminator
        strcat(newToken, pidStr);
        strcat(newToken, pos + 2);
    }
    else {
        newToken = token;
    }

    free(pidStr);
    return newToken;
}

void tokenizeInput(char* input, struct Command* cmd) {
    char* saveptr;
    const char delim[] = " \t\r\n";

    initCommand(cmd);       // reset cmd struct

    char* token = strtok_r(input, delim, &saveptr);
    char* newToken;
    while (token != NULL) {
        char* expandedToken = expandDollarDollar(token);

        // Resize the array of arg strings
        cmd->args = realloc(cmd->args, (cmd->numArgs + 2) * sizeof(char*));

        // Allocate memory for the new token
        char* newToken = malloc((strlen(expandedToken) + 1) * sizeof(char));
        strcpy(newToken, expandedToken);

        // Assign the new token to the args array
        cmd->args[cmd->numArgs] = newToken;
        cmd->args[cmd->numArgs + 1] = NULL; // Set the next element to NULL
        cmd->numArgs++;

        token = strtok_r(NULL, delim, &saveptr);
    }
}

void removeRedirectionArgs(struct Command* cmd, int startIndex, int removeCount) {
    for (int i = startIndex; i < cmd->numArgs - removeCount; i++) {
        cmd->args[i] = cmd->args[i + removeCount];
    }
    cmd->numArgs -= removeCount;
}

int performRedirection(struct Command* cmd) {
    // Since I didn't want to bother separating this function, I'm using this int to return a combination of states
    // 00 = No redirection at all
    // 10 = Input Redirection only
    // 01 = Output redirection jonly
    // 11 = Input & Output redirection both
    int redirected = 00;

    for (int i = 0; i < cmd->numArgs; i++) {
        // Output redirection: >
        if (strcmp(cmd->args[i], ">") == 0) {

            redirected += 01;

            // Check if there is a next argument
            if (i + 1 >= cmd->numArgs) {
                fprintf(stderr, "Missing filename after '>'\n");
                exit(EXIT_FAILURE);
            }

            // Open the output file
            int fd = open(cmd->args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd == -1) {
                perror("target open");
                exit(EXIT_FAILURE);
            }

            // Redirect stdout to the output file
            int result = dup2(fd, 1);
            if (result == -1) { perror("target dup2"); exit(2); } // taken from slide 3.4 of CS 344, Brewster

            // Close the original file descriptor
            close(fd);

            // Remove the redirection arguments from the command? 
            // removeRedirectionArgs(cmd, i, 2);

            // Skip the redirection arguments
            i++;
            continue;
        }

        // Input redirection: <
        if (strcmp(cmd->args[i], "<") == 0) {

            redirected += 10;

            // Check if there is a next argument
            if (i + 1 >= cmd->numArgs) {
                fprintf(stderr, "Missing filename after '<'\n");
                exit(EXIT_FAILURE);
            }

            // Open the input file
            int fd = open(cmd->args[i + 1], O_RDONLY);
            if (fd == -1) {
                perror("source open");
                exit(EXIT_FAILURE);
            }

            // Redirect stdin to the input file
            int result = dup2(fd, 0);
            if (result == -1) { perror("source dup2"); exit(2); } // taken from slide 3.4 of CS 344, Brewster

            // Close the original file descriptor
            close(fd);

            // Remove the redirection arguments from the command? 
            // removeRedirectionArgs(cmd, i, 2);

            // Skip the redirection arguments
            i++;
            continue;
        }
    }

    return redirected;
}

int runInBackground(struct Command* cmd) {
    int background = 0;
    if (cmd->numArgs > 0) {
        if (strcmp(cmd->args[cmd->numArgs - 1], "&") == 0) {
            background = 1;
            cmd->args[cmd->numArgs - 1] = NULL;  // Remove the "&" from the args
            cmd->numArgs--;
        }
    }

    return background;
}

void redirectInputToDevNull() {
    int devNull = open("/dev/null", O_RDONLY);
    if (devNull == -1) {
        perror("Error opening /dev/null");
        exit(1);
    }

    // Duplicate the file descriptor for stdin
    int dupResult = dup2(devNull, 0);
    if (dupResult == -1) {
        perror("Error redirecting input to /dev/null");
        exit(1);
    }

    close(devNull);
}

void redirectOutputToDevNull() {
    int devNull = open("/dev/null", O_WRONLY);
    if (devNull == -1) {
        perror("Error opening /dev/null");
        exit(1);
    }

    // Duplicate the file descriptor for stdout
    int dupResult = dup2(devNull, 1);
    if (dupResult == -1) {
        perror("Error redirecting output to /dev/null");
        exit(1);
    }

    close(devNull);
}

void freeCommand(struct Command* cmd) {
    for (int i = 0; i < cmd->numArgs; i++) {
        free(cmd->args[i]);
    }
    free(cmd->args);
    initCommand(cmd);
}

void debugPrintArgs(const struct Command* cmd) {
    printf("Number of arguments: %d\n", cmd->numArgs);
    printf("Arguments:\n");
    for (int i = 0; i < cmd->numArgs; i++) {
        printf("%d: %s\n", i, cmd->args[i]);
    }
}