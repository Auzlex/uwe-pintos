#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);


// enum for process status
enum process_status
{
	NOT_LOADED, 							// Initial Loaidng State
	LOAD_COMPLETE,							// Load Complete
	LOAD_FAILED      						// Load Failed
};

// define pid_t as an int this will be used as our process id
typedef int pid_t;

// struct for storing child process information
struct process
{
	struct list_elem elem;					// List element for child processes list.
	pid_t pid;                    			// process id
	bool running;                				// to determine if the process is still running
	bool waiting;							// to determine if the process is waiting
	int exit_status;                		// exit status code of the process
	enum process_status process_status; 	// status of the process
	struct semaphore sema_wait;        		// Used to wait for a process to exit before returning it's exit status.
	struct semaphore sema_initialization;   // Used to wait on init for the file being executed to load (or fail to load).
};

/* File descriptor struct */
struct file_desc 
{
  struct file * fp;							// reference to the file
  int fd; 									// the file descriptor
  struct list_elem elem; 					// list for list_push_front
};

#endif /* userprog/process.h */
