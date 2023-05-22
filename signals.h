#ifndef SIGNALS_H
#define SIGNALS_H

typedef void (*SignalHandler)(int);

volatile int foreground_flag;     // for toggling foreground-only mode

// ignores SIGINT
void ignoreSIGINT(int);

// terminates when receiving SIGINT
void exitSIGINT(int);

// reprompt when SIGINT 
void repromptSIGINT(int);

// Function for setting SIGINT handler
void handleSIGINT(SignalHandler);


// ignores SIGTSTP
void ignoreSIGTSTP(int);

// If SIGTSTP is received, call this to toggle foreground-only mode
void toggleForeground(int signum);


/*
* When a background process terminates, a message showing the process id, 
* termination method (i.e. exited normally or signaled), and exit status will be printed. 
*/
void handleSIGCHLD();


#endif