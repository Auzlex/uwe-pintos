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


// called on halt
/*void
halt (void) 
{
  //syscall0 (SYS_HALT);
  NOT_REACHED ();
}*/

void halt(void){shutdown_power_off();}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
	// get the syscall number int or enum stored in the stack pointer
	uint32_t *syscall_number = f->esp;
	
	// debug log
	//printf( "syscall_handler -> invoked syscall_number = %d\n", syscall_number );
	
	// handle the enum via switch statement
	switch(*syscall_number)
	{
		case SYS_HALT: // invoked on system halt
			
			// debug log
			printf( "SYS_HALT called!\n" );
			
			// sys halt ivoked
			halt();
			
			break;
		case SYS_EXIT: // invoked on system exit
			
			// debug log
			printf( "SYS_EXIT called!\n" );
			
			// get the status of a thread from the stack
			int status = *((int*)f->esp + 1);
			
			// get the current thread
			struct thread* current = thread_current();
			
			// set the status of the thread to exit
			//current->process_info->exit_status = status;
			current->exit_code = status;
			
			// terminate thread
			thread_exit();
			
			break;
			
		case SYS_WRITE:;
			
			// https://www.quora.com/What-is-the-file-descriptor-What-are-STDIN_FILENO-STDOUT_FILENO-and-STDERR_FILENO
			// get the file descriptor from the stack
			int fd = *(int *)(f->esp + 4);
			
			// create a buffer
			void *buffer = *(char**)(f->esp + 8);
			
			// size of buffer
			unsigned size = *(unsigned *)(f->esp + 12);

			// check if we can actually write out
			if(fd == STDOUT_FILENO)
			{
				//printf("ECHO ->: ");
				// begin writing characters into the console
				putbuf((const char*)buffer, (unsigned )size);
				//printf("\n");
			}
			else
			{
				// print out error that we can't output
				printf("sys_write does not support fd output\n");
			}
			
			break;
			
		default:
			printf( "SYS_CALL (%d) not implemented\n", *syscall_number);
			thread_exit();
			break;
	}
}
