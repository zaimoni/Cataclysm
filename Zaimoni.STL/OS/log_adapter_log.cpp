// log_adapter_log.cpp
// implements standard data format adapters for logging
// C++ only (these are for overloads)
// (C)2009 Kenneth Boyd, license: MIT.txt

#include "../Logging.h"
#include "../Pure.C/format_util.h"
#include <limits.h>
#include <stdio.h>

// intmax_t
void LOG(intmax_t B) {
	char buf[sizeof(intmax_t)*CHAR_BIT/3+2];
	z_imaxtoa(B,buf,10);
	_log(buf,strlen(buf));
}

// uintmax_t
void LOG(uintmax_t B) {
	char buf[sizeof(uintmax_t)*CHAR_BIT/3+2];
	z_umaxtoa(B,buf,10);
	_log(buf,strlen(buf));
}

// long double
void LOG(long double B) {
	char buf[25];
	sprintf(buf,"%.16Lg",B);
//	_gcvt(B,10,buf);
	_log(buf,strlen(buf));
}

