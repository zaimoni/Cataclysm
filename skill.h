#ifndef _SKILL_H_
#define _SKILL_H_

#include "skill_enum.h"
#include "enum_json.h"

DECLARE_JSON_ENUM_SUPPORT(skill)

const char* skill_name(skill);
const char* skill_description(skill);
double price_adjustment(int);
#endif
