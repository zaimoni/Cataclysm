#include "pldata.h"

#include <sstream>

// this file has to build without material changes for Socrates' Daimon, so no referring to the player class here until that builds for Socrates' Daimon.
static stat_delta addict_linear_stat_step(const addiction& add)
{
    stat_delta ret = { 0,0,0,0 };

    switch (add.type) {
    case ADD_PKILLER:
        ret.Str = 7;
        break;
    }

    return ret;
}

stat_delta addict_stat_effects(const addiction& add)
{
    auto step = addict_linear_stat_step(add);
    stat_delta ret = { 0,0,0,0 };

    // historically:
    // * ADD_CIG claimed but did not impart -1 INT
    // * ADD_CAFFIENE claimed but did not impart -1 STR
    switch (add.type) {
    case ADD_ALCOHOL:
        ret.Per = -1;
        ret.Int = -1;
        break;

    case ADD_PKILLER:
        ret.Str = -1;
        ret.Per = -1;
        ret.Dex = -1;
        break;

    case ADD_SPEED:
        ret.Str = -1;
        ret.Int = -1;
        break;

    case ADD_COKE:
        ret.Per = -1;
        ret.Int = -1;
        break;
    }

    if (step.Str) ret.Str -= add.intensity / step.Str;
    if (step.Dex) ret.Dex -= add.intensity / step.Dex;
    if (step.Int) ret.Int -= add.intensity / step.Int;
    if (step.Per) ret.Per -= add.intensity / step.Per;

    return ret;
}

std::string addiction_name(const addiction& cur)
{
    switch (cur.type) {
    case ADD_CIG:		return "Nicotine Withdrawal";
    case ADD_CAFFEINE:	return "Caffeine Withdrawal";
    case ADD_ALCOHOL:	return "Alcohol Withdrawal";
    case ADD_SLEEP:	return "Sleeping Pill Dependance";
    case ADD_PKILLER:	return "Opiate Withdrawal";
    case ADD_SPEED:	return "Amphetamine Withdrawal";
    case ADD_COKE:	return "Cocaine Withdrawal";
    case ADD_THC:	return "Marijuana Withdrawal";
    default:		return "Erroneous addiction";
    }
}

static const char* addiction_static_text(const addiction& cur)
{
    switch (cur.type) {
    case ADD_CIG: return "Occasional cravings";
    case ADD_CAFFEINE: return "Slight sluggishness; Occasional cravings";
    case ADD_ALCOHOL: return "Occasional Cravings;\nRisk of delirium tremens";
    case ADD_SLEEP: return "You may find it difficult to sleep without medication.";
    case ADD_PKILLER: return "\nDepression and physical pain to some degree.  Frequent cravings.  Vomiting.";
    case ADD_SPEED: return "\nMovement rate reduction.  Depression.  Weak immune system.  Frequent cravings.";
    case ADD_COKE: return "Frequent cravings.";
    case ADD_THC: return "Occasional cravings";
    default: return nullptr;
    }
}

std::string addiction_text(const addiction& cur)
{
    const auto s_text = addiction_static_text(cur);
    if (!s_text) return std::string();  // invalid
#ifdef SOCRATES_DAIMON
    const auto step = addict_linear_stat_step(cur);
#endif
    const auto delta = addict_stat_effects(cur);
    std::ostringstream dump;
    bool non_empty = false;

    if (delta.Str) {
        dump << "Strength " << delta.Str;
#ifdef SOCRATES_DAIMON
        if (step.Str) dump << "-(intensity/" << step.Str << ")";
#endif
        non_empty = true;
    }
    if (delta.Dex) {
        if (non_empty) dump << "; ";
        dump << "Dexterity " << delta.Dex;
#ifdef SOCRATES_DAIMON
        if (step.Dex) dump << "-(intensity/" << step.Dex << ")";
#endif
        non_empty = true;
    }
    if (delta.Int) {
        if (non_empty) dump << "; ";
        dump << "Intelligence " << delta.Int;
#ifdef SOCRATES_DAIMON
        if (step.Int) dump << "-(intensity/" << step.Int << ")";
#endif
        non_empty = true;
    }
    if (delta.Per) {
        if (non_empty) dump << "; ";
        dump << "Perception " << delta.Per;
#ifdef SOCRATES_DAIMON
        if (step.Per) dump << "-(intensity/" << step.Per << ")";
#endif
        non_empty = true;
    }
    if (!non_empty) return s_text;
    if ('\n' != s_text[0]) dump << "; ";
    return dump.str() + s_text;
}
