#include "grammar.h"

namespace grammar {

// English build option.
static std::string text(noun::role r, const noun& n)
{
	switch (r)
	{
	case noun::role::direct_object: return n.direct_object();
	case noun::role::possessive: return n.possessive();
	default: return std::string();
	}
}

static const char* text(article prefix, const std::string& target)
{
	switch (prefix)
	{
	case article::definite: return "the ";
	case article::indefinite:
		// \todo need an is-vowel test; ultimately a UNICODE issue as accent marks
		// should not change vowel status
		return strchr("aeiouAEIOU", target.front()) ? "an " : "a ";
	default: return nullptr;
	}
}

std::string noun::desc(article prefix, role r)
{
	auto ret(text(r, *this));
	if (!is_proper()) {	// not *always*, but exceptions generally have an implicit reading that is consistent
		auto prepend(text(prefix, ret));
		if (prepend) ret = prepend + ret;
	}
	return ret;
}

}
