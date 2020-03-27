#ifndef HTML_HPP
#define HTML_HPP 1

#include <string>
#include <vector>
#include <map>

namespace html {

class tag {
private:
	std::string _name;	// if this is empty, just a container for content
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
	tag(const char* name) : _name(name) {}
	tag(const std::string& name) : _name(name) {}
	tag(std::string&& name) : _name(std::move(name)) {}

	static tag wrap(const std::string& src) {
		tag ret;
		ret._content.push_back(src);
		return ret;
	}

	static tag wrap(std::string&& src) {
		tag ret;
		ret._content.push_back(std::move(src));
		return ret;
	}

	// \todo name accessor

	// content manipulation
	void append(const tag& src) { _content.push_back(src); }
	void append(tag&& src) { _content.push_back(std::move(src)); }
	void append(const std::string& src) { _content.push_back(wrap(src)); }
	void append(std::string&& src) { _content.push_back(wrap(std::move(src))); }

	// attribute manipulation
	void unset(const std::string& key) { _attr.erase(key); }
	void set(const std::string& key, std::string val) { _attr[key] = destructive_quot_escape(val); }
	void set(const std::string& key, std::string&& val) { _attr[key] = std::move(destructive_quot_escape(val)); }
	void set(std::string&& key, std::string val) { _attr[std::move(key)] = destructive_quot_escape(val); }
	void set(std::string&& key, std::string&& val) { _attr[std::move(key)] = std::move(destructive_quot_escape(val)); }

	// to normal text
	std::string to_s() const;
	std::string to_s_start() const;
	std::string to_s_content() const;
	std::string to_s_end() const;

	// infrastructure
	static std::string& destructive_quot_escape(std::string& x);
};

}	// namespace html

#endif
