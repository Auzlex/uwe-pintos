#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "threads/synch.h"

#include "filesys/filesys.h"

// reference to esp;
static uint32_t *esp;

static void syscall_handler (struct intr_frame *);

// create a new lock struct for locking threads/synchronization
struct lock syscall_lock;

/* 

	Memory Validation Functions

*/

// called when we want to check if a virtual address is valid
bool IsValidVAddress(void * vaddress)
{
	// get the current thread
	struct thread *cur = thread_current();
	
	// invoke the is_user_vaddr in vaddr.h which checks if address is less than PHYS_BASE
	// invoke pagedir_get_page to return the physical address of the virtual address
	// e.g the current threads pagedir. Check for null pointer if the UADDR is unmapped
	return (is_user_vaddr(vaddress) && pagedir_get_page(cur->pagedir,vaddress));
}

bool AutoValidatePointer(uint32_t *esp, int a)
{

	// validate the V address by assigned value or via its location in memory
	bool validation = (!IsValidVAddress(esp+a) || !IsValidVAddress(*(esp+a)));
	
	if(validation)
	{
		// if we have a faulty pointer terminate this thread.
		printf("AutoValidatePointer (a = %d) -> validation returned true terminating thread! pointer returned = %d\n", a, *(esp+a));
		
		// status code -1
		exit(-1);	
	}
	
	return validation;
}

/* 

	Syscall Implemented Functions

*/

// called on syscall initialize
void syscall_init (void) 
{
	// init register
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
	
	// initialize lock or syscall_lock
	lock_init(&syscall_lock);
}

// halt should shutdown and power off the pintos os
void halt(void)
{
	shutdown_power_off();
}

// Terminates the current user program while returning status to the kernal.
void exit(int status)
{
	// debug log
	printf( "syscall.c -> exit(int status = %d) -> invoked!\n", status );
	
	// get the current thread
	struct thread* current = thread_current();

	// set the status of the thread to exit
	current->exit_code = status;

	// terminate thread
	thread_exit();
}

// start another process
/*tid_t exec(const char* cmd_line)
{
	// debug log
	printf( "syscall.c -> exec(const char* cmd_Line = %s) -> invoked!\n", cmd_line );

	// reference to child_process 
	struct thread *child_process;
	tid_t pid;
	
	// begin synchronization :: 
	lock_acquire(&syscall_lock);
	
	// begin a new process and store its processID into pid_t
	pid = process_execute(cmd_line);
	
	// get the child process
	child_process = get_child_process(pid);
	
	// end synchronization :: 
	lock_release(&syscall_lock);
	
	// check if the child process successfully loaded if so return the process id
	// else return -1 :: -1 implies the process id is not valid
	// "Must return pid -1, which otherwise should not be a valid pid, if the program cannot load or run for any reason"
	if(child_process->is_child_loaded==true)
	{
		return pid;
	}
	else
	{
		return -1;
	}
}*/

bool create(char* filename, unsigned size)
{
	bool success; // boolean used to determine if filesys_create was successful
	
	// begin synchronization :: 
	lock_acquire (&syscall_lock);
	
	printf( "create( filename = %s, size = %d ) -> invoking filesys_create\n", filename,  size );
	
	// create file
	success = filesys_create(filename, size);  
	
	printf( "was filesys_create successful? success = %s\n", success ? "true" : "false" );
	
	// end synchronization :: 
	lock_release (&syscall_lock);
	
	return success;
}

// called when a sys call is not implemented
void throw_not_implemented_message_and_terminate_thread(int syscallnum)
{
	// debug log what sys call is not implemented
	printf( "SYS_CALL (%d) not implemented\n", syscallnum);
	
	printf("\n\n");
	printf("SYS_HALT = %d\n", SYS_HALT);
	printf("SYS_EXIT = %d\n", SYS_EXIT);
	printf("SYS_EXEC = %d\n", SYS_EXEC);
	printf("\n\n");
	
	// thread exit
	thread_exit();
}
	
static void
syscall_handler (struct intr_frame *f UNUSED)
{
	// get the syscall number int or enum stored in the stack pointer
	//uint32_t *syscall_number = f->esp;
	int syscall_number = *(int*)f->esp;
	
	// main reference to stack pointer with no offset
	esp = f->esp;
	
	// debug log
	//printf( "syscall_handler -> invoked syscall_number = %d\n", syscall_number );
	
	// handle the enum via switch statement
	switch(syscall_number)
	{
		case SYS_HALT: // invoked on system halt
			
			// debug log
			printf( "SYS_HALT called!\n" );
			
			// invoke the local procedure in this syscall.c named halt
			halt();
			
			break;
		case SYS_EXIT: // invoked on system exit
			
			// debug log
			printf( "SYS_EXIT called!\n" );
			
			// get the status of a thread from the stack
			int status = *((int*)esp + 1);
			
			// invoke the local exit procedure in this syscall.c with the status int
			exit(status);
			
			break;
		
		case SYS_EXEC:
			
			// debug log
			printf( "SYS_EXEC called!\n" );
			throw_not_implemented_message_and_terminate_thread(syscall_number);
			
			/*// validate pointer
			if(AutoValidatePointer(esp,1))
			{
				// if auto validate pointer returns true
				// halt code here as the thread will be terminated
				// exit(-1); is invoked inside of AutoValidatePointer
				return;
			}
			
			// get the command line pointer from the stack
			char* cmdline = ((char*) *(esp + 1));
			
			// execute program and get the processID
			tid_t processID = exec(cmdline);
			
			// set the EAX register :: System calls that return a value can do so
			// by modifying the eax member of struct intr_frame
			f->eax = processID;*/
			
			break;
		case SYS_WAIT:
			
			// Implement 
			
			break;
			
		
		case SYS_CREATE:
			
			// debug log
			printf( "SYS_CREATE called!\n" );
			
			/*// validate memory
			if(AutoValidatePointer(esp,4) || AutoValidatePointer(esp,8))
			{
				// if auto validate pointer returns true
				// halt code here as the thread will be terminated
				// exit(-1); is invoked inside of AutoValidatePointer
				return;
			}
			
			printf( "Pointers validated proceeding...\n" );*/
			
			// get the file name and file size from the stack
			char* filename = ((char*) *(esp + 4));
			unsigned filesize = ((unsigned *) *(esp + 8));
	
			// set the EAX register :: System calls that return a value can do so
			// by modifying the eax member of struct intr_frame
			f->eax = create(filename, filesize);
			
			break;
		case SYS_REMOVE:
			
			// Implement 
			
			break;
		case SYS_OPEN:
			
			// Implement 
			
			break;
		case SYS_FILESIZE:
			
			// Implement 
			
			break;
		case SYS_READ:
			
			// Implement 
			
			break;
		case SYS_WRITE:; // invoked on system write
			
			// https://www.quora.com/What-is-the-file-descriptor-What-are-STDIN_FILENO-STDOUT_FILENO-and-STDERR_FILENO
			// get the file descriptor from the stack
			int fd = *(int *)(esp + 4);
			
			// create a buffer
			void *buffer = *(char**)(esp + 8);
			
			// size of buffer
			unsigned size = *(unsigned *)(esp + 12);

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
		case SYS_SEEK:
			
			// Implement 
			
			break;
		case SYS_TELL:
			
			// Implement 
			
			break;
		case SYS_CLOSE:
			
			// Implement 
			
			break;
			
		/* 
			PROJECT 3 and optionally PROJECT 4 IMPLEMENTATION
		*/
			
		/*case SYS_MMAP:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;
			
		case SYS_MUNMAP:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;*/
			
		/* 
			PROJECT 4 IMPLEMENTATION
		*/
			
		/*case SYS_CHDIR:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;
			
		case SYS_MKDIR:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;
			
		case SYS_READDIR:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;
			
		case SYS_ISDIR:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;
			
		case SYS_INUMBER:
			
			// Implement 
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(*syscall_number);
			
			break;*/
			
		default:
			
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(syscall_number);
			break;
	}
}
