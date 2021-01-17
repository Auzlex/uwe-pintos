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

// Function to remove a file from the filesystem.dsk
bool remove_file(char* filename)
{
	// Create a boolean to store whether the file remove was successful
	bool success;
	// Begin synchronization
	lock_acquire (&syscall_lock);
	printf("remove(filename: %s) -> invoking filesys_remove\n", filename);
	// Remove the file and save the result to the success variable
	success = filesys_remove(filename);  
	// Print whether this was successful
	printf("success: %s\n", success ? "true" : "false");
	// End synchronization
	lock_release (&syscall_lock);
	// Return the success variable
	return success;
}

// Function to read a designated amount of data from a file
static int read(int fd, void *dataBuf, unsigned readSize)
{
	/* Validation */
	// Copy the inputted dataBuffer into a byte called buffer
	uint8_t buffer = (uint8_t *) dataBuf;

  	// Create an unsigned int to store the data from the file
  	unsigned data = 0;
  	// If the file descriptor is the standard input (0) .aka reading from the keyboard
  	if (fd == STDIN_FILENO) {
  		// Create a byte
      	uint8_t byte;
      	// While the data to be returned is smaller than the maxium amount of data to be returned and the keyboard input isn't 0
      	while (data < readSize && (byte = input_getc()) != 0) {
      		// Set the value of the buffer byte to to the keyboard input and increment the buffer
        	*buffer++ = byte;
        	// Increment the data integer
          	data ++;
        }
        // Return the data integer
    	return (int) data;
    }

    /* Load the file from the file decriptor */
    // Sychronisation - aquire a lock from the current thread so that the file can't be edited while accessing it
  	lock_acquire(&filesys_lock);
  	// Get the file descriptor for the corresponding fd number
  	struct file_desc * file_descriptor = get_file_descriptor(fd);
  	// Get the file from the file_descriptor
  	struct file* file = file_descriptor->fp;
  	// If the file is NULL
  	if (file == NULL) {
  		// Relase the file
    	lock_release(&filesys_lock);
    	// Return -1 representing a failure
    	return -1;
    }

  	// Set the data to the results of a file read
  	data = file_read(file, dataBuf, readSize);

  	// Release the file being read
  	lock_release(&filesys_lock);
  	// Return the data from the file as an integer
	return (int) data;
}

// Get the position of the from the beggining in the open file (file descriptor)
static unsigned tell(int fd) {
    // Synchronisation - lock the file so that it cannot be opened by another process
  	lock_acquire(&filesys_lock);
  	// Get the file descriptor for the corresponding fd number
  	struct file_desc * file_descriptor = get_file_descriptor(fd);
  	// Get the file from the file_descriptor
  	struct file* file = file_descriptor->fp;
  	// If the file is Null
  	if (acFile == NULL) {
  		// Release the file
      	lock_release(&filesys_lock);
      	// Return -1 representing a failure
      	return -1;
    }
  	// Get the actual position in the file
  	unsigned pos = file_tell(acFile);
  	// Release the file
  	lock_release(&filesys_lock);
  	return pos;
}

// Function to check all the files in the current thread and return the file descriptor with the corresponding fd number
struct file_desc * get_file_descriptor(int fd) {
	// Get the current thread
	struct thread * current_thread = thread_current();
	// Create a list element to hold the current file descriptor while traversing the linked list
  	struct list_elem * current_element;
  	// Create a blank file_desc to store a succesful match
  	struct file_desc * return_descriptor = NULL;
  	// Traverse each element in the linked list of file_descriptors in the current thread
  	for (current_element = list_begin(&current_thread->file_list); current_element != list_end(&current_thread->file_list); current_element = list_next(current_element)) {
    	// Get the file_desc structure from the current position in the lisr
    	struct file_desc * file_descriptor = list_entry(e, struct file_desc, elem);
    	// If the current file_descriptors fd number is the same as the fd argument
    	if (file_descriptor->fd == fd) {
    		// Save the file descriptor to a variable
    		return_descriptor = file_descriptor;
    		// Break the loop
    		break;
      	}
  	} 
  	// Return the return_descriptor (null if nothing matched)
  	return return_descriptor;
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
			printf("SYS_REMOVE called\n");
			// Get the file name from the stack
			char* file_name = ((char*) *(f->esp + 4));
			// Remove the file and save the result to
			f->eax = remove_file(file_name);
			break;
		case SYS_OPEN:
			
			// Implement 
			
			break;
		case SYS_FILESIZE:
			
			// Implement 
			
			break;
		case SYS_READ:
			printf("SYS_READ called\n");
			// Get the file descriptor from the stack
			int file_d = *(int *)(f->esp + 4);
			// Get the buffer from the stack
			void *buff = *(char**)(f->esp + 8);
			// Set the size 
			unsigned length = *(unsigned *)(f->esp + 12);
			// Read the file and save it to the eax register
			f->eax = read(file_d, buff, length);
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
			// Get the file descriptor value from the stack
			fd = *(int *)(f->esp + 4);
			// Get the position in the file from the file descriptor number and save it tot he eax register
			f->eax = tell(fd);
			// Break the switch statement
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
