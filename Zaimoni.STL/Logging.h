/* Logging.h */
/* Unified error logging */
/* (C)2009,2016,2018 Kenneth Boyd, license: MIT.txt */

#ifndef ZAIMONI_LOGGING_H
#define ZAIMONI_LOGGING_H

#include "Compiler.h"
#include <string.h>		/* need strlen here */

/* deal with assert */
/* note that including standard library assert.h/cassert will cause assert to be reserved (oops) */
/* but as long as we don't do that, this is legal under both C99 and C++98 */
#undef assert
#ifdef NDEBUG
#	define assert(A)	((void)0)
#	define ZAIMONI_PASSTHROUGH_ASSERT(A)	A
#else
/* Interoperate with Microsoft: return code 3 */
#	define assert(A)	((A) ? (void)0 : _fatal_code(__FILE__ ":" DEEP_STRINGIZE(__LINE__) ": " #A,3))
#	define ZAIMONI_PASSTHROUGH_ASSERT(A)	((A) ? (void)0 : FATAL_CODE(#A,3))
#endif

/*!
 * reports error, then calls exit(EXIT_FAILURE).
 *
 * \param B C string containing fatal error message
 */
MS_NO_RETURN EXTERN_C void _fatal(const char* const B) NO_RETURN;

/*!
 * reports error, then calls exit(exit_code).
 *
 * \param B C string containing fatal error message
 */
MS_NO_RETURN EXTERN_C void _fatal_code(const char* const B,int exit_code) NO_RETURN;

EXTERN_C void _inform(const char* const B, size_t len);		/* Windows GUI crippled (assumes len := strlen() */
EXTERN_C void _inc_inform(const char* const B, size_t len);	/* only C stdio */
EXTERN_C void _log(const char* const B, size_t len);		/* Windows GUI crippled (assumes len := strlen() */

#define INC_INFORM_STRING_LITERAL(B) _inc_inform(B,sizeof(B)-1)
#define INFORM_STRING_LITERAL(B) _inform(B,sizeof(B)-1)
#define LOG_STRING_LITERAL(B) _log(B,sizeof(B)-1)

#ifdef __cplusplus
/* overloadable adapters for C++ and debug-mode code */
/* all-uppercased because we may use macro wrappers on these */
MS_NO_RETURN void FATAL(const char* const B) NO_RETURN;
inline void FATAL(const char* const B) {_fatal(B);}

MS_NO_RETURN void FATAL_CODE(const char* const B,int exit_code) NO_RETURN;
inline void FATAL_CODE(const char* const B,int exit_code) {_fatal_code(B,exit_code);}

/* SEVERE_WARNING, WARNING, INFORM, and LOG are distinct in behavior only for the custom console */
inline void INC_INFORM(const char* const B) {_inc_inform(B,strlen(B));}
inline void INFORM(const char* const B) {_inform(B,strlen(B));}
inline void LOG(const char* const B) {_log(B,strlen(B));}

/* have C++, so function overloading */
inline void INFORM(const char* const B, size_t len) {_inform(B,len);}
inline void INC_INFORM(const char* const B, size_t len) {_inc_inform(B,len);}
inline void LOG(const char* const B, size_t len) {_log(B,len);}

/* char shorthands */
inline void INFORM(char B) {_inform(&B,1);}
inline void INC_INFORM(char B) {_inc_inform(&B,1);}
inline void LOG(char B) {_log(&B,1);}

/* signed integer shorthands */
void INFORM(intmax_t B);
void LOG(intmax_t B);
void INC_INFORM(intmax_t B);

/* unsigned integer shorthands */
void INFORM(uintmax_t B);
void LOG(uintmax_t B);
void INC_INFORM(uintmax_t B);

/* long double shorthands */
void INFORM(long double B);
void LOG(long double B);
void INC_INFORM(long double B);

#ifdef __GNUC__
inline void INFORM(unsigned long B) {return INFORM((uintmax_t)(B));}
inline void LOG(unsigned long B) {return LOG((uintmax_t)(B));}
inline void INC_INFORM(unsigned long B) {return INC_INFORM((uintmax_t)(B));}
#endif

#ifdef __GNUC__
inline void INFORM(unsigned int B) {return INFORM((uintmax_t)(B));}
inline void LOG(unsigned int B) {return LOG((uintmax_t)(B));}
inline void INC_INFORM(unsigned int B) {return INC_INFORM((uintmax_t)(B));}
#endif

#ifdef __GNUC__
inline void INFORM(int B) {return INFORM((intmax_t)(B));}
inline void LOG(int B) {return LOG((intmax_t)(B));}
inline void INC_INFORM(int B) {return INC_INFORM((intmax_t)(B));}
#endif

#ifdef __GNUC__
inline void INFORM(unsigned short B) {return INFORM((uintmax_t)(B));}
inline void LOG(unsigned short B) {return LOG((uintmax_t)(B));}
inline void INC_INFORM(unsigned short B) {return INC_INFORM((uintmax_t)(B));}
#endif

#ifdef __GNUC__
inline void INFORM(unsigned char B) {return INFORM((uintmax_t)(B));}
inline void LOG(unsigned char B) {return LOG((uintmax_t)(B));}
inline void INC_INFORM(unsigned char B) {return INC_INFORM((uintmax_t)(B));}
#endif

#else	/* !defined(__cplusplus) */
#ifdef NDEBUG
#	define FATAL(B) _fatal(B)
#	define FATAL_CODE(B,CODE) _fatal_code(B,CODE)
#else
#   define FATAL(B) _fatal((_LOG(ZAIMONI_FUNCNAME),LOG_STRING_LITERAL(__FILE__ ":" DEEP_STRINGIZE(__LINE__)),B))
#   define FATAL_CODE(B,CODE) _fatal_code((LOG(ZAIMONI_FUNCNAME),LOG_STRING_LITERAL(__FILE__ ":" DEEP_STRINGIZE(__LINE__)),B),CODE)
#endif
#define INC_INFORM(B) _inc_inform(B,strlen(B))
#define INFORM(B) _inform(B,strlen(B))
#define LOG(B) _log(B,strlen(B))
#endif

/* this should look like an assert even in NDEBUG mode */
#define SUCCEED_OR_DIE(A) ((A) ? (void)0 : _fatal_code(__FILE__ ":" DEEP_STRINGIZE(__LINE__) ": " #A,3))

/* match assert.h C standard header, which uses NDEBUG */
#ifndef NDEBUG	/* self-aware version */
/* use similar extensions on other compilers */
#ifdef __cplusplus
#   define FATAL(B) FATAL((LOG(ZAIMONI_FUNCNAME),LOG_STRING_LITERAL(__FILE__ ":" DEEP_STRINGIZE(__LINE__)),B))
#   define FATAL_CODE(B,CODE) FATAL_CODE((LOG(ZAIMONI_FUNCNAME),LOG_STRING_LITERAL(__FILE__ ":" DEEP_STRINGIZE(__LINE__)),B),CODE)
#endif

#define ARG_TRACE_PARAMS , const char* const _file, long _line
#define ARG_TRACE_ARGS , __FILE__, __LINE__
#define ARG_TRACE_LOG INFORM_INC(_file); INFORM_INC_STRING_LITERAL(":"); INFORM(_line)

#define AUDIT_IF_RETURN(A,B) if (A) {LOG_STRING_LITERAL(#A "; returning " #B); return B;}
#define AUDIT_STATEMENT(A) {A;}
#define DEBUG_STATEMENT(A) A
/* Interoperate with Microsoft: return code 3 */
#define DEBUG_FAIL_OR_LEAVE(A,B) if (A) FATAL_CODE(#A,3)

#define VERIFY(A,B) if (A) FATAL((LOG_STRING_LITERAL(#A),B))
#define REPORT(A,B) if (A) INFORM((LOG_STRING_LITERAL(#A),B))
#define DEBUG_LOG(A) LOG(A)

#else	/* fast version */
#define ARG_TRACE_PARAMS
#define ARG_TRACE_ARGS
#define ARG_TRACE_LOG

#define AUDIT_IF_RETURN(A,B) if (A) {return B;}
#define AUDIT_STATEMENT(A) A
#define DEBUG_STATEMENT(A)
#define DEBUG_FAIL_OR_LEAVE(A,B) if (A) B

#define VERIFY(A,B) if (A) FATAL(B)
#define REPORT(A,B) if (A) INFORM(B)
#define DEBUG_LOG(A)
#endif

#endif /* ZAIMONI_LOGGING_H */
