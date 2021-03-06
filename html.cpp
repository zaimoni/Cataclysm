#include "html.hpp"
#include <stdexcept>

namespace html {

// C++20: constexpr std::string constructors
static const std::string start_opening_tag("<");
static const std::string start_closing_tag("</");
static const std::string end_tag(">");
static const std::string space(" ");
static const std::string end_selfclose_tag(" />");	// browsers parse this as HTML or XHTML
static const std::string open_attr_val("=\"");
static const std::string close_attr_val("\"");
static const std::string quot_entity("&quot;");

// \todo fix these to account for text/content interactions
void tag::append(const tag& src)
{
	if (!_text.empty()) _content.push_back(wrap(std::move(_text)));
	_content.push_back(src);
}

void tag::append(tag&& src)
{
	if (!_text.empty()) _content.push_back(wrap(std::move(_text)));
	_content.push_back(std::move(src));
}

void tag::append(const std::string& src)
{
	if (!_text.empty()) _content.push_back(wrap(std::move(_text)));
	_content.push_back(wrap(src));
}

void tag::append(std::string&& src)
{
	if (!_text.empty()) _content.push_back(wrap(std::move(_text)));
	_content.push_back(wrap(std::move(src)));
}

std::vector<tag*> tag::alias()
{
	decltype(auto) ub = _content.size();
	std::vector<tag*> ret(ub, nullptr);
	if (0 < ub) {
		const auto src = _content.data();
		do {
			--ub;
			ret[ub] = src + ub;
		} while (0 < ub);
	}
	return ret;
}


static void content_to_vector(const std::vector<tag>& src, std::vector<std::string>& dest)
{
	if (!src.empty()) for (auto& x : src) {
		auto test = x.to_s();
		if (!test.empty()) dest.push_back(x.to_s());
	}
}

static std::string destructive_vector_to_s(std::vector<std::string>& src)
{
	std::string ret;
	while (!src.empty()) {
		ret = src.back() + std::move(ret);
		src.pop_back();
	}
	return ret;
}

std::string tag::to_s() const
{
	const bool has_name = !_name.empty();
	if (_content.empty() && _text.empty()) return has_name ? start_opening_tag+_name+end_selfclose_tag : std::string();
	std::vector<std::string> ret;
	if (has_name) ret.push_back(to_s_start());
	if (!_text.empty()) ret.push_back(_text);
	else content_to_vector(_content, ret);
	if (has_name) ret.push_back(to_s_end());
	return destructive_vector_to_s(ret);
}

std::string& tag::destructive_quot_escape(std::string& x)
{
	size_t pos = 0;
	while (std::string::npos != (pos = x.find(close_attr_val, pos))) x.replace(pos, 1, quot_entity);
	return x;
}

static void attr_to_vector(const std::map<std::string, std::string>& src, std::vector<std::string>& dest)
{
	if (!src.empty()) for (auto& x : src) dest.push_back(space + x.first + open_attr_val + x.second + close_attr_val);
}

std::string tag::to_s_start() const
{
	if (_name.empty()) return std::string();
	if (_attr.empty()) return start_opening_tag + _name + end_tag;
	std::vector<std::string> ret;
	ret.push_back(start_opening_tag + _name);
	attr_to_vector(_attr, ret);
	ret.push_back(end_tag);
	return destructive_vector_to_s(ret);
}

std::string tag::to_s_content() const
{
	if (!_text.empty()) return _text;
	std::vector<std::string> ret;
	content_to_vector(_content, ret);
	return destructive_vector_to_s(ret);
}

std::string tag::to_s_end() const
{
	if (_name.empty()) return std::string();
	return start_closing_tag + _name + end_tag;
}

tag* tag::querySelector(const std::string& selector)
{
	if (selector.empty()) return nullptr;
	auto pos = selector.find_first_of(" *>+.#[");
	if (std::string::npos == pos) return _querySelector_tagname(selector);
	auto str = selector.data();
	if (0 < pos) {
		auto leading_tag = selector.substr(0, pos);
		auto test = _querySelector_tagname(leading_tag);
		if (!test) return nullptr;
		while (' ' == str[pos]) if (selector.size() <= ++pos) return test;
		return test->querySelector(selector.substr(pos));
	}
	switch (str[pos])
	{
	case ' ':
		while (' ' == str[pos]) if (selector.size() <= ++pos) return nullptr;
		return querySelector(selector.substr(pos));
	case '*':	// wildcard selector
		{
		if (selector.size() <= ++pos) return this;
		while (' ' == str[pos]) if (selector.size() <= ++pos) return this;
		auto subselector = selector.substr(pos);
		for (auto& x : _content) if (auto test = x.querySelector(subselector)) return test;
		return nullptr;
		}
	case '#':
		if (selector.size() <= ++pos) return nullptr;
		while (' ' == str[pos]) if (selector.size() <= ++pos) return nullptr;
		{
		auto subselector = selector.substr(pos);
		pos = subselector.find_first_of(" *>+.#[");
		if (std::string::npos == pos) return _querySelector_attr_val("id", subselector);
		if (0 == pos) return nullptr;
		auto relay = _querySelector_attr_val("id", subselector.substr(0, pos));
		if (!relay) return nullptr;
		str = subselector.data();
		while (' ' == str[pos]) if (subselector.size() <= ++pos) return relay;
		return relay->querySelector(subselector.substr(pos));
		}
	}
	// unhandled
#ifndef NDEBUG
	throw std::logic_error("need to handle more selector syntax");
#endif
	return nullptr;
}

tag* tag::_querySelector_tagname(const std::string& tagname)
{
	if (tagname == _name) return this;
	if (!_content.empty()) for (auto& _tag : _content) if (auto test = _tag._querySelector_tagname(tagname)) return test;
	return nullptr;
}

tag* tag::_querySelector_attr_val(const std::string& key, const std::string& val)
{
	auto id_val = read(key);
	if (id_val && *id_val == val) return this;
	for (auto& x : _content) if (auto test = x._querySelector_attr_val(key, val)) return test;
	return nullptr;
}


void to_text::print(const tag& src) {
	auto _html = src.to_s();
	if (!_html.empty()) fwrite(_html.c_str(), 1, _html.size(), dest);
}

static void _start_print(const tag& src, FILE* dest)
{
	auto _html = src.to_s_start();
	if (!_html.empty()) fwrite(_html.c_str(), 1, _html.size(), dest);
	_html = src.to_s_content();
	if (!_html.empty()) fwrite(_html.c_str(), 1, _html.size(), dest);
}

void to_text::start_print(const tag& src) {
	_start_print(src, dest);
	_stack.push_back(src);
}

void to_text::start_print(tag&& src) {
	_start_print(src, dest);
	_stack.push_back(std::move(src));
}

bool to_text::end_print() {
	if (!_stack.empty()) {
		auto _html = _stack.back().to_s_end();
		if (!_html.empty()) fwrite(_html.c_str(), 1, _html.size(), dest);
		_stack.pop_back();
	}
	return !_stack.empty();
}

void to_text::raw_copy(FILE* src)
{
	int ch;
	while (EOF != (ch = fgetc(src))) {
		if (EOF == fputc(ch, dest)) break;
	}
}

static bool id_identity_char(unsigned char src)
{
	if ('-' == src) return true;
	return isalnum(src);
}

std::string encode_id(const char* src)
{
	static_assert(16 * 16 > (unsigned char)(-1)); // check for needing more cases (grossly non-POSIX systems e.g. PDP-11).  EBCDIC would pass this.
	static constexpr const char* hex_string = "0123456789ABCDEF";
	std::string ret;
	if (!src) return ret;
	size_t i = 0;
	while (unsigned char ch = src[i++]) {
		if (id_identity_char(ch)) {
			ret += ch;
			continue;
		}
		if ('_' == ch) {
			ret += "__";
			continue;
		}
		// proper unicode:
		// * _u for page 0
		// * _U for astral characters
		ret += "_x";
		ret += hex_string[ch / 16];
		ret += hex_string[ch % 16];
	}
	return ret;
}

}	// namespace html
