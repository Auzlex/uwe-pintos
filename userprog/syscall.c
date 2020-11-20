#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f UNUSED)
{
  

  /* de-reference stack pointer */

  /* get the system call enum number */
  int syscallnum = *((int*)f->esp);

  int par = *((int*)f->esp+4);

  printf ("syscall_handler -> invoked call! type %d\n", syscallnum); /* system call */


  thread_exit ();
}
