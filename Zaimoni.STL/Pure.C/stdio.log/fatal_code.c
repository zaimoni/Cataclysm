/* fatal_code.c */
/* implements Logging.h for C stdout/stderr */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void _fatal_code(const char* const B,int exit_code)
{
	fwrite(B,strlen(B),1,stderr);
	fwrite("\n",1,1,stderr);
	exit(exit_code);
}

