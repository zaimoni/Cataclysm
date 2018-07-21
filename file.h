#ifndef FILE_H
#define FILE_H 1

#include <errno.h>
#include "Zaimoni.STL/Pure.C/comptest.h"	/* for C library test results */

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include <string>
#include <vector>

namespace cataclysm {

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
class OS_dir
{
private:
	_finddata_t file_found;
	intptr_t search_handle;
	int _err;
public:
	OS_dir() : search_handle(-1),_err(0) {};
	OS_dir(const OS_dir& src) = delete;
	~OS_dir() { close(); }
	OS_dir& operator=(const OS_dir& src) = delete;

	// these three throw the errno value on critical failure; catch(int) for it
	bool exists(const char* file);
	bool force_dir(const char* file);
	void get_filenames(std::vector<std::string>& dest);

	void close() { if (-1 != search_handle) { _findclose(search_handle); search_handle = -1; }; }
};

// signal that we have the class
#define OS_dir OS_dir
#else
#endif

}

#endif