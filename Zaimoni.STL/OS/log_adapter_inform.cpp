// log_adapter_inform.cpp
// implements standard data format adapters for logging
// C++ only (these are for overloads)
// (C)2009 Kenneth Boyd, license: MIT.txt

#include "../Logging.h"
#include "../Pure.C/format_util.h"
#include <limits.h>
#include <stdio.h>

// intmax_t
void INFORM(intmax_t B) {
	char buf[sizeof(intmax_t)*CHAR_BIT/3+2];
	z_imaxtoa(B,buf,10);
	_inform(buf,strlen(buf));
}

// uintmax_t
void INFORM(uintmax_t B) {
	char buf[sizeof(uintmax_t)*CHAR_BIT/3+2];
	z_umaxtoa(B,buf,10);
	_inform(buf,strlen(buf));
}

// long double
void INFORM(long double B) {
	char buf[25];
	sprintf(buf,"%.16Lg",B);
//	_gcvt(B,10,buf);
	_inform(buf,strlen(buf));
}

