/* Compiler.h */
/* cross-compiler compatiblity header */
/* (C)2009,2010,2018 Kenneth Boyd, license: MIT.txt */

#ifndef ZAIMONI_COMPILER_H
#define ZAIMONI_COMPILER_H 1

/* C standard headers */
#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

/* these two control macros are command-line, not auto-detected */
/* ZAIMONI_HAVE_ACCURATE_MSIZE should be defined when linking with the custom memory manager */
/* ZAIMONI_FORCE_ISO should be defined when intentionally ISO even then */
#if defined(ZAIMONI_HAVE_ACCURATE_MSIZE) && !defined(ZAIMONI_FORCE_ISO)
#define ZAIMONI_LEN_WITH_NULL(A) (A)
#define ZAIMONI_NULL_TERMINATE(A)
#define ZAIMONI_ISO_PARAM(A)
#define ZAIMONI_ISO_FAILOVER(A,B) A
#define ZAIMONI_ISO_ONLY(A)
#else
#define ZAIMONI_LEN_WITH_NULL(A) ((A)+1)
#define ZAIMONI_NULL_TERMINATE(A) A = '\0'
#define ZAIMONI_ISO_PARAM(A) , A
#define ZAIMONI_ISO_FAILOVER(A,B) B
#define ZAIMONI_ISO_ONLY(A) A
#endif

/* delayed expansion of preprocessing operators */
#define STRINGIZE(A) #A
#define DEEP_STRINGIZE(A) STRINGIZE(A)
#define CONCATENATE(A,B) A ## B
#define DEEP_CONCATENATE(A,B) CONCATENATE(A,B)

/* How to report a function name */
#ifdef __GNUC__
#	define ZAIMONI_FUNCNAME __PRETTY_FUNCTION__
#elif 1300<=_MSC_VER	/* __FUNCDNAME__ extension cuts in at Visual C++ .NET 2002 */
#	define ZAIMONI_FUNCNAME __FUNCDNAME__
#else
/* if no extensions, assume C99 */
#	define ZAIMONI_FUNCNAME __func__
#endif

/* hooks for C/C++ adapter code */
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

/* hooks for compiler extensions (this could be part of compiler test results) */
#define NO_RETURN
#define MS_NO_RETURN

/* GCC */
#ifdef __GNUC__
#undef NO_RETURN
#define NO_RETURN __attribute__ ((noreturn))
#endif
#ifdef _MSC_VER
#undef MS_NO_RETURN
#define MS_NO_RETURN __declspec(noreturn)
#endif

#ifdef __cplusplus
#	undef NO_RETURN
#	define NO_RETURN [[noreturn]]
#endif

#endif	/* ZAIMONI_COMPILER_HPP */
