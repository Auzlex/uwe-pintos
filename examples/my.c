#include <stdio.h>
#include <syscall.h>

int
main (void)
{
    printf("Hello, World: Texting Create\n");
	
	//halt();
	
	//exec();
	//wait(
	//exec("echo Natalie Portman is the reason I work out. I have this fantasy where we start talking at the Vanity Fair Oscars party bar");//);
    
	create ("createdfile", 0);
	
    return EXIT_SUCCESS;
}
