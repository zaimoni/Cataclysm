#include "file.h"

namespace cataclysm {

#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
bool OS_dir::exists(const char* file) {
	close();
	search_handle = _findfirst(file, &file_found);
	_err = (-1 == search_handle) ? errno : 0;
	if (-1 == search_handle && ENOENT != _err) throw _err;	// yes, throws int
	return -1 != search_handle;
}

bool OS_dir::force_dir(const char* file)
{
	if (exists(file)) return (_A_SUBDIR | file_found.attrib);
	if (0 > mkdir(file)) return false;
	if (exists(file)) return (_A_SUBDIR | file_found.attrib);
	return false;
}


void OS_dir::get_filenames(std::vector<std::string>& dest)
{
	if (-1 == search_handle) return;
	dest.push_back(file_found.name);
	while (0 <= _findnext(search_handle, &file_found)) dest.push_back(file_found.name);
	_err = errno;
	if (ENOENT != _err) throw _err;
	close();
}
#else
#endif

}