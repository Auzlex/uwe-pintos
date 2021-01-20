#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    SYS_HALT,                   /* Halt the operating system. */ 				// done and tested
    SYS_EXIT,                   /* Terminate this process. */ 					// done and tested
		
    SYS_EXEC,                   /* Start another process. */					// done and tested (broken with args)
    SYS_WAIT,                   /* Wait for a child process to die. */			// done and tested (broken and does not wait)
		
    SYS_CREATE,                 /* Create a file. */							// done and tested
    SYS_REMOVE,                 /* Delete a file. */							// done and tested 	
    SYS_OPEN,                   /* Open a file. */								// done and tested
    SYS_FILESIZE,               /* Obtain a file's size. */						// done and tested
    SYS_READ,                   /* Read from a file. */							// done not tested
    SYS_WRITE,                  /* Write to a file. */							// done
    SYS_SEEK,                   /* Change position in a file. */				// not done
    SYS_TELL,                   /* Report current position in a file. */		// done needs tested
    SYS_CLOSE,                  /* Close a file. */								// done and tested

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
