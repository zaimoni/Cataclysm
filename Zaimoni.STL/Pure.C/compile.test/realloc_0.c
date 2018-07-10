/* realloc_0.c */
/* tests for whether realloc(...,0) returns NULL */
/* (C)2010 Kenneth Boyd, license: MIT.txt */

#define STRING_LITERAL_TO_STDOUT(A) fwrite(A,sizeof(A)-1,1,stdout)

#include <stdlib.h>
#include <stdio.h>

int main()
{
	char* test = (char*)(malloc(3));
	if (!test) return 0;
	char* test2 = (char*)(realloc(test,0));
	if (!test2) STRING_LITERAL_TO_STDOUT("#define ZAIMONI_REALLOC_TO_ZERO_IS_NULL 1\n");
	return 0;
}

