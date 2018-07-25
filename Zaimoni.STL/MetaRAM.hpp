/* MetaRAM.hpp */
/* C++ interface to C++/C memory management libraries */
/* limited functionality in C */
/* (C)2009,2015,2018 Kenneth Boyd, license: MIT.txt */

#ifndef ZAIMONI_METARAM_HPP
#define ZAIMONI_METARAM_HPP 1

#include "flat_alg.h"
#include <string.h>
#include <stdlib.h>
#include <new>

/* strictly speaking this is a stdlib.h repair */
#ifndef MAX_PATH
#ifdef _MAX_PATH
#define MAX_PATH _MAX_PATH
#endif
#endif

/* ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS : code fragment suitable as a guard clause for non-ansi free/delete that can't handle NULL */
/* good alternative if needed: */
/* if (NULL!=A) */
#ifndef ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS
#define ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(A)
#define ZAIMONI_ANSI_DEALLOCATORS 1
#endif

#ifdef __cplusplus
#include "polymorphic.hpp"

namespace zaimoni	{
//! for suppressing typecast on realloc
template<typename T> inline
typename std::enable_if<std::is_trivially_default_constructible<T>::value && std::is_trivially_destructible<T>::value && std::is_trivially_copy_assignable<T>::value, T*>::type
REALLOC(T* Target, size_t newsize)
{
	return reinterpret_cast<T*>(realloc(Target,newsize));
}

// next three don't really provide new functionality, they're here to complete the semantics
// DELETE collides with winnt.h (included by windows.h on Win32 systems)
#ifndef DELETE
template<typename T> inline void
DELETE(T* Target)
{
	ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(Target) delete Target;
}
#endif

template<typename T> inline void
DELETEARRAY(T* Target)
{
	ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(Target) delete [] Target;
}

// has trivial destructor is required to free rather than delete
template<typename T>
inline typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
FREE(T* Target)
{
	ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(Target) free(Target);
}

template<typename T> inline void
DELETE_AND_NULL(T*& Target)
{
	ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(Target)
	{
	delete Target;
	Target = 0;
	}
}

template<typename T> inline void
DELETEARRAY_AND_NULL(T*& Target)
{
	ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(Target)
	{
	delete [] Target;
	Target = 0;
	}
}

// has trivial destructor is required to free rather than delete
template<typename T>
inline typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
FREE_AND_NULL(T*& Target)
{
	ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(Target)
	{
	free(Target);
	Target = 0;
	}
}

// indirection glue
// _copy_buffer competes with STL std::copy
template<typename T,typename U>
typename std::enable_if<!is_polymorphic_base<U>::value, void>::type
_copy_buffer(U* dest, const T* src, size_t Idx)
{
	do	{
		--Idx;
		dest[Idx] = src[Idx];
		}
	while(0<Idx);
}

template<typename T>
typename std::enable_if<!std::is_trivially_copy_assignable<T>::value, void>::type
_copy_buffer(T* dest, const T* src, size_t Idx)
{
	do	{
		--Idx;
		dest[Idx] = src[Idx];
		}
	while(0<Idx);
}

template<typename T>
inline typename std::enable_if<std::is_trivially_copy_assignable<T>::value, void>::type
_copy_buffer(T* dest, const T* src, size_t Idx)
{
	memmove(dest,src,Idx*sizeof(T));
}

// for resize
template<typename T>
typename std::enable_if<has_MoveInto<T>::value, void>::type
_copy_expendable_buffer(T* dest, T* src, size_t Idx)
{
	do	{
		--Idx;
		src[Idx].MoveInto(dest[Idx]);
		}
	while(0<Idx);
}

template<typename T>
inline typename std::enable_if<!has_MoveInto<T>::value, void>::type
_copy_expendable_buffer(T* dest, const T* src, size_t Idx)
{	_copy_buffer(dest,src,Idx);	}

template<typename T>
void
_copy_expendable_buffer(T** dest, const T** src, size_t Idx)
{	_copy_buffer(dest,src,Idx);	}

// _vector_assign competes with std::fill_n
// should be preferred to the _elementwise_op approach in algor.hpp
template<typename T,typename U>
typename std::enable_if<is_polymorphic_base<U>::value, void>::type
_vector_assign(U* dest, const T& src, size_t Idx)
{
	do	dest[--Idx] = src;
	while(0<Idx);
}

template<typename T>
typename std::enable_if<!(std::is_trivially_copy_assignable<T>::value && 1==sizeof(T)), void>::type
_vector_assign(T* dest,typename zaimoni::param<T>::type src, size_t Idx)
{
	do	dest[--Idx] = src;
	while(0<Idx);
}

template<typename T>
inline typename std::enable_if<std::is_trivially_copy_assignable<T>::value && 1==sizeof(T), void>::type
_vector_assign(T* dest,typename zaimoni::param<T>::type src, size_t Idx)
{
	memset(dest,src,Idx);
}

template<typename T>
inline bool
_value_equal(const T& lhs, const T& rhs)
{
	return lhs==rhs;
}

template<typename T>
bool
_value_equal(const T* lhs, const T* rhs)
{
	if (!lhs) return !rhs;
	if (!lhs) return false;
	return _value_equal(*lhs,*rhs);
}

template<typename T>
bool
_value_vector_equal(const T& lhs, const T& rhs, size_t Idx)
{
	while(0<Idx)
		{
		--Idx;
		if (!_value_equal(lhs[Idx],rhs[Idx])) return false;
		};
	return true;
}

// _vector_equal competes with std::equal
template<typename T,typename U>
bool
_vector_equal(const T& lhs, const U& rhs, size_t Idx)
{
	while(0<Idx)
		{
		--Idx;
		if (lhs[Idx]!=rhs[Idx]) return false;
		};
	return true;
}

// objects
template<typename T>
inline typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
_single_flush(T* _ptr)
{
	delete _ptr;
}

// pointer arrays
template<typename T>
inline void
_single_flush(T** _ptr)
{
	single_flush(*_ptr);
	free(_ptr);
}

template<typename T>
inline typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
_single_flush(T* _ptr)
{
	free(_ptr);
}

template<typename T>
inline void
_weak_flush(T** _ptr)
{
	free(_ptr);
}

// _new_buffer/_new_buffer_nonNULL_throws and _flush [MetaRAM2.hpp] have to be synchronized for ISO C++
// _new_buffer_nonNULL is in MetaRAM2.hpp, as it depends on Logging.h
template<typename T>
inline typename std::enable_if<!(std::is_trivially_default_constructible<T>::value && std::is_trivially_destructible<T>::value), T*>::type
_new_buffer(size_t Idx)
{
	return new(std::nothrow) T[Idx];
}

template<typename T>
inline typename std::enable_if<std::is_trivially_default_constructible<T>::value && std::is_trivially_destructible<T>::value, T*>::type
_new_buffer(size_t Idx)
{
	return reinterpret_cast<T*>(calloc(Idx,sizeof(T)));
}

template<typename T>
inline typename std::enable_if<!(std::is_trivially_default_constructible<T>::value && std::is_trivially_destructible<T>::value), T*>::type
_new_buffer_nonNULL_throws(size_t Idx)
{
	return new T[Idx];
}

template<typename T>
inline typename std::enable_if<std::is_trivially_default_constructible<T>::value && std::is_trivially_destructible<T>::value, T*>::type
_new_buffer_nonNULL_throws(size_t Idx)
{
	T* tmp = reinterpret_cast<T*>(calloc(Idx,sizeof(T)));
	if (!tmp) throw std::bad_alloc();
	return tmp;
}

}	// end namespace zaimoni

#else	/* not in C++ */

#ifdef ZAIMONI_ANSI_DEALLOCATORS
#define FREE(T) free(T)
#define FREE_AND_NULL(T) {free(T); T=NULL;}
#else
#define FREE(T) {ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(T) free(T);}
#define FREE_AND_NULL(T) {ZAIMONI_PROTECT_NONANSI_NONNULL_CHECKING_DEALLOCATORS(T) {free(T); T=NULL;};}
#endif

#endif	/* not in C++ */

#ifdef __cplusplus
namespace zaimoni {
#endif

/* other functions */
inline void
copy_c_string(char*& dest, const char* src)
{
	if (is_empty_string(src))
		FREE_AND_NULL(dest);
	else{
		char* Tmp2 = REALLOC(dest,strlen(src)+1);
		if (Tmp2)
			{
			strcpy(Tmp2,src);
			dest = Tmp2;
			}
		else{
			FREE_AND_NULL(dest);
			}
		}
}

#ifdef __cplusplus
}
#endif

#endif
