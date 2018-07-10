/* log.c */
/* implements Logging.h for C stdout/stderr */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#include <stddef.h>
#include <stdio.h>

void _log(const char* const B, size_t len)
{
	fwrite(B,len,1,stderr);
	fwrite("\n",1,1,stderr);
}

