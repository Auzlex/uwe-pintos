#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

// boolean used to hide debug logs
static bool DebugLogs = true;

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

struct process *process_create (tid_t tid);
struct process *get_child_process (struct thread *t, tid_t child_tid);
void update_parent_process_status (struct thread *child, enum process_status status);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
	
	if(DebugLogs) // debug log
		printf("process_execute -> file_name = %s\n", file_name);
	
	char *fn_copy;		// reference to palloc page
	tid_t tid;			// reference to current thread id 
	struct thread *cur; // reference to current thread
	struct process *cur_p;  // reference to our process information

	/* Make a copy of FILE_NAME.
	Otherwise there's a race between the caller and load(). */
	fn_copy = palloc_get_page (0); // 0 PAL_USER | PAL_ZERO
	
	// check if the fn_copy returned null if so this process executed an error
	if (fn_copy == NULL)
	{	
		if(DebugLogs) // debug log
			printf("process_execute (FAILED fn_copy==null) -> file_name = %s\n", file_name);
		
		return TID_ERROR;	
	}

	strlcpy (fn_copy, file_name, PGSIZE);
	char *save_ptr, *real_name; // variables for strtok_r  

	// strip the arguments from the actual file name of the program
	real_name = strtok_r(file_name, " ", &save_ptr);

	if(DebugLogs) // debug log
		printf("process_execute -> real_name = %s  file_name = %s\n", real_name, file_name);

	/* Create a new thread to execute FILE_NAME. */
	tid = thread_create (real_name, PRI_DEFAULT, start_process, fn_copy);
	
	// if error free page
	if (tid == TID_ERROR)
	{
		palloc_free_page (fn_copy); 	
	}
	
	// get the current thread
	cur = thread_current();
	
	// current process create with thread id
	cur_p = process_create (tid);

	// if the current process is null then woopies we failed to create a new process
	if (cur_p == NULL)
	{
		// free page and return -1
		palloc_free_page (fn_copy); 
		
		// free process
		free(cur_p);
		
		if(DebugLogs) // debug log
			printf("process_execute() -> cur_p == NULL\n");
		
		return -1;
	}

	// Inserts ELEM just before BEFORE, which may be either an interior element or a tail.
	list_push_back (&cur->children, &cur_p->elem);
	
	// check process load status if is failed then return -1
	if (cur_p->process_status == LOAD_FAILED)
	{
		if(DebugLogs) // debug log
			printf("cur_p->process_status == LOAD_FAILED\n");
		return -1;	
	}
	
	//Frees block P, which must have been previously allocated with malloc(), calloc(), or realloc().
	//free(cur_p);

	// if we are successful then woola
	return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  	char *file_name = file_name_;
  	struct intr_frame if_;
  	bool success;
	
	if(DebugLogs) // debug log
		printf("start_process -> file_name = %s\n", file_name);

	/* Initialize interrupt frame and load executable. */
	memset (&if_, 0, sizeof if_);
	if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
	if_.cs = SEL_UCSEG;
	if_.eflags = FLAG_IF | FLAG_MBS;

	// return success on load
	success = load (file_name, &if_.eip, &if_.esp);

	/* If load failed, quit. */
	palloc_free_page (file_name);
	if (!success) 
	{
		// if thread fails
		thread_current()->exit_code = -1;
		thread_exit ();
	}
	
	// so if we successfully load the process lets update their parent process
	// pass in the current thread
	// depending on the successful load of a process we will return a difference status
	// bless ternary operators!!!
	update_parent_process_status(thread_current(), success ? LOAD_COMPLETE : LOAD_FAILED);

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
	asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
	NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{
    // FIXME: @bgaster --- quick hack to make sure processes execute!
  	//for(;;) ;
    
    //return -1;
	
	// get the current thread
	struct thread *cur = thread_current ();
	
	// get the current child process
	struct process *p = get_child_process (cur, child_tid);
	int exit_status; // temp variable for exit status because we will remove the child later

	// not a child (or invalid)
	if (p == NULL)
	{
		if(DebugLogs) // debug log
			printf("p == null -> returning -1\n", (int)child_tid);
		return -1;
	}
		

	// already being waited for
	if (p->waiting)
	{
		if(DebugLogs) // debug log
			printf("p is already waiting -> returning -1\n", (int)child_tid);
		return -1;
	}

	// mark it as being waited for
	p->waiting = true;

	// wait for process to exit if it hasn't already
	exit_status = p->exit_status;

	if(DebugLogs) // debug log
		printf("process_wait*( child_tid = %d ) -> invoked.\n", (int)child_tid);
	
	// remove child process now that it's done with
	list_remove (&p->elem);
	
	//Frees block P, which must have been previously allocated with malloc(), calloc(), or realloc().
	free (p);

	// return the exit status
	return exit_status;

}


/* Free the current process's resources. */
void
process_exit (void)
{
	struct thread *cur = thread_current ();
	uint32_t *pd;

	/* 
	
		SOME SORT OF CODE IMPLMENTATION NEEDS TO GO IN HERE
		TO TERMINATE CHILD PROCESSES
	
	*/
	
	/* Deallocating each child process' memory
     from children's list */
	while(!list_empty(&cur->children))
	{
		struct list_elem * e = list_pop_front(&cur->children);
		struct process * p = list_entry(e,struct process,elem);
		list_remove(e);
		free(p);
	}
	
	// assign the thread exit code to the structure
	cur->exit_code = thread_exitcode();

	/* Destroy the current process's page directory and switch back
	to the kernel-only page directory. */
	pd = cur->pagedir;
	
	if (pd != NULL) 
	{
		/* Correct ordering here is crucial.  We must set
		 cur->pagedir to NULL before switching page directories,
		 so that a timer interrupt can't switch back to the
		 process page directory.  We must activate the base page
		 directory before destroying the process's page
		 directory, or our active page directory will be one
		 that's been freed (and cleared). */
		cur->pagedir = NULL;
		pagedir_activate (NULL);
		pagedir_destroy (pd);

		/* get current name */
		printf("%s: exit(0)\n",cur->name);
	}
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
	struct thread *t = thread_current ();
	
	//printf("process_activate t name = %s\n", t->name);

	/* say the thread has sucessfully loaded */
	t->is_child_loaded = true;

	/* Activate thread's page tables. */
	pagedir_activate (t->pagedir);

	/* Set thread's kernel stack for use in processing
	 interrupts. */
	tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, char ** argv, int argc);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

	char file_name_copy[100];   
	strlcpy(file_name_copy, file_name, 100);   
	char *argv[255]; 
	int argc; 
	char *save_ptr;
	
	argv[0] = strtok_r(file_name_copy, " ", &save_ptr);  
	
	char *token;   
	argc = 1; 
	
	while((token = strtok_r(NULL, " ", &save_ptr))!=NULL) 
	{ 
		argv[argc++] = token; 
	}
	
  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

	printf("load -> argv[0] = %s\n", argv[0]);
	
  /* Open executable file. */
  file = filesys_open (argv[0]);

  if (file == NULL) 
    {
	  printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp, argv, argc))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp,  char **argv, int argc) 
{
	uint8_t *kpage;
	bool success = false;

	printf( "setup_stack -> esp = %s, argv = %s, argc = %d \n", esp, argv, argc );

	// obtain a single free page to return its kernal virtual address
	// PAL_USER is set so the page is obtained from the user pool
	// PAL_ZERO is set so the page is filled with zeros
	kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	
	// we check for kpage just in case because if there are no pages
	// then we recieve a null pointer
	if (kpage != NULL) 
	{
		// mapping from the user virtual address UPAGE to kernal adress KPAGE to the page table
		success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);

		// on success
		if (success) 
		{

		  *esp = PHYS_BASE; 		// get the stack
		  int i = argc;				// get the argument number from the load function       
		  uint32_t * arr[argc];   	// this array holds reference to differences arguments in the stack       

		  // setup arguments
		  // while i less than or equal to 0 decrement i by 1
		  while(--i >= 0) 
		  { 
			  // subtrack the length of the argument + 1 * size of the char (1 byte) size from the stack
			  *esp = *esp -(strlen(argv[i])+1)*sizeof(char);

			  // set element at i with the refernce of esp e.g. the arguments stored in the stack
			  arr[i] = (uint32_t *)*esp;

			  // copying the argument to the esp (stack) at i;
			  memcpy(*esp,argv[i],strlen(argv[i])+1);
		  } 

		  *esp = *esp -4; 			// subtrack 4 from the stack
		  (*(int *)(*esp)) = 0;		// sentinel:: set the value 0 int           
		  i = argc;         		// set i to arg num because it already got decremented

		  // while i less than or equal to 0 decrement i by 1
		  while( --i >= 0) 
		  { 
			  *esp = *esp -4;					// subtrack 4 from the stack
			  (*(uint32_t **)(*esp)) = arr[i]; 	// reference to an arugment pointer in the stack as int 32
		  } 

		  // as far as everyone is aware it shifts the data into the bottom section of the hex dump
		  *esp = *esp -4; // shift the data in the stack by moving it back by 4

		  // shift data in 4 segments in stack of c0000000 c0000010
		  (*(uintptr_t  **)(*esp)) = (*esp+4); 

		  // shift everything in the stack back by 4 again
		  *esp = *esp -4; 

		  // inject our argc arguments number which is stored at the top as 02 if the command is "echo x"
		  *(int *)(*esp) = argc; 

		  // move everything in the stack by negative 4
		  *esp = *esp -4;
		  (*(int *)(*esp)) = 0; // another sentinel stack pointer target to be 0

		} 
		else
		{
			palloc_free_page (kpage);
		}
        
    }

	// debug hex dump :: prints out contents of the stack between base and current esp posistion
	hex_dump(PHYS_BASE, *esp, PHYS_BASE-(*esp), true);   
	
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

// this function creates a pointer to a struct when called
struct process *process_create (tid_t tid) // we accept a thread id
{
	
	if(DebugLogs) // debug log
	{	
		printf("process_create( tid_t tid = %d )\n",(int)tid);
		printf("process_create( tid_t tid = %d )\n",tid);	
	}
	
	// we request to allocate memory for our struct and get a pointer
  	struct process *p_ptr = malloc (sizeof (struct process));

	// null check the pointer
	if (p_ptr == NULL)
	{
		if(DebugLogs) // debug log
			printf("p_ptr returned null -> process_create will return -1\n");
		return NULL; // return null because it failed to allocate memory
	}
		
	// we set the size of the pointer to be the size of the struct
	memset (p_ptr, 0, sizeof (struct process));
	
	// convert the thread id to process id which is defiend in process.h
	// thread id is also an int
	p_ptr->pid = (pid_t)tid;
	
	if(DebugLogs) // debug log
		printf("\n\nprocess struct pid = %d\n\n",p_ptr->pid);
	
	// set the thread status first
	p_ptr->process_status = NOT_LOADED;
	
	// set thread status variables
	p_ptr->running = true;
	p_ptr->waiting = false;
	
	// initialize list elem on the process
	list_init(&p_ptr->elem);

	// return struct pointer
  	return p_ptr;
}

/*static inline bool
is_head (struct list_elem *elem)
{
  return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

static inline bool
is_interior (struct list_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}*/

// this function creates a pointer to a struct when called
struct process *get_child_process (struct thread *t, tid_t thread_id)
{
	struct list_elem *e;
	struct process *p;
	

	// null check the thread
	if (t == NULL)
	{
		if(DebugLogs) // debug log
			printf("struct thread *t returned null\n");
		return NULL;
	}
		
	//if(DebugLogs) // debug log
		//printf("\nITERATION LIST DEBUG:\n\n");
	
	// null check next elem in the list
	/*if(is_head(e) != NULL)
	{
		// for every element within the threads children
		for (e = list_begin (&t->children); e != list_end (&t->children);e = list_next (e))
		{

			//if(is_head(e) != NULL)
			//{
				// get the process struct
				p = list_entry (e, struct process, elem);

				//if(DebugLogs) // debug log
					//printf("ids %d == %d\n", (pid_t)p->pid, (int)thread_id);

				// we need to check if the target child thread id is the same as the process id
				if (p->pid == (pid_t) thread_id)
					return p; // if so return it
			//}

		}

	}*/
	
	//if(DebugLogs) // debug log
		//printf("\n\nITERATION LIST DEBUG END\n\n");

	// if we could not find our process id return null it has no child processes
	return NULL;
}

// called when we want to update a parent process status given a thread and process_status
void update_parent_process_status (struct thread *child, enum process_status status)
{
	// get the child process from the thread
  	struct process *p = get_child_process (child->parent, child->tid);

	// null check
	if (p != NULL)
	{
		// if the process is valid update its status
  		p->process_status = status;
	}
	else
	{
		if(DebugLogs) // debug log
			printf("update_parent_process_status( struct thread*child->tid = %d ) -> get_child_process return null\n", (int)child->tid);
	}
	
	//Frees block P, which must have been previously allocated with malloc(), calloc(), or realloc().
	//free (p);

}


//--------------------------------------------------------------------
