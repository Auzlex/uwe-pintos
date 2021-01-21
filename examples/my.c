#include <stdio.h>
#include <syscall.h>

int
main (void)
{
    printf("My.c is now running main!\n");
	
	/*  
		test syscall halt
	*/
	//halt(); // works

	/*  
		test syscall exit
	*/
	//exit(-1); // works
	
	/*  
		test syscall exec 
	*/
	//exec("echo"); // works
	
	/* 
		test syscall wait with exec :: idk probably still does not work
	*/
	/*printf("\n\nStarting exec(\"echo\")\n\n");
	
	int processID = (int)exec("echo");
	
	printf("\n\necho started...\n\n");
	printf("\n\nwaiting for echo...\n\n");
	wait(processID);
	
	printf("\n\necho waited...\n\n");*/
	
	
	/* 
		test syscall create,open,file size, seek, tell, read, write
		// thanks to this brilliant internal documentation of pintos:
		// https://www.cse.iitd.ernet.in/~sbansal/os/previous_years/2014/pintos/doc/pintos_html/file_8c-source.html#l00096
	*/
	printf("\n\ncreating testfile.txt and getting file size...\n\n");
	
	// text to be written into file
	const char sample = "Dis is very nouice yes! xd";
	
	// create testfile
	create("testfile.txt", 12);
	
	int handle = open("testfile.txt"); //testfile.txt
	
	// test seek
	seek(handle, 4);
	
	// test tell
	printf("tell() = %ud\n", (unsigned )tell(handle)); 
	
	// get the file size
	filesize(handle);
	
	// write to file
	printf("\n\n write...\n\n");
	write(handle, sample, sizeof sample - 1 );
	
	// read
	printf("\n\n read...\n\n");
	read(handle, sample, sizeof sample - 1);
	
	// test system call close the file
	close(handle);
	
	// test system call remove testfile.txt
	remove("testfile.txt");

    return EXIT_SUCCESS;
}
