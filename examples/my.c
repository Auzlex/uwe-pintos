#include <stdio.h>
#include <syscall.h>

int
main (void)
{
    printf("My.c is now running main -> void, \n");
	
	//halt();
	
	//exec();
	//wait(
	//exec("echo Natalie Portman is the reason I work out. I have this fantasy where we start talking at the Vanity Fair Oscars party bar");//);
    exec("echo Natalie Portman is the reason I work out.");
	create("testfile", 5);
	open("testfile");
	
	//wait(0);
	
    return EXIT_SUCCESS;
}
