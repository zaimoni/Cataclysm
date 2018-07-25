/* fatal_code.c */
/* implements Logging.h for C stdout/stderr */
/* (C)2009,2018 Kenneth Boyd, license: MIT.txt */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef GUI_FATAL
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#endif

void _fatal_code(const char* const B,int exit_code)
{
	fwrite(B,strlen(B),1,stderr);
	fwrite("\n",1,1,stderr);
#ifdef GUI_FATAL
#ifdef _WIN32
	MessageBox(NULL,B,GUI_FATAL,MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
#endif
#endif
	exit(exit_code);
}

