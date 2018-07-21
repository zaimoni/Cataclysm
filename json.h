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

// function object
struct str_compare
{
	int operator()(const char* lhs, const char* rhs) const { return strcmp(lhs, rhs); }
	int operator()(const std::string& lhs, const std::string& rhs) const { return strcmp(lhs.c_str(), rhs.c_str()); }
};


class JSON
{
public:
	static std::map<std::string, JSON, str_compare> cache;

	enum mode : unsigned char {
		none = 0,
		object,
		array,
		string,
		literal
	};

private:
	union {
		std::string* _scalar;
		std::vector<JSON>* _array;
		std::map<std::string,JSON, str_compare> * _object;
	};
	unsigned char _mode;
public:
	JSON() : _mode(none), _scalar(0) {}
	JSON(const JSON& src);
	JSON(JSON&& src);
	JSON(std::istream& src);
	~JSON() { reset(); };

	JSON& operator=(const JSON& src);
	JSON& operator=(JSON&& src);

	unsigned char mode() const { return _mode; };
	void reset();
	bool empty() const;

	std::string scalar() const { return _mode <= string ? *_scalar : std::string(); }
	bool is_scalar() const { return string <= _mode; }
	bool destructive_grep(bool (ok)(const JSON&));	// 2018-07-21: not only do not need to allow for function objects, they converted compile-time errors to run-time errors.

	bool syntax_ok() const;
protected:
	JSON(std::istream& src,unsigned long& line, char& first, bool must_be_scalar = false);
private:
	void finish_reading_object(std::istream& src, unsigned long& line);
	void finish_reading_array(std::istream& src, unsigned long& line);
	void finish_reading_string(std::istream& src, unsigned long& line, char& first);
	void finish_reading_literal(std::istream& src, unsigned long& line, char& first);
};

}


#endif