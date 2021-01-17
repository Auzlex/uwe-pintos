#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    SYS_HALT,                   /* Halt the operating system. */ 				// done
    SYS_EXIT,                   /* Terminate this process. */ 					// done
		
    SYS_EXEC,                   /* Start another process. */					// implemented but returns a page Fault violation error
    SYS_WAIT,                   /* Wait for a child process to die. */			// implemented idk how to test it
		
    SYS_CREATE,                 /* Create a file. */							// done
    SYS_REMOVE,                 /* Delete a file. */							// fa w doing this one		
    SYS_OPEN,                   /* Open a file. */								// done charles	implemented
    SYS_FILESIZE,               /* Obtain a file's size. */						// Chris doing this one
    SYS_READ,                   /* Read from a file. */							// fa w doing this one		
    SYS_WRITE,                  /* Write to a file. */							// done
    SYS_SEEK,                   /* Change position in a file. */				// Chris doing this one
    SYS_TELL,                   /* Report current position in a file. */		// fa w doing this one		
    SYS_CLOSE,                  /* Close a file. */								// Chris doing this one

    /* Project 3 and optionally project 4. */
    SYS_MMAP,                   /* Map a file into memory. */
    SYS_MUNMAP,                 /* Remove a memory mapping. */

    /* Project 4 only. */
    SYS_CHDIR,                  /* Change the current directory. */
    SYS_MKDIR,                  /* Create a directory. */
    SYS_READDIR,                /* Reads a directory entry. */
    SYS_ISDIR,                  /* Tests if a fd represents a directory. */
    SYS_INUMBER                 /* Returns the inode number for a fd. */
  };

#endif /* lib/syscall-nr.h */
