#ifndef HTML_HPP
#define HTML_HPP 1

#include <string>
#include <vector>
#include <map>
#include <stdio.h>

namespace html {

class tag final {
private:
	std::string _name;	// if this is empty, just a container for content
	std::string _text;
	std::vector<tag> _content;
	std::map<std::string, std::string> _attr;
public:
	tag() = default;
	tag(const tag& src) = default;
	tag(tag&& src) = default;
	~tag() = default;
	tag& operator=(const tag& src) = default;
	tag& operator=(tag&& src) = default;

	// non-default construction options
	// no-content tag
	tag(const char* name) : _name(name) {}
	tag(const std::string& name) : _name(name) {}
	tag(std::string&& name) : _name(std::move(name)) {}

	// no-tag string (can't use constructor idiom as that's for a no-content tag)
	static tag wrap(const std::string& src) {
		tag ret;
		ret._text = src;
		return ret;
	}

	static tag wrap(std::string&& src) {
		tag ret;
		ret._text = std::move(src);
		return ret;
	}

	// tag with string content
	tag(const char* name, const char* text) : _name(name), _text(text) {}
	tag(const std::string& name, const char* text) : _name(name), _text(text) {}
	tag(std::string&& name, const char* text) : _name(std::move(name)), _text(text) {}
	tag(const char* name, const std::string& text) : _name(name),_text(text) {}
	tag(const std::string& name, const std::string& text) : _name(name), _text(text) {}
	tag(std::string&& name, const std::string& text) : _name(std::move(name)), _text(text) {}
	tag(const char* name, std::string&& text) : _name(name), _text(std::move(text)) {}
	tag(const std::string& name, std::string&& text) : _name(name), _text(std::move(text)) {}
	tag(std::string&& name, std::string&& text) : _name(std::move(name)), _text(std::move(text)) {}

	// \todo name accessor

	// content manipulation
	void append(const tag& src) { _content.push_back(src); }	// \todo fix these to account for text/content interactions
	void append(tag&& src) { _content.push_back(std::move(src)); }
	void append(const std::string& src) { _content.push_back(wrap(src)); }
	void append(std::string&& src) { _content.push_back(wrap(std::move(src))); }
	void clear() {
		decltype(_content) discard;
		_content.swap(discard);
		decltype(_text) discard2;
		_text.swap(discard2);
	}
	tag* operator[](size_t n) { return _content.size() > n ? _content.data() + n : 0; }

	// attribute manipulation
	void unset(const std::string& key) { _attr.erase(key); }
	void set(const std::string& key, const std::string& val) {
		std::string x(val);
		_attr[key] = std::move(destructive_quot_escape(x));
	}
	void set(const std::string& key, std::string&& val) { _attr[key] = std::move(destructive_quot_escape(val)); }
	void set(std::string&& key, const std::string& val) {
		std::string x(val);
		_attr[std::move(key)] = destructive_quot_escape(x);
	}
	void set(std::string&& key, std::string&& val) { _attr[std::move(key)] = std::move(destructive_quot_escape(val)); }
	const std::string* read(const std::string& key) const { return _attr.count(key) ? &_attr.at(key) : 0; }

	// W3C API
	tag* querySelector(const std::string& selector);	// returns first matching tag

	// to normal text
	std::string to_s() const;
	std::string to_s_start() const;
	std::string to_s_content() const;
	std::string to_s_end() const;

	// infrastructure
	static std::string& destructive_quot_escape(std::string& x);	// returns reference to x
private:
	tag* _querySelector_tagname(const std::string& tagname);
	tag* _querySelector_attr_val(const std::string& key, const std::string& val);
};

class to_text final
{
private:
	std::vector<tag> _stack;
	FILE* dest;
public:
	to_text(FILE* src) : dest(src) {};
	to_text(const to_text& src) = delete;
	to_text(to_text&& src) = delete;
	~to_text() {
		if (dest) fclose(dest);
		dest = 0;
	};
	to_text& operator=(const to_text& src) = delete;
	to_text& operator=(to_text&& src) = delete;

	void print(const tag& src);
	void start_print(const tag& src);
	void start_print(tag&& src);
	bool end_print(); // returns true iff no more tags to complete printing
};

}	// namespace html

#endif