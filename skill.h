#ifndef _SKILL_H_
#define _SKILL_H_

#include "enum_json.h"

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

DECLARE_JSON_ENUM_SUPPORT(skill)

const char* skill_name(skill);
const char* skill_description(skill);

#ifndef SOCRATES_DAIMON
inline std::string skill_link(skill sk) { return skill_name(sk); }
#else
std::string skill_link(skill sk);
#endif

#endif
