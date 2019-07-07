#include "json.h"
#include <type_traits>
#include <stdexcept>
#include <istream>
#include <sstream>

namespace cataclysm {

std::map<const std::string, JSON> JSON::cache;

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

bool JSON::empty() const
{
	switch (_mode)
	{
	case object: return !_object;
	case array: return !_array;
	case string:
	case literal: return !_scalar;
	default: return true;	// invalid, so no useful data anyway
	}
}

size_t JSON::size() const
{
	switch (_mode)
	{
	case object: return _object ? _object->size() : 0;
	case array: return _array ? _array->size() : 0;
	case string:
	case literal: return _scalar ? 1 : 0;
	default: return 0;	// invalid, so no useful data anyway
	}
}

void JSON::push(const JSON& src)
{
	if (array != _mode) {
		reset();
		_mode = array;
	}
	if (!_array) _array = new std::vector<JSON>();
	_array->push_back(src);
}

void JSON::push(JSON&& src)
{
	if (array != _mode) {
		reset();
		_mode = array;
	}
	if (!_array) _array = new std::vector<JSON>();
	_array->push_back(src);
}

JSON JSON::grep(bool (ok)(const JSON&)) const
{
	JSON ret;
	if (empty()) return ret;

	switch (_mode)
	{
	case string:
	case literal: 
		if (ok(*this)) ret = *this;
		return ret;
	case object:
		{
		_object_JSON conserve;
		for (const auto& tmp : *_object) {
			if (ok(tmp.second)) conserve[tmp.first] = tmp.second;
		}
		if (conserve.empty()) return ret;
		ret._mode = object;
		ret._object = new _object_JSON(std::move(conserve));
		}
		return ret;
	case array:
		{
		std::vector<JSON> conserve;
		for (const auto& tmp : *_array) {
			if (ok(tmp)) conserve.push_back(tmp);
		}
		if (conserve.empty()) return ret;
		ret._mode = array;
		ret._array = new std::vector<JSON>(std::move(conserve));
		}
		return ret;
	}
	return ret;	// formal fall-through
}

bool JSON::destructive_grep(bool (ok)(const JSON&))
{
	switch (_mode)
	{
	case string:
	case literal: return ok(*const_cast<const JSON*>(this));
	case object:
		if (empty()) return false;
		{
			std::vector<std::string> doomed;
			for (const auto& tmp : *_object) {
				if (!ok(tmp.second)) doomed.push_back(tmp.first);
			}
			for (const auto& tmp : doomed) _object->erase(tmp);
		}
		if (!_object->empty()) return true;
		delete _object;
		_object = 0;
		return false;
	case array:
		if (empty()) return false;
		{
			size_t i = _array->size();
			do {
				--i;
				if (!ok((*_array)[i])) _array->erase(_array->begin() + i);
			} while (0 < i);
		}
		if (!_array->empty()) return true;
		delete _array;
		_array = 0;
		return false;
	}
	return !empty();
}

bool JSON::destructive_grep(bool (ok)(const std::string& key, const JSON&), bool (postprocess)(const std::string& key, JSON&))
{
	if (object != _mode) return false;
	if (!_object) return true;

	// assume we have RAM, etc.
	std::vector<std::string> keys;
	for (const auto& iter : *_object) {
		if (!ok(iter.first,iter.second)) keys.push_back(iter.first);
	}
	for (const auto& key : keys) {
		if (!postprocess(key, (*_object)[key])) _object->erase(key);
	}
	if (_object->empty()) {
		delete _object;
		_object = 0;
	}
	return true;
}

bool JSON::destructive_merge(JSON& src)
{
	if (object != src._mode) return false;
	if (none == _mode) {	// we are blank.  retype as empty object.
		_mode = object;
		_object = 0;	// leak rather than crash
	}
	if (object != _mode) return false;
	if (!src._object) return true;	// no keys
	_object_JSON* working = (_object ? _object : new _object_JSON());
	// assume we have RAM, etc.
#if OBSOLETE
	std::vector<std::string> keys = src.keys();
	for (const auto& key : keys) {
		(*working)[key] = std::move((*src._object)[key]);
		src._object->erase(key);
	}
#else
	for (auto& iter : *src._object) {
		(*working)[iter.first] = std::move(iter.second);
	}
	delete src._object;
	src._object = 0;
#endif
	if (!working->empty()) _object = working;
	return true;
}

bool JSON::destructive_merge(JSON& src, bool (ok)(const JSON&))
{
	if (object != src._mode) return false;
	if (none == _mode) {	// we are blank.  retype as empty object.
		_mode = object;
		_object = 0;	// leak rather than crash
	}
	if (object != _mode) return false;
	if (!src._object) return true;	// no keys
	_object_JSON* working = (_object ? _object : new _object_JSON());
	// assume we have RAM, etc.
	std::vector<std::string> keys;
	for (const auto& iter : *src._object) {
		if (ok(iter.second)) keys.push_back(iter.first);
	}
	for (const auto& key : keys) {
		(*working)[key] = std::move((*src._object)[key]);
		src._object->erase(key);
	}
	if (!working->empty()) _object = working;
	return true;
}

std::vector<std::string> JSON::keys() const
{
	std::vector<std::string> ret;
	if (object == _mode && _object) {
		for (const auto& iter : *_object) ret.push_back(iter.first);
	}
	return ret;
}

void JSON::unset(const std::vector<std::string>& src)
{
	if (object != _mode || !_object || src.empty()) return;
	for (const auto& key : src) {
		if (_object->count(key)) _object->erase(key);
	}
	if (_object->empty()) {
		delete _object;
		_object = 0;
	}
}

void JSON::unset(const std::string& src)
{
	if (object != _mode || !_object || src.empty()) return;
	if (_object->count(src)) _object->erase(src);
	if (_object->empty()) {
		delete _object;
		_object = 0;
	}
}

bool JSON::is_legal_JS_literal(const char* src)
{
	if (!src || !*src) return false;
	if (!strcmp(src, "true")) return true;
	if (!strcmp(src, "false")) return true;
	if (!strcmp(src, "null")) return true;
	// false-positive here...want to tolerate numerals
	if (strchr("+-0123456789", *src)) return true;
	return false;
}

void JSON::set(const std::string& src, const JSON& val)
{
	if (object != _mode) {
		reset();
		_mode = object;
	}
	if (!_object) _object = new _object_JSON();
	(*_object)[src] = val;
}

void JSON::set(const std::string& src, JSON&& val)
{
	if (object != _mode) {
		reset();
		_mode = object;
	}
	if (!_object) _object = new _object_JSON();
	(*_object)[src] = std::move(val);
}

// constructor and support thereof
JSON::JSON(const JSON& src)
: _mode(src._mode),_scalar(0)
{
	switch(src._mode)
	{
	case object:
		_object = src._object ? new _object_JSON(*src._object) : 0;
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

static bool consume_whitespace(std::istream& src,unsigned long& line)
{
	bool last_was_cr = false;
	bool last_was_nl = false;
	do {
	   int test = src.peek();
	   if (EOF == test) return false;
	   switch(test) {
		case '\r':
			if (last_was_nl) {	// Windows/DOS, as viewed by archaic Mac
				last_was_cr = false;
				last_was_nl = false;
				break;
			}
			last_was_cr = true;
			++line;
			break;
		case '\n':
			if (last_was_cr) {	// Windows/DOS
				last_was_cr = false;
				last_was_nl = false;
				break;
			}
			last_was_nl = true;
			++line;
			break;
		default:
		   last_was_cr = false;
		   last_was_nl = false;
		   if (!strchr(" \t\f\v", test)) return true;	// ignore other C-standard whitespace
		   break;
	   }
	   src.get();
	   }
	while(true);
}

// unget is unreliable so relay the triggering char back instead
static unsigned char scan_for_data_start(std::istream& src, char& result, unsigned long& line)
{
	if (!consume_whitespace(src,line)) return JSON::none;
	result = 0;
	bool last_was_cr = false;
	bool last_was_nl = false;
	while (!src.get(result).eof())
		{
		if ('[' == result) return JSON::array;
		if ('{' == result) return JSON::object;
		if ('"' == result) return JSON::string;
		// reject these
		if (strchr(",}]:", result)) return JSON::none;
		return JSON::literal;
		}
	return JSON::none;
}

static const std::string JSON_read_failed("JSON read failed before end of file, line: ");

JSON::JSON(std::istream& src)
: _mode(none), _scalar(0)
{
	src.exceptions(std::ios::badbit);	// throw on hardware failure
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
			std::stringstream msg;
			msg << JSON_read_failed << line << '\n';
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
			std::stringstream msg;
			msg << JSON_read_failed << line << '\n';
			throw std::runtime_error(msg.str());
		}
		return;
	}
}


JSON::JSON(JSON&& src)
{
	static_assert(std::is_standard_layout<JSON>::value, "JSON move constructor is invalid");
	memmove(this, &src, sizeof(JSON));
	memset(&src, 0, sizeof(JSON));
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
	memset(&src, 0, sizeof(JSON));
	return *this;
}

static bool next_is(std::istream& src, char test)
{
	if (src.peek() == test)
		{
		src.get();
		return true;
		}
	return false;
}

static const std::string  JSON_object_read_failed("JSON read of object failed before end of file, line: ");
static const std::string  JSON_object_read_truncated("JSON read of object truncated, line: ");

void JSON::finish_reading_object(std::istream& src, unsigned long& line)
{
	if (!consume_whitespace(src, line))
		{
		std::stringstream msg;
		msg << JSON_object_read_failed << line << '\n';
		throw std::runtime_error(msg.str());
		}
	if (next_is(src,'}')) {
		_mode = object;
		_object = 0;
		return;
	}

	_object_JSON dest;
	char _last;
	do {
		JSON _key(src, line, _last, true);
		if (none == _key.mode()) {	// no valid data
			std::stringstream msg;
			msg << JSON_object_read_failed << line << '\n';
			throw std::runtime_error(msg.str());
		}
		if (!consume_whitespace(src, line)) {	// oops, at end prematurely
			std::stringstream msg;
			msg << JSON_object_read_truncated << line << '\n';
			throw std::runtime_error(msg.str());
		}
		if (!next_is(src, ':')) {
			std::stringstream msg;
			msg << "JSON read of object failed, expected : got '" << (char)src.peek() << "' code point " << src.peek() << ", line: " << line << '\n';
			throw std::runtime_error(msg.str());
		}
		if (!consume_whitespace(src, line)) {	// oops, at end prematurely
			std::stringstream msg;
			msg << JSON_object_read_truncated << line << '\n';
			throw std::runtime_error(msg.str());
		}
		JSON _value(src, line, _last);
		if (none == _value.mode()) {	// no valid data
			std::stringstream msg;
			msg << JSON_object_read_failed << line << '\n';
			throw std::runtime_error(msg.str());
		}
		dest[std::move(_key.scalar())] = std::move(_value);
		if (!consume_whitespace(src, line)) {	// oops, at end prematurely (but everything that did arrive is ok)
			_mode = object;
			_object = dest.empty() ? 0 : new _object_JSON(std::move(dest));
			return;
		}
		if (next_is(src, '}')) {
			_mode = object;
			_object = dest.empty() ? 0 : new _object_JSON(std::move(dest));
			return;
		}
		if (!next_is(src, ',')) {
			std::stringstream msg;
			msg << "JSON read of object failed, expected , or }, line: " << line << '\n';
			throw std::runtime_error(msg.str());
		}
	} while (true);
}

static const std::string JSON_array_read_failed("JSON read of array failed before end of file, line: ");

void JSON::finish_reading_array(std::istream& src, unsigned long& line)
{
	if (!consume_whitespace(src, line))
	{
		std::stringstream msg;
		msg << JSON_array_read_failed << line << '\n';
		throw std::runtime_error(msg.str());
	}
	if (next_is(src, ']')) {
		_mode = array;
		_array = 0;
		return;
	}

	std::vector<JSON> dest;
	char _last = ' ';
	do {
		{
		JSON _next(src, line, _last);
		if (none == _next.mode()) {	// no valid data
			std::stringstream msg;
			msg << JSON_array_read_failed << line << '\n';
			throw std::runtime_error(msg.str());
		}
		dest.push_back(std::move(_next));
		}
		if (!consume_whitespace(src, line)) {	// early end but data so far ok
			_mode = array;
			_array = dest.empty() ? 0 : new std::vector<JSON>(std::move(dest));
			return;
		}
		if (next_is(src, ']')) {	// array terminated legally{
			_mode = array;
			_array = dest.empty() ? 0 : new std::vector<JSON>(std::move(dest));
			return;
		}
		if (!next_is(src, ',')) {
			std::stringstream msg;
			msg << "JSON read of array failed, expected , or ], line: " << line << '\n';
			throw std::runtime_error(msg.str());
		}
	} while (true);
}

void JSON::finish_reading_string(std::istream& src, unsigned long& line, char& first)
{
	std::string dest;
	bool in_escape = false;

	do {
		int test = src.peek();
		if (EOF == test) break;
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

static const char* reject_for_JSON_literal(char c)
{
	return strchr(" \r\n\t\v\f{}[],:\"", c);
}

void JSON::finish_reading_literal(std::istream& src, unsigned long& line, char& first)
{
	std::string dest;
	dest += first;

	do {
		int test = src.peek();
		if (EOF == test) break;
		if (reject_for_JSON_literal(test)) break;	// done
		src.get(first);
		dest += first;
	} while(!src.eof());
	// done
	_mode = literal;
	_scalar = new std::string(std::move(dest));
}

static const char* escape_for_JSON_string(char c)
{
	return strchr("\r\n\t\v\f\"\\", c);
}

static std::ostream& write_string(std::ostream& os, const std::string& src)
{
	os.put('"');
	for (const auto c : src) {
		if (auto reject = escape_for_JSON_string(c)) {
			os.put('\\');
			switch (*reject) {
			case '\r':
				os.put('r');
				break;
			case '\n':
				os.put('n');
				break;
			case '\t':
				os.put('t');
				break;
			case '\v':
				os.put('v');
				break;
			case '\f':
				os.put('f');
				break;
			case '"':
				os.put('"');
				break;
			case '\\':
				os.put('\\');
				break;
				// default: throw std::string(....);
			}
			continue;
		}
		os.put(c);
		continue;
	}
	os.put('"');
	return os;
}

static std::ostream& write_literal(std::ostream& os, const std::string& src)
{
	bool ok_as_literal = true;
	for (const auto c : src) if (reject_for_JSON_literal(c)) {
		ok_as_literal = false;
		break;
	}
	if (ok_as_literal) return os << src;
	return write_string(os, src);
}

std::ostream& JSON::write_array(std::ostream& os, const std::vector<JSON>& src, int indent)
{
	os.put('[');
	const auto ub = src.size();
	auto i = 0;
	while (i < ub) {
		src[i].write(os, indent+1);
		if (++i < ub) {
			os.put(',');
			os << std::endl;
			int _indent = indent;
			while(0 < _indent--) os.put('\t');
		}
	}
	os.put(']');
	return os;
}

std::ostream& JSON::write_object(std::ostream& os, const _object_JSON& src, int indent)
{
	os.put('{');
	const auto ub = src.size();
	auto i = 0;
	for (const auto x : src) {
		write_literal(os, x.first);
		os.put(':');
		x.second.write(os, indent + 1);
		if (++i < ub) {
			os.put(',');
			os << std::endl;
			int _indent = indent;
			while (0 < _indent--) os.put('\t');
		}
	}
	os.put('}');
	return os;
}

std::ostream& JSON::write(std::ostream& os, int indent) const
{
	switch (_mode)
	{
	case JSON::object: return write_object(os, *_object, indent);
	case JSON::array: return write_array(os, *_array, indent);
	case JSON::literal: return write_literal(os, *_scalar);
	case JSON::string: return write_string(os, *_scalar);
	// default: throw std::string(....);
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const JSON& src)
{
	switch (src._mode)
	{
	case JSON::object:
		if (!src._object) return os << "{}";
		return src.write_object(os, *src._object);
	case JSON::array:
		if (!src._array) return os << "[]";
		return src.write_array(os, *src._array);
	case JSON::literal:
		if (!src._scalar) return os << "\"\"";
		return write_literal(os, *src._scalar);
	case JSON::string:
		if (!src._scalar) return os << "\"\"";
		return write_string(os, *src._scalar);
		// default: throw std::string(....);
	}
	return os;
}

}	// namespace cataclysm

using cataclysm::JSON;

bool fromJSON(const JSON& src, std::string& dest)
{
	if (!src.is_scalar()) return false;
	dest = src.scalar();
	return true;
}

bool fromJSON(const JSON& src, int& dest)
{
	if (!src.is_scalar()) return false;
	dest = stoll(src.scalar());
	return true;
}

bool fromJSON(const JSON& src, unsigned int& dest)
{
	if (!src.is_scalar()) return false;
	dest = stoull(src.scalar());
	return true;
}

bool fromJSON(const JSON& src, char& dest)
{
	if (!src.is_scalar()) return false;
	dest = src.scalar()[0];
	return true;
}

bool fromJSON(const JSON& src, bool& dest)
{
	if (!src.is_scalar()) return false;
	const auto x(src.scalar());
	if ("false" == x) {
		dest = false;
		return true;
	}
	if ("true" == x) {
		dest = true;
		return true;
	}
	// there may be other logical aliases to consider, but we ignore them here
	return false;
}

cataclysm::JSON toJSON(int src) {
	return JSON(std::to_string(src));
}
