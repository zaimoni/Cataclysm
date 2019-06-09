#ifndef _SKILL_H_
#define _SKILL_H_

#include "enums.h"
#include <string>

enum skill {
 sk_null = 0,
// Melee
 sk_dodge, sk_melee, sk_unarmed, sk_bashing, sk_cutting, sk_stabbing,
// Combat
 sk_throw, sk_gun, sk_pistol, sk_shotgun, sk_smg, sk_rifle, sk_archery,
  sk_launcher,
// Crafting
 sk_mechanics, sk_electronics, sk_cooking, sk_tailor, sk_carpentry,
// Medical
 sk_firstaid,
// Social
 sk_speech, sk_barter,
// Other
 sk_computer, sk_survival, sk_traps, sk_swimming, sk_driving,
 num_skill_types	// MUST be last!
};

const char* JSON_key(skill src);

namespace cataclysm {

	template<>
	struct JSON_parse<skill>
	{
		skill operator()(const char* src);
		skill operator()(const std::string& src) { return operator()(src.c_str()); };
	};

}

const char* skill_name(skill);
const char* skill_description(skill);
double price_adjustment(int);
#endif
