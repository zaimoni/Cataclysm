#ifndef JSON_H
#define JSON_H 1

#include <string.h>
#include <string>
#include <vector>
#include <map>

// Cf. www.json.org
// We're not reusing C:DDA's JSON support because of different implementation requirements.

// THe intent here is not a full JSON parser, but rather a loose parser that accepts more than the strict standard.

namespace cataclysm {

// function object
struct str_compare
{
	int operator()(const char* lhs, const char* rhs) const { return strcmp(lhs, rhs); }
};


class JSON
{
public:
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
	~JSON() { reset(); };

	JSON& operator=(const JSON& src);
	JSON& operator=(JSON&& src);

	void reset();

	bool syntax_ok() const;
};

}


#endif