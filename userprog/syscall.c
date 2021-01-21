#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "process.h"

#include "threads/vaddr.h"
#include "threads/synch.h"

#include "filesys/filesys.h"

int grabFromStack(struct intr_frame *f UNUSED, int pos);
void syscall_init (void);
bool IsValidVAddress(void * vaddress);

void halt(void);
void exit(int status);
tid_t exec(const char* cmd_line);
int wait(tid_t id);
bool create(char* filename, unsigned size);
bool remove_file(char* filename);
int open(const char* filename);
int get_filesize(int fd);
int write (int fd, void *buffer, unsigned size);
void seek (int fd, unsigned position);
int read(int fd, void *dataBuf, unsigned readSize);
unsigned tell(int fd);
void close_via_fd (int fd);

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
	File loading functions 
*/

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
    	struct file_desc * file_descriptor = list_entry(current_element, struct file_desc, elem);
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

// called by another application when we want to execute another process
tid_t exec(const char * cmd_line)
{
	//struct thread *cur; // reference to current thread
	//struct process *p; // reference to child process
	
	// debug log
    if(debug)
	    printf( "syscall.c -> exec(const char* cmd_Line = %s) -> invoked!\n", cmd_line );

	// reference to child_process 
	//struct thread *child_process;
	tid_t pid;
	
	// null check that input
	if(cmd_line == NULL)
	{
		// debug log
		if(debug)
			printf( "Error: For some reason cmd_line returned null!\n" );

		return -1;
	}

	// begin synchronization :: 
	lock_acquire(&syscall_lock);
	
	// begin a new process and store its processID into pid_t
	pid = process_execute(cmd_line);
	
	// end synchronization :: 
	lock_release(&syscall_lock);
	
	return pid;

}

// Waits for a child process pidand retrieves the child's exit status. 
int wait(tid_t id)
{
	
	if(debug)
		printf("wait( tid_t = %d ) -> invoked\n", (int)id);
	
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

// Opens a file of a given name. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened. 
int open(const char* filename)
{

	if(filename == NULL)
	{
		if(debug)
			printf("given filename is null\n");
		return -1;
	}

	// begin synchronization :: because a file can be opened by a single process or different processes which implies child processes
	lock_acquire(&syscall_lock);

	// open file with our desired name
	struct file* fp = filesys_open (filename);

	// end synchronization :: 
	lock_release(&syscall_lock);
	
	// if the file pointer is null then return -1 as bad file name or we cannot open it???
	if (fp == NULL) 
	{
		if(debug)
			printf("file does not exist fp == null\n");

		return -1;
	}
	/* 
    
        FILE DESCRIPTOR STRUCT REFERENCE in process.h

        struct file_desc {
			struct file * fp;		// reference to the file
			int fd; 				// the file descriptor
			struct list_elem elem; 	// list for list_push_front
		};

    */

	// allocate memory for the file descriptor
	struct file_desc * fd_elem = malloc(sizeof(struct file_desc));

	// if we open the same file at a different time we make sure the file descriptor is different
	fd_elem->fd = ++thread_current()->fd_count;

	// store the file into the fd_elem in the file descriptor struct
	fd_elem->fp = fp;

	// removes elements first though last exclusive from their current list and inserts them just before
	// which be either the interior element or the tail
	list_push_front( &thread_current()->file_list, &fd_elem->elem );
	
	if(debug)
		printf("returned file desciptor = %d\n",fd_elem->fd);

	// return the file descriptor
	return fd_elem->fd;
}

// get file size
int get_filesize(int fd)
{
	
	// begin synchronization ::
	lock_acquire(&syscall_lock);
	
	// struct for the file
	struct file_desc *file_descriptor = get_file_descriptor(fd); // get the file descriptor
	
	/* If no elem having the descriptor fd exists */
	if(file_descriptor == NULL)
		return -1;
	
	// Get the file from the file_descriptor
  	struct file* file_ptr = file_descriptor->fp;
	
	// get length of file
	int fileLength = 0; //stores file length 
	if (file_ptr != NULL) // if file is not null
	{ 
		// gets length of file
		fileLength = file_length(file_ptr);

		if(debug)
			printf( "get_filesize( int fd = %d ) -> filelength = %d\n", fd, fileLength );
	}
	
	// end synchronization ::
	lock_release(&syscall_lock);
	
	return fileLength;
}

// Writes size bytes from buffer to the open file fd
int write (int fd, void *buffer, unsigned size)
{

	// check if we can actually write out
	if(fd == STDOUT_FILENO) // STDOUT
	{
		// writes n characters to the BUFFER to the console
		putbuf((const char*)buffer, (unsigned )size);
		
		return size;
	}
	else
	{

		if(debug)
		{
			printf( "write (int fd = %d, void *buffer, unsigned size %d)\n", fd, (unsigned )size );
		}
			
		int fs = -1;

		// struct for the file
		struct file_desc *file_descriptor = get_file_descriptor(fd); // get the file descriptor
		
		/* If no elem having the descriptor fd exists */
		if(file_descriptor == NULL)
		{
			return fs;
		}
		
		// Get the file from the file_descriptor
		struct file* file_ptr = file_descriptor->fp;

		// begin sync
		lock_acquire(&syscall_lock);

		// write to the file using filesys function
		fs = file_write(file_ptr,buffer,size);

		// end sync
		lock_release(&syscall_lock);
		
		return fs;
	}
}

// Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning of the file. 
void seek (int fd, unsigned position)
{

	if(debug) // debug
		printf( "seek( int fd = %d, unsigned position = %d )\n", fd, (int)position );

	// struct for the file
	struct file_desc *file_descriptor = get_file_descriptor(fd); // get the file descriptor

	/* If no elem having the descriptor fd exists */
	if(file_descriptor == NULL)
		return -1;

	// Get the file from the file_descriptor
	struct file* file_ptr = file_descriptor->fp;

	// get length of file
	int fileLength = 0; //stores file length 
	if (file_ptr == NULL) // if file is null return -1
	{ 
		return -1;
	}

	// begin synchronization ::
	lock_acquire(&syscall_lock);
	// perform file seek
	file_seek(file_ptr,position);
	// end synchronization ::
	lock_release(&syscall_lock);
}

// Function to read a designated amount of data from a file
int read(int fd, void *dataBuf, unsigned readSize) 
{
	/* Validation */
	// Copy the inputted dataBuffer into a byte called buffer
	//uint8_t buffer = (uint8_t *) dataBuf;

	// writes n characters to the BUFFER to the console
	if(debug)
	{
		printf( "read(int fd = %d, void *dataBuf, unsigned readSize = %d)\n", fd, (unsigned )readSize );
	}	

	// if the file desciptor is minus 1 then bogo we got a faulty fd stop here!
	if(fd == -1)
	{
		return -1;
	}

  	// Create an unsigned int to store the data from the file
  	unsigned data = 0;
  	// If the file descriptor is the standard input (0) .aka reading from the keyboard
  	if (fd == STDIN_FILENO) {
  		// While data is smaller than the read size
		while (data < readSize) {
			// Store the keyboard input as the dataBuf
      		*((char *)dataBuf+data) = input_getc();
      		// Increment the data variable
      		data++;
    	}
    	// Return the data variable
		return data;
    }

  	// Get the file descriptor for the corresponding fd number
  	struct file_desc * file_descriptor;
  	file_descriptor = get_file_descriptor(fd);

  	// Get the file from the file_descriptor
  	struct file* file = file_descriptor->fp;
  	// If the file is NULL
  	if (file == NULL) {
    	// Return -1 representing a failure
    	return -1;
    }

    // Sychronisation - aquire a lock from the current thread so that the file can't be edited while accessing it
  	lock_acquire(&syscall_lock);

  	// Set the data to the results of a file read
  	data = file_read(file, dataBuf, readSize);

  	// Release the file being read
  	lock_release(&syscall_lock);	

  	// Return the data from the file as an integer
	return (int) data;
}

// Get the position of the from the beggining in the open file (file descriptor)
unsigned tell(int fd) 
{
    // Synchronisation - lock the file so that it cannot be opened by another process
  	lock_acquire(&syscall_lock);
  	// Get the file descriptor for the corresponding fd number
  	struct file_desc * file_descriptor = get_file_descriptor(fd);
  	// Get the file from the file_descriptor
  	struct file* acFile = file_descriptor->fp;
  	// If the file is Null
  	if (acFile == NULL) {
  		// Release the file
      	lock_release(&syscall_lock);
      	// Return -1 representing a failure
      	return -1;
    }
  	// Get the actual position in the file
  	unsigned pos = file_tell(acFile);
  	// Release the file
  	lock_release(&syscall_lock);
  	return pos;
}

void close_via_fd (int fd)
{
	if(debug)
		printf("close_via_fd(int fd = %d)\n",fd);
	
	// if the file desciptor is minus 1 then bogo we got a faulty fd stop here!
	if(fd == -1)
	{
		return -1;
	}

	// Get the file descriptor for the corresponding fd number
	struct file_desc * file_descriptor = get_file_descriptor(fd);
	
  	// Get the file from the file_descriptor
  	struct file* acFile = file_descriptor->fp;
	
  	// If the file is Null
	
  	if (acFile == NULL) 
	{
  		// Release the file
      	lock_release(&syscall_lock);
		
      	// Return -1 representing a failure
      	return -1;
    }
	
	// Synchronisation
  	lock_acquire(&syscall_lock);
	
	// invoke the filesys to close the file with the filepointer in the file descriptor
	file_close(acFile);
	
	// end Synchronisation
	lock_release(&syscall_lock);
	
	// remove the file descriptor from the list elem
  	list_remove(&file_descriptor->elem);
	
	// remove file_descriptor from memory with free because we used malloc to assigned it in memory
  	free(file_descriptor);
	
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
    //if(debug)
	    //printf( "syscall_handler -> invoked syscall_number = %d\n", syscall_number );
	
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
			if(!IsValidVAddress(f->esp + 1)) // 1
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
			// get the command line pointer from the stack
			const char* cmdline = ((char*) *((int*)f->esp + 1)); // 1
			
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
			if(!IsValidVAddress(f->esp + 1)) // f->esp + 1
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
			f->eax = wait(*((int*)f->esp + 1)); // *((int*)f->esp + 1)
			
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
			
			if(debug) // debug log 
				printf("SYS_REMOVE called\n");
			
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
			
			// Get the file name from the stack
			char* fileName = ((char*) *((int*)f->esp + 1));
			// Remove the file and save the result to
			f->eax = remove_file(fileName);
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
		case SYS_FILESIZE: // Chris has not responded so Charles has implemented it
			
			if(debug)
				printf("SYS_FILESIZE called\n");

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
			
			// get file descriptor from the stack
			// set the EAX register :: System calls that return a value can do so
			// by modifying the eax member of struct intr_frame
  			f->eax = get_filesize(*((int*)f->esp + 1));

			break;
		case SYS_READ:;// implemented by faegan
			
			//if(debug)
				//printf("SYS_READ called\n");
			
			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 4) || !IsValidVAddress(f->esp + 8) || !IsValidVAddress(f->esp + 12))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
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
			
			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 4) || !IsValidVAddress(f->esp + 8) || !IsValidVAddress(f->esp + 12))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
			// information reference https://www.quora.com/What-is-the-file-descriptor-What-are-STDIN_FILENO-STDOUT_FILENO-and-STDERR_FILENO
			// get the file descriptor from the stack
			int fd = *(int *)(f->esp + 4);
			
			// create a buffer
			void *buffer = *(char**)(f->esp + 8);
			
			// size of buffer
			unsigned size = *(unsigned *)(f->esp + 12);

			// write to file
			f->eax = write(fd, buffer, size);

			break;
		case SYS_SEEK:;
			
			if(debug)
				printf("SYS_SEEK called\n");
			
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
			
			// get the file descriptor from the stack
			int tfd = *(int *)(f->esp + 4);
			
			// posistion
			unsigned pos = *(unsigned *)(f->esp + 5);

			// seek with file descriptor
			seek(tfd, pos);
			
			break;
		case SYS_TELL: // implemented by faegan

			if(debug)
				printf("SYS_TELL called\n");

			// validate memory
			// invalid pointers must be rejected without harm to the kernel or other running processes
			if(!IsValidVAddress(f->esp + 4))
			{
				if(debug)
					printf( "Pointers not valid exiting thread :: kernal violation...\n" );
				
				// if any IsValidVAddress returns false
				// halt code here as the thread will be terminated
				exit(-1); // is invoked because why continue if we have bad memory here
				return;
			}
			
			// Get the file descriptor value from the stack
			fd = *(int *)(f->esp + 4);
			// Get the position in the file from the file descriptor number and save it tot he eax register
			f->eax = tell(fd);
			// Break the switch statement
			break;
		case SYS_CLOSE:
			
			if(debug)
				printf("SYS_CLOSE called\n");
			
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
			
			// get the fd from the stack and bomvayage file!
			close_via_fd(*((int*)f->esp + 1)); // 4
			
			break;
			
		default:
			
			// invoke procedure
			throw_not_implemented_message_and_terminate_thread(syscall_number);
			break;
	}
}
