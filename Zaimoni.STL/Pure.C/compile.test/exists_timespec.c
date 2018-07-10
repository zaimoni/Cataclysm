/* exists_timespec.c */
/* tests for POSIX timespec */
/* (C)2018 Kenneth Boyd, license: MIT.txt */

#include <stdio.h>
#include <time.h>

#define STRING_LITERAL_TO_STDOUT(A) fwrite(A,sizeof(A)-1,1,stdout)

int main()
{
	struct timespec x0;	/* check for both struct and required fields */
	x0.tv_sec;
	x0.tv_nsec;
	STRING_LITERAL_TO_STDOUT("#define ZAIMONI_HAVE_POSIX_TIMESPEC_IN_TIME_H 1\n");
	return 0;
}
