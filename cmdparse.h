#ifndef CMDPARSE_H
#define CMDPARSE_H

// Struct in which you can store all the different elements included in a command
// You would string tokenize an input string and shove each substring into the args array,
// incrementing numArgs as u do so

struct Command {
    int numArgs;
    char** args;
};

/*
* Inits a Command struct
*/
void initCommand(struct Command*);

/*
* Expands "$$" symbols into the process ID
*/
char* expandDollarDollar(char* token);

/*
* Tokenizes command input string into a Command struct
*/
void tokenizeInput(char* input, struct Command*);

/*
* Function to remove redirection arguments when passing a command into exec()
*/
void removeRedirectionArgs(struct Command*, int, int);

/*
* Returns 1 if there was a ">" or "<" symbol detected in the passed command
* Otherwise, returns 0.
*/
int performRedirection(struct Command*);

/*
* Checks if the last arg in a command is '&' to determine whether to run this command in the background
* Also removes the "&" from the args
*/
int runInBackground(struct Command* cmd);


void redirectInputToDevNull();

void redirectOutputToDevNull();

/*
* Frees each token in the Command struct
*/
void freeCommand(struct Command*);

/*
* Debug function that prints out the command line arguments
*/
void debugPrintArgs(const struct Command*);

#endif