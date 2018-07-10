/* format_util.h */
/* deal with failure of both C99 and POSIX3 to standardize conversion functions */
/* (C)2009 Kenneth Boyd, license: MIT.txt */

#ifndef FORMAT_UTIL_H
#define FORMAT_UTIL_H

#include <stddef.h>
#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! 
 * sort of like the non-standard function ultoa: renders an unsigned integer in buf as a string for bases 2 through 36, and returns buf.
 * 
 * \param target unsigned integer
 * \param buf    character buffer
 * \param radix  radix to render as
 * 
 * \return char* original buf
 *
 * \pre NULL!=buf
 * \pre 2<=radix<=36
 */
char* z_umaxtoa(uintmax_t target,char* const buf,unsigned int radix);

/* provide an assert debugging harness (depends on extern "C" to be safe) if assert is defined at all and useful */
#ifndef NDEBUG
#ifdef assert
#define z_umaxtoa(target,buf,radix) (assert(NULL!=(buf)),assert(2<=(radix) && 36>=(radix)),z_umaxtoa(target,buf,radix))
#endif
#endif

/*! 
 * sort of like the non-standard function ltoa: renders an unsigned integer in buf as a string for bases 2 through 36, and returns buf.
 * 
 * \param target signed integer
 * \param buf character buffer
 * \param radix radix to render as
 * 
 * \return char* original bug
 *
 * \pre NULL!=buf
 * \pre 2<=radix<=36
 */
char* z_imaxtoa(intmax_t target,char* const buf,int radix);

/* provide an assert debugging harness (depends on extern "C" to be safe) if assert is defined at all and useful */
#ifndef NDEBUG
#ifdef assert
#define z_imaxtoa(target,buf,radix) (assert(NULL!=(buf)),assert(2<=(radix) && 36>=(radix)),z_imaxtoa(target,buf,radix))
#endif
#endif

/*! 
 * sort of like strtoull: renders a string as an unsigned integer.  Implementing to void thinking about whether unsigned long long
 * really is the largest unsigned integer type on a system.  This skips leading C locale whitespace before attempting conversion.
 * POSIX 3 analog: strtoumax
 * 
 * \param buf : string to convert
 * \param radix : radix to parse
 * 
 * \return uintmax_t : converted number
 *
 * \pre NULL!=buf
 * \pre 2<=radix<=36
 * \post if there is no recognizable positive integer to parse, return 0 and set errno to EINVAL
 * \post if the positive integer is larger than UINTMAX_MAX, return 0 and set errno to ERANGE
 */
uintmax_t z_atoumax(const char* buf,size_t radix);

/* provide an assert debugging harness (depends on extern "C" to be safe) if assert is defined at all and useful */
#ifndef NDEBUG
#ifdef assert
#define z_atoumax(buf,radix) (assert(NULL!=(buf)),assert(2U<=(radix) && 36U>=(radix)),z_atoumax(buf,radix))
#endif
#endif

/*! 
 * provides a relatively safe way to overflow-aware cast from uintmax_t to intmax_t, with intentional negation.  errno is set to ERANGE if the result is out-of-bounds.
 * this wraps handling two's-complement vs one's complement vs sign bit
 * 
 * \param negative
 * \param x
 * 
 * \return intmax_t
 *
 * \post if the result is outside of the range of intmax_t, return 0 and set errno to ERANGE
 */
intmax_t imax_from_umax(bool negative,uintmax_t x);


#ifdef __cplusplus
}	/* end extern "C" */
#endif

#endif
