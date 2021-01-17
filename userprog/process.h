#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

// Child struct to track child processes
struct child 
{
  tid_t id;
  int ret_val;
  int used;
  struct list_elem elem;
};

/* File descriptor */
struct file_desc {
  struct file * fp;// reference to the file
  int fd; // the file descriptor
  struct list_elem elem; // list for list_push_front
};

#endif /* userprog/process.h */
