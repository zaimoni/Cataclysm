#include "json.h"
#include <type_traits>
#include <stdexcept>
#include <istream>
#include <sstream>

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

// unget is unreliable so relay the triggering char back instead
static unsigned char scan_for_data_start(std::istream& src, char& result, unsigned long& line)
{
	result = 0;
	bool last_was_cr = false;
	bool last_was_nl = false;
	while (!src.get(result).eof())
		{
		// ignore C-standard whitespace, but increment line # appropriately
		if ('\r' == result) {
			if (last_was_nl) {	// Windows/DOS, as viewed by archaic Mac
 				last_was_cr = false;
				last_was_nl = false;
				continue;
			}
			last_was_cr = true;
			++line;
			continue;
		} else if ('\n' == result) {
			if (last_was_cr) {	// Windows/DOS
				last_was_cr = false;
				last_was_nl = false;
				continue;
			}
			last_was_nl = true;
			++line;
			continue;
		} else {
		  last_was_cr = false;
		  last_was_nl = false;
		  if (strchr(" \t\f\v", result)) continue;	// ignore other C-standard whitespace
		}

		if ('[' == result) return JSON::array;
		if ('{' == result) return JSON::object;
		if ('"' == result) return JSON::string;
		// reject these
		if (strchr(",}]:", result)) return JSON::none;
		return JSON::literal;
		}
	return JSON::none;
}

JSON::JSON(std::istream& src)
: _mode(none), _scalar(0)
{
	src.exceptions(std::ios::failbit | std::ios::badbit);	// throw on hardware failure
	char last_read = ' ';
	unsigned long line = 1;
	const unsigned char code = scan_for_data_start(src,last_read,line);
	switch (code)
	{
	case object:
		finish_reading_object(src, line);
		return;
	case array:
		finish_reading_array(src, line);
		return;
	default:
		if (!src.eof() || !strchr(" \r\n\t\v\f", last_read)) {
			std::stringstream msg("JSON read failed before end of file, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		return;
	}
}

JSON::JSON(std::istream& src, unsigned long& line, char& last_read, bool must_be_scalar)
	: _mode(none), _scalar(0)
{
	const unsigned char code = scan_for_data_start(src, last_read, line);
	switch (code)
	{
	case object:
		if (must_be_scalar) return;	// object needs a string or literal as its key
		finish_reading_object(src, line);
		return;
	case array:
		if (must_be_scalar) return;	// object needs a string or literal as its key
		finish_reading_array(src, line);
		return;
	case string:
		finish_reading_string(src, line,last_read);
		return;
	case literal:
		finish_reading_literal(src, line,last_read);
		return;
	default:
		if (!src.eof() || !strchr(" \r\n\t\v\f", last_read)) {
			std::stringstream msg("JSON read failed before end of file, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		return;
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

void JSON::finish_reading_object(std::istream& src, unsigned long& line)
{
	std::map<std::string, JSON, str_compare> dest;
	char _last;
	do {
		JSON _key(src, line, _last, true);
		if (none == _key.mode()) {	// no valid data
			if ('}' == _last) {	// object terminated legally
				_mode = object;
				_object = dest.empty() ? 0 : new std::map<std::string, JSON, str_compare>(std::move(dest));
				return;
			}
			std::stringstream msg("JSON read of object failed before end of file, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		if (src.eof()) {	// oops, at end prematurely
			std::stringstream msg("JSON read of object truncated, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		{	// acceptable characters are :
			do {
				src.get(_last);
				if (strchr(" \r\n\t\v\f", _last)) {
					if (src.eof()) {
						std::stringstream msg("JSON read of object truncated, line: ");
						msg << line;
						throw std::runtime_error(msg.str());
					}
					continue;
				}
				if (':' == _last) break;
				std::stringstream msg("JSON read of object failed before end of file, line: ");
				msg << line;
				throw std::runtime_error(msg.str());
			} while (true);
		}
		JSON _value(src, line, _last);
		if (none == _value.mode()) {	// no valid data
			std::stringstream msg("JSON read of object failed before end of file, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		dest[std::move(_key.scalar())] = std::move(_value);
		{	// acceptable characters are , {
			do {
				src.get(_last);
				if (strchr(" \r\n\t\v\f", _last)) {
					if (src.eof()) {
						std::stringstream msg("JSON read of object truncated, line: ");
						msg << line;
						throw std::runtime_error(msg.str());
					}
					continue;
				}
				if ('}' == _last) {
					_mode = object;
					_object = dest.empty() ? 0 : new std::map<std::string, JSON, str_compare>(std::move(dest));
					return;
				}
				if (',' == _last) break;
				std::stringstream msg("JSON read of object failed before end of file, line: ");
				msg << line;
				throw std::runtime_error(msg.str());
			} while (true);
		}
	} while (true);
}

void JSON::finish_reading_array(std::istream& src, unsigned long& line)
{
	std::vector<JSON> dest;
	char _last = ' ';
	do {
		{
		JSON _next(src, line, _last);
		if (none == _next.mode()) {	// no valid data
			if (']' == _last) {	// array terminated legally
				_mode = array;
				_array = dest.empty() ? 0 : new std::vector<JSON>(std::move(dest));
				return;
			}
			std::stringstream msg(src.eof() ? "JSON read of array truncated, line: " : "JSON read of array failed before end of file, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		const bool was_literal = (literal == _next.mode());
		// something valid here
		dest.push_back(std::move(_next));
		if (was_literal) {
			if (']' == _last) {	// array terminated legally
				_mode = array;
				_array = dest.empty() ? 0 : new std::vector<JSON>(std::move(dest));
				return;
			}
			if (',' == _last) continue;	// another item expected afterwards
			if (!strchr(" \r\n\t\v\f",_last)) {
				std::stringstream msg("JSON read of array failed before end of file, line: ");
				msg << line;
				throw std::runtime_error(msg.str());
			}
		}
		}
		if (src.eof()) {	// oops, at end prematurely
			std::stringstream msg("JSON read of array truncated, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		}
		{	// acceptable characters are , ]
		do {
			src.get(_last);
			if (strchr(" \r\n\t\v\f", _last)) {
				if (src.eof()) {
					std::stringstream msg("JSON read of array truncated, line: ");
					msg << line;
					throw std::runtime_error(msg.str());
				}
				continue;
			}
			if (']' == _last) {
				_mode = array;
				_array = dest.empty() ? 0 : new std::vector<JSON>(std::move(dest));
				return;
			}
			if (',' == _last) break;
			std::stringstream msg("JSON read of array failed before end of file, line: ");
			msg << line;
			throw std::runtime_error(msg.str());
		} while (true);
		}
	} while (true);
}

void JSON::finish_reading_string(std::istream& src, unsigned long& line, char& first)
{
	std::string dest;
	bool in_escape = false;
	dest += first;

	do {
		src.get(first);
		if (in_escape) {
			switch(first)
			{
			case 'r':
				first = '\r';
				break;
			case 'n':
				first = '\r';
				break;
			case 't':
				first = '\t';
				break;
			case 'v':
				first = '\f';
				break;
			case 'f':
				first = '\f';
				break;
			case 'b':
				first = '\b';
				break;
			case '"':
				first = '"';
				break;
			case '\'':
				first = '\'';
				break;
			case '\\':
				first = '\\';
				break;
			// XXX would like to handle UNICODE to UTF8
			default:
				dest += '\\';
				break;
			}
			dest += first;
			in_escape = false;
			continue;
		} else if ('\\' == first) {
			in_escape = true;
			continue;
		} else if ('"' == first) {
			break;
		}
		dest += first;
	} while (!src.eof());
	// done
	_mode = string;
	_scalar = new std::string(std::move(dest));
}

void JSON::finish_reading_literal(std::istream& src, unsigned long& line, char& first)
{
	std::string dest;
	dest += first;

	do {
		src.get(first);
		if (strchr(" \r\n\t\v\f{}[],:\"", first)) break;	// done
		dest += first;
	} while(!src.eof());
	// done
	_mode = literal;
	_scalar = new std::string(std::move(dest));
}

}