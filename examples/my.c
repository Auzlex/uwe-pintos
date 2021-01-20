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
		test syscall create,open,file size
	*/
	printf("\n\ncreating testfile.txt and getting file size...\n\n");
	
	// text to be written into file
	const char sample = "Dis is very nouice yes! xd";
	
	// create testfile
	create("testfile.txt", 12);
	
	int handle = open("testfile.txt"); //testfile.txt

	
	// get the file size
	filesize(handle);
	
	printf("\n\n write...\n\n");
	
	// write to file
	write(handle, sample, sizeof sample - 1 );
	
	printf("\n\n read...\n\n");
	
	// read
	read(handle, sample, sizeof sample - 1);
	
	// close the file
	close(handle);
	
	// remove testfile
	remove("testfile.txt");

	
    return EXIT_SUCCESS;
}
