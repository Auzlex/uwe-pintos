#include <stdio.h>
#include <syscall.h>

int
main (void)
{
    printf("My.c is now running main -> void, \n");
	
	//halt();
	
	//exec("echo"); // works
	int processID = (int)
	//exec("echo");
	wait(processID); // works
	printf("Wait success\n now creating testfile\n\n");
	create("testfile", 5);
	filesize(open("testfile"));
	
	
	//close("testfile");
	//remove("testfile", 5);

	
    return EXIT_SUCCESS;
}
