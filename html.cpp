#include "html.hpp"

namespace html {

// no constexpr std::string constructors available in C++17
static const std::string start_opening_tag("<");
static const std::string start_closing_tag("</");
static const std::string end_tag(">");
static const std::string space(" ");
static const std::string end_selfclose_tag(" />");	// browsers parse this as HTML or XHTML
static const std::string open_attr_val("=\"");
static const std::string close_attr_val("\"");
static const std::string quot_entity("&quot;");

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
	if (_content.empty()) return has_name ? start_opening_tag+_name+end_selfclose_tag : std::string();
	std::vector<std::string> ret;
	if (has_name) ret.push_back(to_s_start());
	content_to_vector(_content, ret);
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
	std::vector<std::string> ret;
	content_to_vector(_content, ret);
	return destructive_vector_to_s(ret);
}

std::string tag::to_s_end() const
{
	if (_name.empty()) return std::string();
	return start_closing_tag + _name + end_tag;
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

}	// namespace html
