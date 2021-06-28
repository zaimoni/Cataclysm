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
	virtual unsigned gender() const { return 0; } // 0: neuter; 1 masculine; 2 feminine (English); languages that need more genders use codes higher than 2
	virtual std::string subject() const = 0;
	virtual std::string direct_object() const = 0;
	virtual std::string indirect_object() const = 0;
	virtual std::string possessive() const = 0;

	std::string desc(role r, article prefix = article::none);
	virtual std::string pronoun(role r) const { return typical_pronoun(r); }
	virtual std::string regular_verb_agreement(const std::string& verb) const;
protected:
	std::string typical_pronoun(role r) const; // third person singular
	static void regular_possessive(std::string& src);
};

std::string capitalize(std::string&& src);

}

#endif
