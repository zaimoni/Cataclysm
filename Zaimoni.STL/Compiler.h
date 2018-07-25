/* Compiler.h */
/* cross-compiler compatiblity header */
/* (C)2009,2010 Kenneth Boyd, license: MIT.txt */

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

/* hooks for C/C++ adapter code */
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

/* hooks for compiler extensions
   CONST_C_FUNC: like PURE_C_FUNC, but also cannot access global memory (through pointers or global variables)
   PURE_C_FUNC: no effects except return value, return value depends only on parameters and global variables
   PTR_RESTRICT: C99 restrict semantics (pointer may be assumed not to be aliased) */
#define NO_RETURN
#define MS_NO_RETURN
#define CONST_C_FUNC
#define PURE_C_FUNC
#define ALL_PTR_NONNULL
#define PTR_NONNULL(A)
#define PTR_RESTRICT

/* GCC */
#ifdef __GNUC__
#define THROW_COMPETENT 1
#undef NO_RETURN
#undef CONST_C_FUNC
#undef PURE_C_FUNC
#undef ALL_PTR_NONNULL
#undef PTR_NONNULL
#undef PTR_RESTRICT
#define NO_RETURN __attribute__ ((noreturn))
#define CONST_C_FUNC __attribute__ ((const))
#define PURE_C_FUNC __attribute__ ((pure))
#define ALL_PTR_NONNULL __attribute__ ((nonnull))
#define PTR_NONNULL(A) __attribute__ ((nonnull A))
#define PTR_RESTRICT __restrict__
#endif
#ifdef _MSC_VER
#undef MS_NO_RETURN
#define MS_NO_RETURN __declspec(noreturn)
#endif

#endif	/* ZAIMONI_COMPILER_HPP */
