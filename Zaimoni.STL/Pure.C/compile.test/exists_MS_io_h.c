/* exists_MS_io_h.c */
/* tests for Microsoftish io.h */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#include <stdio.h>
#include <io.h>

#define STRING_LITERAL_TO_STDOUT(A) fwrite(A,sizeof(A)-1,1,stdout)

long get_filelength(FILE* src)
{
	return _filelength(_fileno(src));
}

int main()
{
	STRING_LITERAL_TO_STDOUT("#define ZAIMONI_HAS_MICROSOFT_IO_H 1\n");
	return 0;
}
