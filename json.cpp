#include "json.h"
#include <type_traits>
#include <stdexcept>

namespace cataclysm {

bool JSON::syntax_ok() const
{
	switch(_mode)
	{
	case object:
		if (!_object) return true;
		for (const auto& it : *_object) if (!it.second.syntax_ok()) return false;
		return true;
	case array:
		if (!_array) return true;
		for (const auto& json : *_array) if (!json.syntax_ok()) return false;
		return true;
	case string:
		if (!_scalar) return false;
		return true;	// zero-length string ok
	case literal:
		if (!_scalar) return false;
		return !_scalar->empty();	// zero-length literal not ok (has to be at least one not-whitespace character to be seen
	default: return false;
	}
}

void JSON::reset()
{
	switch (_mode)
	{
	case object:
		if (_object) delete _object;
		break;
	case array:
		if (_array) delete _array;
		break;
	case string:
	case literal:
		if (_scalar) delete _scalar;
		break;
	// just leak if it's invalid
	}
	_mode = none;
	_scalar = 0;
}


JSON::JSON(const JSON& src)
: _mode(src._mode),_scalar(0)
{
	switch(src._mode)
	{
	case object:
		_object = src._object ? new std::map<std::string,JSON,str_compare>(*src._object) : 0;
		break;
	case array:
		_array = src._array ? new std::vector<JSON>(*src._array) : 0;
		break;
	case string:
	case literal:
		_scalar = src._scalar ? new std::string(*src._scalar) : 0;
		break;
	case none: break;
	default: throw std::runtime_error("invalid JSON src for copy");
	}
}

JSON::JSON(JSON&& src)
{
	static_assert(std::is_standard_layout<JSON>::value, "JSON move constructor is invalid");
	memmove(this, &src, sizeof(JSON));
	src._mode = none;
	src._scalar = 0;
}

JSON& JSON::operator=(const JSON& src)
{
	JSON tmp(src);
	return *this = std::move(tmp);
}

JSON& JSON::operator=(JSON&& src)
{
	static_assert(std::is_standard_layout<JSON>::value, "JSON move assignment is invalid");
	reset();
	memmove(this, &src, sizeof(JSON));
	src._mode = none;
	src._scalar = 0;
	return *this;
}

}