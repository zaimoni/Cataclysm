#include "grammar.h"
#include <cstring>
#include <cctype>

namespace grammar {

// English build option.
static std::string text(noun::role r, const noun& n)
{
	switch (r)
	{
	case noun::role::subject: return n.subject();
	case noun::role::direct_object: return n.direct_object();
	case noun::role::indirect_object: return n.indirect_object();
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

std::string noun::desc(role r, article prefix) const
{
	auto ret(text(r, *this));
	if (!is_proper()) {	// not *always*, but exceptions generally have an implicit reading that is consistent
		auto prepend(text(prefix, ret));
		if (prepend) ret = prepend + ret;
	}
	return ret;
}

// for now, just "most precise singular"
std::string noun::typical_pronoun(role r) const
{
	switch (r)
	{
	case noun::role::subject:
		switch(gender())
		{
		case 1: return "he";
		case 2: return "she";
		default: return "it";
		}
	case noun::role::direct_object:
	case noun::role::indirect_object:
		switch(gender())
		{
		case 1: return "him";
		case 2: return "her";
		default: return "it";
		}
	case noun::role::possessive:
		switch(gender())
		{
		case 1: return "his";
		case 2: return "hers";
		default: return "its";
		}
	default: return std::string();
	}
}

std::string noun::regular_verb_agreement(const std::string& verb) const
{	// \todo more precise handling
	return verb + "s";
}

void noun::regular_possessive(std::string& src)
{
	src += "'s";
}

std::string noun::VO_phrase(const std::string& verb, const std::string& obj) const
{
	return regular_verb_agreement(verb) + " " + obj;
}


std::string capitalize(std::string&& src)
{
	if (!src.empty()) src.front() = toupper(src.front());
	return std::move(src);
}

std::string SVO_sentence(const noun& s, const std::string& verb, const std::string& obj, const char* terminate)
{
	return grammar::capitalize(s.subject()) + " " + s.regular_verb_agreement(verb) + " " + obj + terminate;
}


}
