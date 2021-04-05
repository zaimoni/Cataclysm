#ifndef GRAMMAR_H
#define GRAMMAR_H 1

#include <string>

namespace grammar {

// not quite, but anything in this enumeration should be mutually exclusive with the/a/an in English
enum class article {
	none = 0,
	definite,
	indefinite
};

// English has *very* simple grammar (no issues with e.g. gender agreement of verbs, or number/gender agreement of
// possessives with the owned/component item)
struct noun
{
	enum class role {
		subject = 0,
		direct_object,
		indirect_object,
		possessive
	};

	virtual bool is_proper() const { return false; }
	virtual std::string direct_object() const = 0;
	virtual std::string possessive() const = 0;

	std::string desc(article prefix, role r);
};

}

#endif
