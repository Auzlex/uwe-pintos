#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "process.h"

#include "threads/vaddr.h"
#include "threads/synch.h"

#include "filesys/filesys.h"

void syscall_init (void);
bool IsValidVAddress(void * vaddress);

void halt(void);
void exit(int status);
tid_t exec(const char* cmd_line);
int wait(tid_t id);

bool create(char* filename, unsigned size);
int open(const char* filename);

void throw_not_implemented_message_and_terminate_thread(int syscallnum);
static void syscall_handler (struct intr_frame *);

// create a new lock struct for locking threads/synchronization
struct lock syscall_lock;

// decides if we want to show debug logs
bool debug = true;

// called on syscall initialize
void syscall_init (void) 
{
	// initialize lock or syscall_lock
	lock_init(&syscall_lock);

	// init register
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");	
}

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

/* 
	Syscall Implemented Functions
*/

// halt should shutdown and power off the pintos os
void halt(void)
{
	shutdown_power_off();
}


// Terminates the current user program while returning status to the kernal.
void exit(int status)
{
	// debug log
    if(debug)
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
    if(debug)
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

tid_t exec(const char * cmd_line)
{
	// debug log
    if(debug)
	    printf( "syscall.c -> exec(const char* cmd_Line = %s) -> invoked!\n", cmd_line );

	// reference to child_process 
	//struct thread *child_process;
	tid_t pid;
	
	// begin synchronization :: 
	//lock_acquire(&syscall_lock);
	
	// null check that input
	if(cmd_line == NULL)
	{
		// debug log
		if(debug)
			printf( "Error: For some reason cmd_line returned null!\n" );

		return -1;
	}

	// begin a new process and store its processID into pid_t
	pid = process_execute(cmd_line);
	
	// get the child process
	//child_process = get_child_process(pid);
	
	// end synchronization :: 
	//lock_release(&syscall_lock);
	
	// check if the child process successfully loaded if so return the process id
	// else return -1 :: -1 implies the process id is not valid
	// "Must return pid -1, which otherwise should not be a valid pid, if the program cannot load or run for any reason"
	//if(child_process->is_child_loaded==true)
	//{
	return pid;
	//}
	//else
	//{
		//return -1;
	//}
}

// Waits for a child process pidand retrieves the child's exit status. 
int wait(tid_t id)
{
	// invoke process_wait
  	tid_t tid = process_wait(id);
  	return tid;
}

// called when we want to create a file
bool create(char* filename, unsigned size)
{
	bool success; // boolean used to determine if filesys_create was successful
	
	// begin synchronization :: 
	lock_acquire (&syscall_lock);
	
    if(debug)
	    printf( "create( filename = %s, size = %d ) -> invoking filesys_create\n", filename,  size );
	
	// create file
	success = filesys_create(filename, size);  
	
    if(debug)
	    printf( "was filesys_create successful? success = %s\n", success ? "true" : "false" );
	
	// end synchronization :: 
	lock_release (&syscall_lock);
	
	return success;
}

// Opens a file of a given name. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened. 
int open(const char* filename)
{

	// begin synchronization :: because a file can be opened by a single process or different processes which implies child processes
	lock_acquire(&syscall_lock);

	// open file with our desired name
	struct file* fp = filesys_open (filename);

	// end synchronization :: 
	lock_release(&syscall_lock);
	
	// if the file pointer is null then return -1 as bad file name or we cannot open it???
	if (fp == NULL) 
		return -1;
	
	/* 
    
        FILE DESCRIPTOR STRUCT REFERENCE in process.h

        struct file_desc {
			struct file * fp;		// reference to the file
			int fd; 				// the file descriptor
			struct list_elem elem; 	// list for list_push_front
		};

    */

	// we will use a file descriptor struct
	struct file_desc * fd_elem = malloc(sizeof(struct file_desc));

	// if we open the same file at a different time we make sure the file descriptor is different
	fd_elem->fd = ++thread_current()->fd_count;

	// store the file into the fd_elem in the file descriptor struct
	fd_elem->fp = fp;

	// removes elements first though last exclusive from their current list and inserts them just before
	// which be either the interior element or the tail
	//list_push_front( &thread_current()->file_list, &fd_elem->elem );
	
	if(debug)
		printf("returned file desciptor = %d\n",fd_elem->fd);

	// return the file descriptor
	return fd_elem->fd;
}

// called when a sys call is not implemented
void throw_not_implemented_message_and_terminate_thread(int syscallnum)
{
	// debug log what sys call is not implemented
    if(debug)
	    printf( "SYS_CALL (%d) not implemented\n", syscallnum);

	// thread exit
	thread_exit();
}
	
static void
syscall_handler (struct intr_frame *f UNUSED)
{
	// get the syscall number int or enum stored in the stack pointer
	//uint32_t *syscall_number = f->esp;
	int syscall_number = *(int*)f->esp;
	
	// debug log
    if(debug)
	    printf( "syscall_handler -> invoked syscall_number = %d\n", syscall_number );
	
	// handle the enum via switch statement
	switch(syscall_number)
	{
		case SYS_HALT: // invoked on system halt
			
			// debug log
            if(debug)
			    printf( "SYS_HALT called!\n" );
			
			// invoke the local procedure in this syscall.c named halt
			halt();
			
			break;
		case SYS_EXIT: // invoked on system exit
			
			// debug log
            if(debug)
			    printf( "SYS_EXIT called!\n" );
			
			// get the status of a thread from the stack
			int status = *((int*)f->esp + 1);
			
			// invoke the local exit procedure in this syscall.c with the status int
			exit(status);
			
			break;
		
		case SYS_EXEC:
			
			// debug log
			if(debug)
				printf( "SYS_EXEC called!\n" );

			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 1))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
			// get the command line pointer from the stack
			const char* cmdline = ((char*) *((int*)f->esp + 1));
			
            // execute program and get the processID
			// set the EAX register :: System calls that return a value can do so
			// by modifying the eax member of struct intr_frame
			f->eax = exec(cmdline);
			
			break;
		case SYS_WAIT:

			// debug log
			if(debug)
				printf( "SYS_WAIT called!\n" );
			
			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 1))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}

			// invoke syscall wait
			// set the EAX register :: System calls that return a value can do so
			f->eax = wait(*((int*)f->esp + 1));
			
			break;
		case SYS_CREATE:
			
			// debug log
            if(debug)
			    printf( "SYS_CREATE called!\n" );
			
			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 4) || !IsValidVAddress(f->esp + 5))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
            if(debug) // debug log
			    printf( "Pointers validated proceeding...\n" );
			
			// get the file name and file size from the stack
			char* filename = ((char*) *((int*)f->esp + 4));
			unsigned filesize = ((unsigned *) *((int*)f->esp + 5));
	
			// set the EAX register :: System calls that return a value can do so
			// by modifying the eax member of struct intr_frame
			f->eax = create(filename, filesize);
			
			break;
		case SYS_REMOVE:
			
			// Implement 
			
			break;
		case SYS_OPEN:
			
            if(debug) // debug log 
                printf( "SYS_OPEN -> start\n" );

			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 1))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
			// get desired file to open name from the stack
            // we assigned the the stack position to an int pointer and then void return pointer to a char
			char* open_filename = ((char*) *((int*)f->esp + 1));
			
			if(debug)
            	printf( "open_filename = %s\n", open_filename );
			
			// open file name
			// set the EAX register :: System calls that return a value can do so
			// by modifying the eax member of struct intr_frame
			f->eax = open(open_filename);
			
			break;
		case SYS_FILESIZE:
			
			// Implement 
			
			break;
		case SYS_READ:
			
			// Implement 
			
			break;
		case SYS_WRITE:; // invoked on system write
			
			// information reference https://www.quora.com/What-is-the-file-descriptor-What-are-STDIN_FILENO-STDOUT_FILENO-and-STDERR_FILENO
			// get the file descriptor from the stack
			int fd = *(int *)(f->esp + 4);
			
			// create a buffer
			void *buffer = *(char**)(f->esp + 8);
			
			// size of buffer
			unsigned size = *(unsigned *)(f->esp + 12);

			// check if we can actually write out
			if(fd == STDOUT_FILENO)
			{
                // writes n characters to the BUFFER to the console
				putbuf((const char*)buffer, (unsigned )size);
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
			
		default:
			
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(syscall_number);
			break;
	}
}
