/* fatal.c */
/* implements Logging.h for C stdout/stderr */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void _fatal(const char* const B)
{
	fwrite(B,strlen(B),1,stderr);
	fwrite("\n",1,1,stderr);
	exit(EXIT_FAILURE);
}

