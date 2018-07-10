/* format_util.c */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#include "format_util.h"

#include <errno.h>
#include <string.h>
#include <float.h>

#ifdef __cplusplus
extern "C" 
#endif
char* z_umaxtoa(uintmax_t target,char* const buf,unsigned int radix)
{
	char* ret = buf;
	uintmax_t power_up = 1;
	while(power_up<=target/radix) power_up *= radix;
	do	{
		unsigned char tmp = (unsigned char)(target/power_up);
		tmp += (10>tmp) ? (unsigned char)('0') : (unsigned char)('A')-10U;	// ahem...assumes ASCII linear A-Z
		*ret++ = tmp;
		target %= power_up;
		power_up /= radix;
		}
	while(0<power_up);
	*ret = '\0';
	return buf;
}

#ifdef __cplusplus
extern "C" 
#endif
char* z_imaxtoa(intmax_t target,char* const buf,int radix)
{
	char* ret = buf;
	if (0<=target)
		{
		intmax_t power_up = 1;
		while(power_up<=target/radix) power_up *= radix;
		do	{
			unsigned char tmp = (unsigned char)(target/power_up);
			tmp += (10>tmp) ? (unsigned char)('0') : (unsigned char)('A')-10U;	// ahem...assumes ASCII linear A-Z
			*ret++ = tmp;
			target %= power_up;
			power_up /= radix;
			}
		while(0<power_up);
		}
	else{
		intmax_t power_up = -1;
		while(power_up>=target/radix) power_up *= radix;
		*ret++ = '-';
		do	{
			unsigned char tmp = (unsigned char)(target/power_up);
			tmp += (10>tmp) ? (unsigned char)('0') : (unsigned char)('A')-10U;	// ahem...assumes ASCII linear A-Z
			*ret++ = tmp;
			target %= power_up;
			power_up /= radix;
			}
		while(0>power_up);
		}
	*ret = '\0';
	return buf;
}

#if 0
#ifdef __cplusplus
extern "C" 
#endif
char* z_ldbltoa(long double target,char* const buf,int radix)
{
	char* ret = buf;
	if (0<=target)
		{
		intmax_t power_up = 1;
		while(power_up<=target/radix) power_up *= radix;
		do	{
			unsigned char tmp = (unsigned char)(target/power_up);
			tmp += (10>tmp) ? (unsigned char)('0') : (unsigned char)('A')-10U;	// ahem...assumes ASCII linear A-Z
			*ret++ = tmp;
			target %= power_up;
			power_up /= radix;
			}
		while(0<power_up);
		}
	else{
		intmax_t power_up = -1;
		while(power_up>=target/radix) power_up *= radix;
		*ret++ = '-';
		do	{
			unsigned char tmp = (unsigned char)(target/power_up);
			tmp += (10>tmp) ? (unsigned char)('0') : (unsigned char)('A')-10U;	// ahem...assumes ASCII linear A-Z
			*ret++ = tmp;
			target %= power_up;
			power_up /= radix;
			}
		while(0>power_up);
		}
	*ret = '\0';
	return buf;
}
#endif

#ifdef __cplusplus
extern "C" 
#endif
uintmax_t z_atoumax(const char* buf,size_t radix)
{
	const uintmax_t max_safe_radix_mult = UINTMAX_MAX/radix;
	uintmax_t ret = 0;
	buf += strspn(buf," \n\r\f\t\v");	/* skip over C whitespace, if any */
	if (10>=radix)
		{
		const char ub = (char)(radix)+'0';
#define CHAR_IN_RANGE(A) ('0'<=(A) && ub>(A))
		if (!CHAR_IN_RANGE(*buf))
			{
			errno = EINVAL;
			return 0;
			};
		do	{
			const unsigned char tmp = *(buf++)-'0';
			if (max_safe_radix_mult<ret)
				{
				errno = ERANGE;
				return 0;
				};
			ret*=radix;
			if (UINTMAX_MAX-tmp<ret)
				{
				errno = ERANGE;
				return 0;
				}
			ret += tmp;
			}
		while(CHAR_IN_RANGE(*buf));
#undef CHAR_IN_RANGE
		return ret;
		}
	else{
#define dec_ub ((char)(1)+'9')
		//! \todo fix assuming ASCII
#define CHAR_IN_RANGE(A) (('0'<=(A) && dec_ub>(A)) || ('A'<=(A) && uc_ub>(A)) || ('a'<=(A) && lc_ub>(A)))
		const char uc_ub = 'A'+(char)(radix-10);
		const char lc_ub = 'a'+(char)(radix-10);
		if (!CHAR_IN_RANGE(*buf))
			{
			errno = EINVAL;
			return 0;
			};
		do	{
			char tmp = *(buf++);
			if (max_safe_radix_mult<ret)
				{
				errno = ERANGE;
				return 0;
				};
			ret*=radix;
			if 		('0'<=tmp && dec_ub>tmp)
				{
				tmp -= '0';
				}
			else if ('A'<=tmp && uc_ub>tmp)
				{
				tmp -= 'A';
				tmp += 10;
				}
			else{
				tmp -= 'a';
				tmp += 10;
				}
			if (UINTMAX_MAX-(uintmax_t)(tmp)<ret)
				{
				errno = ERANGE;
				return 0;
				}
			ret += (uintmax_t)(tmp);
			}
		while(CHAR_IN_RANGE(*buf));
#undef CHAR_IN_RANGE
#undef dec_ub
		return ret;
		}
}

#ifdef __cplusplus
extern "C" 
#endif
intmax_t imax_from_umax(bool negative,uintmax_t x)
{
	if (negative)
		{
		if ((uintmax_t)(INTMAX_MAX) >= x) return -((intmax_t)(x));
#if -INTMAX_MAX!=INTMAX_MIN
/* two's complement : tricky */
		if ((uintmax_t)(INTMAX_MAX)+1U == x) return -INTMAX_MAX-1;
#endif
		}
	else{
		if ((uintmax_t)(INTMAX_MAX) >= x) return (intmax_t)(x);
		}
	errno = ERANGE;
	return 0;
}

