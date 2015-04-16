#include "stdio.h"
#include "pop3.h"



void handlee(int remote_command)
{
	//process command here
	printf("I want to handle %d\n", remote_command);
}

int main()
{
	pop3_main(&handlee);
}

