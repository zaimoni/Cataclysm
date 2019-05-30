#ifndef JSON_H
#define JSON_H 1

#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <iosfwd>

// Cf. www.json.org
// We're not reusing C:DDA's JSON support because of different implementation requirements.

// THe intent here is not a full JSON parser, but rather a loose parser that accepts more than the strict standard.

namespace cataclysm {

class JSON
{
public:
	static std::map<const std::string, JSON> cache;

	enum mode : unsigned char {
		none = 0,
		object,
		array,
		string,
		literal
	};

private:
	typedef std::map<const std::string, JSON> _object_JSON;

	union {
		std::string* _scalar;
		std::vector<JSON>* _array;
		_object_JSON* _object;
	};
	unsigned char _mode;
public:
	JSON() : _mode(none), _scalar(0) {}
	JSON(const JSON& src);
	JSON(JSON&& src);
	JSON(std::istream& src);
	JSON(const std::string& src) : _mode(literal), _scalar(new std::string(src)) {}
	JSON(std::string&& src) : _mode(literal), _scalar(new std::string(src)) {}
	JSON(const char* src,bool is_literal = true) : _mode(is_literal ? literal : string), _scalar(new std::string(src)) {}
	JSON(std::string*& src, bool is_literal = true) : _mode(is_literal ? literal : string), _scalar(src) { src = 0; }
	~JSON() { reset(); };
	friend std::ostream& operator<<(std::ostream& os, const JSON& src);

	JSON& operator=(const JSON& src);
	JSON& operator=(JSON&& src);

	unsigned char mode() const { return _mode; };
	void reset();
	bool empty() const;
	size_t size() const;

	// reserved literal values in strict JSON
	static bool is_null(const JSON& src) { return literal == src._mode && src._scalar && !strcmp("null", src._scalar->c_str()); };

	// scalar evaluation
	bool is_scalar() const { return string <= _mode; }
	std::string scalar() const { return string <= _mode ? *_scalar : std::string(); }
	// object evaluation
	bool has_key(const std::string& key) const { return object == _mode && _object && _object->count(key); }
	JSON& operator[](const std::string& key) { return (*_object)[key]; };
	const JSON& operator[](const std::string& key) const { return (*_object)[key]; };
	bool become_key(const std::string& key) {
		if (!has_key(key)) return false;
		JSON tmp(std::move((*_object)[key]));
		*this = std::move(tmp);
		return true;
	}
	bool extract_key(const std::string& key, JSON& dest) {
		if (!has_key(key)) return false;
		dest = std::move((*_object)[key]);
		unset(key);
		return true;
	}

	// use for testing values of array or object.
	// 2018-07-21: not only do not need to allow for function objects, they converted compile-time errors to run-time errors.
	JSON grep(bool (ok)(const JSON&)) const;	// Cf. Perl4+; evaluated in order for arrays
	bool destructive_grep(bool (ok)(const JSON&));	// evaluated in reverse order for arrays
	// key-value pair that is ok is not changed; a not-ok key-value pair that fails post-processing is deleted
	bool destructive_grep(bool (ok)(const std::string& key,const JSON&),bool (postprocess)(const std::string& key, JSON&));	// use for testing object
	bool destructive_merge(JSON& src);	// keys of src end up in ourselves, the destination.  Cf PHP3+
	bool destructive_merge(JSON& src, bool (ok)(const JSON&));	// keys of src end up in ourselves, the destination.  Cf PHP3+
	std::vector<std::string> keys() const;	// Cf. Perl
	void unset(const std::vector<std::string>& src);	// cf PHP 3+ -- clears keys from object
	void unset(const std::string& src);
	// workarounds for the array deference operator not auto-vivifying
	static bool is_legal_JS_literal(const char* src);
	void set(const std::string& src, const JSON& val);
	void set(const std::string& src, JSON&& val);
	void set(const std::string& src, const char* const val) { set(src, JSON(val, is_legal_JS_literal(val))); }

	bool syntax_ok() const;
protected:
	JSON(std::istream& src,unsigned long& line, char& first, bool must_be_scalar = false);
private:
	void finish_reading_object(std::istream& src, unsigned long& line);
	void finish_reading_array(std::istream& src, unsigned long& line);
	void finish_reading_string(std::istream& src, unsigned long& line, char& first);
	void finish_reading_literal(std::istream& src, unsigned long& line, char& first);

	static std::ostream& write_array(std::ostream& os, const std::vector<JSON>& src, int indent = 1);
	static std::ostream& write_object(std::ostream& os, const _object_JSON& src, int indent = 1);
	std::ostream& write(std::ostream& os, int indent) const;
};

}


#endif