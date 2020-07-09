// master implementation file for new-style saveload support

#ifndef SOCRATES_DAIMON
#include "computer.h"
#else
#include "item.h"
#endif
#include "mapdata.h"
#ifndef SOCRATES_DAIMON
#include "mission.h"
#include "overmap.h"
#include "monster.h"
#include "weather.h"
#include "iuse.h"
#else
#include "mtype.h"
#endif
#include "skill.h"
#include "recent_msg.h"
#ifndef SOCRATES_DAIMON
#include "saveload.h"
#include "submap.h"
#endif
#include "json.h"

#include <istream>
#include <ostream>
#ifdef SOCRATES_DAIMON
#include <sstream>
#endif

#include "Zaimoni.STL/Logging.h"

// enums.h doesn't have a recognizable implementation file, so park this here for now
static const char* const JSON_transcode_material[] = {
	"LIQUID",
	"VEGGY",
	"FLESH",
	"POWDER",
	"COTTON",
	"WOOL",
	"LEATHER",
	"KEVLAR",
	"STONE",
	"PAPER",
	"WOOD",
	"PLASTIC",
	"GLASS",
	"IRON",
	"STEEL",
	"SILVER"
};

// while nc_color does have an implementation file, the header doesn't include enums.h or <string>
static const char* const JSON_transcode_nc_color[] = {
	"c_black",
	"c_white",
	"c_ltgray",
	"c_dkgray",
	"c_red",
	"c_green",
	"c_blue",
	"c_cyan",
	"c_magenta",
	"c_brown",
	"c_ltred",
	"c_ltgreen",
	"c_ltblue",
	"c_ltcyan",
	"c_pink",
	"c_yellow",
	"h_black",
	"h_white",
	"h_ltgray",
	"h_dkgray",
	"h_red",
	"h_green",
	"h_blue",
	"h_cyan",
	"h_magenta",
	"h_brown",
	"h_ltred",
	"h_ltgreen",
	"h_ltblue",
	"h_ltcyan",
	"h_pink",
	"h_yellow",
	"i_black",
	"i_white",
	"i_ltgray",
	"i_dkgray",
	"i_red",
	"i_green",
	"i_blue",
	"i_cyan",
	"i_magenta",
	"i_brown",
	"i_ltred",
	"i_ltgreen",
	"i_ltblue",
	"i_ltcyan",
	"i_pink",
	"i_yellow",
	"c_white_red",
	"c_ltgray_red",
	"c_dkgray_red",
	"c_red_red",
	"c_green_red",
	"c_blue_red",
	"c_cyan_red",
	"c_magenta_red",
	"c_brown_red",
	"c_ltred_red",
	"c_ltgreen_red",
	"c_ltblue_red",
	"c_ltcyan_red",
	"c_pink_red",
	"c_yellow_red"
};

// relocate artifact enum JSON transcoding here for Socrates' Daimon
static const char* const JSON_transcode_artifactactives[] = {
	"STORM",
	"FIREBALL",
	"ADRENALINE",
	"MAP",
	"BLOOD",
	"FATIGUE",
	"ACIDBALL",
	"PULSE",
	"HEAL",
	"CONFUSED",
	"ENTRANCE",
	"BUGS",
	"TELEPORT",
	"LIGHT",
	"GROWTH",
	"HURTALL",
	"",	// AEA_SPLIT
	"RADIATION",
	"PAIN",
	"MUTATE",
	"PARALYZE",
	"FIRESTORM",
	"ATTENTION",
	"TELEGLOW",
	"NOISE",
	"SCREAM",
	"DIM",
	"FLASH",
	"VOMIT",
	"SHADOWS"
};

static const char* const JSON_transcode_artifactpassives[] = {
	"STR_UP",
	"DEX_UP",
	"PER_UP",
	"INT_UP",
	"ALL_UP",
	"SPEED_UP",
	"IODINE",
	"SNAKES",
	"INVISIBLE",
	"CLAIRVOYANCE",
	"STEALTH",
	"EXTINGUISH",
	"GLOW",
	"PSYSHIELD",
	"RESIST_ELECTRICITY",
	"CARRY_MORE",
	"SAP_LIFE",
	"",	// AEP_SPLIT
	"HUNGER",
	"THIRST",
	"SMOKE",
	"EVIL",
	"SCHIZO",
	"RADIOACTIVE",
	"MUTAGENIC",
	"ATTENTION",
	"STR_DOWN",
	"DEX_DOWN",
	"PER_DOWN",
	"INT_DOWN",
	"ALL_DOWN",
	"SPEED_DOWN",
	"FORCE_TELEPORT",
	"MOVEMENT_NOISE",
	"BAD_WEATHER",
	"SICK"
};

static const char* const JSON_transcode_artifactcharging[] = {
	"ARTC_TIME",
	"ARTC_SOLAR",
	"ARTC_PAIN",
	"ARTC_HP"
};

DECLARE_JSON_ENUM_SUPPORT(material)
DECLARE_JSON_ENUM_SUPPORT(nc_color)
DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(material, JSON_transcode_material)
DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(nc_color, JSON_transcode_nc_color)

DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(art_charge, JSON_transcode_artifactcharging)
DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(art_effect_active, JSON_transcode_artifactactives)
DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(art_effect_passive, JSON_transcode_artifactpassives)

using cataclysm::JSON;

// legacy implementations assume text mode streams
// this only makes a difference for ostream, and may not be correct (different API may be needed)

#ifdef BINARY_STREAMS
#define I_SEP
#else
#define I_SEP << " "
#endif

#ifndef SOCRATES_DAIMON
#define IO_OPS_ENUM(TYPE)	\
std::istream& operator>>(std::istream& is, TYPE& dest)	\
{	\
	int tmp;	\
	is >> tmp;	\
	dest = (TYPE)tmp;	\
	return is;	\
}	\
	\
std::ostream& operator<<(std::ostream& os, TYPE src)	\
{	\
	return os << int(src);	\
}

IO_OPS_ENUM(trap_id)
#endif

#define JSON_ENUM(TYPE)	\
JSON toJSON(TYPE src) {	\
	auto x = JSON_key(src);	\
	if (x) return JSON(x);	\
	throw std::runtime_error(std::string("encoding failure: " #TYPE " value ")+std::to_string((int)src));	\
}	\
	\
bool fromJSON(const JSON& src, TYPE& dest)	\
{	\
	if (!src.is_scalar()) return false;	\
	cataclysm::JSON_parse<TYPE> parse;	\
	dest = parse(src.scalar());	\
	return true;	\
}

#ifndef SOCRATES_DAIMON
JSON_ENUM(activity_type)
JSON_ENUM(add_type)
#endif
JSON_ENUM(ammotype)
JSON_ENUM(art_charge)
JSON_ENUM(art_effect_active)
JSON_ENUM(art_effect_passive)
#ifndef SOCRATES_DAIMON
JSON_ENUM(bionic_id)
JSON_ENUM(combat_engagement)
JSON_ENUM(computer_action)
JSON_ENUM(computer_failure)
JSON_ENUM(dis_type)
JSON_ENUM(faction_goal)
JSON_ENUM(faction_job)
JSON_ENUM(field_id)
JSON_ENUM(hp_part)
#endif
JSON_ENUM(itype_id)	// breaks if we need a randart as a morale_point item
JSON_ENUM(material)
#ifndef SOCRATES_DAIMON
JSON_ENUM(mission_id)
#endif
JSON_ENUM(mon_id)
#ifndef SOCRATES_DAIMON
JSON_ENUM(moncat_id)
JSON_ENUM(monster_effect_type)
JSON_ENUM(morale_type)
JSON_ENUM(mutation_category)
#endif
JSON_ENUM(nc_color)
#ifndef SOCRATES_DAIMON
JSON_ENUM(npc_attitude)
JSON_ENUM(npc_class)
JSON_ENUM(npc_favor_type)
JSON_ENUM(npc_need)
JSON_ENUM(npc_mission)
JSON_ENUM(pl_flag)
#endif
JSON_ENUM(skill)
#ifndef SOCRATES_DAIMON
JSON_ENUM(talk_topic)
JSON_ENUM(ter_id)
JSON_ENUM(trap_id)
JSON_ENUM(vhtype_id)
JSON_ENUM(vpart_id)
JSON_ENUM(weather_type)
#endif

template<class LHS, class RHS>
JSON toJSON(const std::pair<LHS, RHS>& src)
{
	JSON ret(JSON::array);
	ret.push(toJSON(src.first));
	ret.push(toJSON(src.second));
	return ret;
}

template<class LHS, class RHS>
bool fromJSON(const JSON& src, std::pair<LHS, RHS>& dest)
{
	if (JSON::array != src.mode() || 2 != src.size()) return false;
	// this is meant to be ACID, so we need a working copy
	std::pair<LHS, RHS> working;
	if (fromJSON(src[0], working.first) && fromJSON(src[1], working.second)) {
		dest = std::move(working);	// in case the underlying types need it
		return true;
	}
	return false;
}

// stereotypical translation of pointers to/from vector indexes
// \todo in general if a loaded pointer index is "invalid" we should warn here; non-null requirements are enforced higher up
// \todo in general warn if a non-null ptr points to an invalid id
#ifndef SOCRATES_DAIMON
JSON toJSON(const mission_type* const src) {
	auto x = JSON_key((mission_id)src->id);
	if (x) return JSON(x);
	throw std::runtime_error(std::string("encoding failure: mission_id value ") + std::to_string(src->id));
}

bool fromJSON(const JSON& src, const mission_type*& dest)
{
	if (!src.is_scalar()) return false;
	cataclysm::JSON_parse<mission_id> parse;
	auto type_id = parse(src.scalar());
	dest = type_id ? &mission_type::types[type_id] : 0;
	return true;
}
#endif

bool fromJSON(const JSON& src, const mtype*& dest)
{
	if (!src.is_scalar()) return false;
	mon_id type_id;
	bool ret = fromJSON(src, type_id);
	if (ret) dest = mtype::types[type_id];
	return ret;
}

bool fromJSON(const JSON& src, const itype*& dest)
{
	if (!src.is_scalar()) return false;
	itype_id type_id;
	int artifact_id;
	bool ret = fromJSON(src, type_id);
	if (ret) dest = item::types[type_id];	// XXX \todo should be itype::types?
	else if (ret = fromJSON(src, artifact_id) && item::types.size() > artifact_id+(num_all_items-1)) dest = item::types[artifact_id + (num_all_items - 1)];
	else if (ret = fromJSON(src, artifact_id) && item::types.size() > artifact_id) dest = item::types[artifact_id];	// \todo V0.3.1+: remove, this is backward-compatibility
	// second try: artifacts
	return ret;
}

bool fromJSON(const JSON& src, const it_ammo*& dest)
{
	if (!src.is_scalar()) return false;
	itype_id type_id;
	bool ret = fromJSON(src, type_id);
	if (ret) {
		if (auto test = dynamic_cast<it_ammo*>(item::types[type_id])) dest = test;
		else ret = false;
	}
	return ret;
}

#ifndef SOCRATES_DAIMON
std::istream& operator>>(std::istream& is, point& dest)
{
	JSON pt(is);
	if (2 != pt.size() || JSON::array!=pt.mode()) throw std::runtime_error("point expected to be a length 2 array");
	if (!fromJSON(pt[0],dest.x) || !fromJSON(pt[1], dest.y)) throw std::runtime_error("point wants integer coordinates");
	return is;
}

std::ostream& operator<<(std::ostream& os, const point& src)
{
	return os << '[' << std::to_string(src.x) << ',' << std::to_string(src.y) << ']';
}

bool fromJSON(const cataclysm::JSON& src, point& dest)
{
	if (2 != src.size() || JSON::array != src.mode()) return false;
	point staging;
	bool ret = fromJSON(src[0], staging.x) && fromJSON(src[1], staging.y);
	if (ret) dest = staging;
	return ret;
}

JSON toJSON(const point& src)
{
	JSON ret;
	ret.push(std::to_string(src.x));
	ret.push(std::to_string(src.y));
	return ret;
}

std::istream& operator>>(std::istream& is, tripoint& dest)
{
	JSON pt(is);
	if (3 != pt.size() || JSON::array != pt.mode()) throw std::runtime_error("tripoint expected to be a length 3 array");
	if (!fromJSON(pt[0], dest.x) || !fromJSON(pt[1], dest.y) || !fromJSON(pt[2], dest.z)) throw std::runtime_error("tripoint wants integer coordinates");
	return is;
}

std::ostream& operator<<(std::ostream& os, const tripoint& src)
{
	return os << '[' << std::to_string(src.x) << ',' << std::to_string(src.y) << ',' << std::to_string(src.z) << ']';
}

bool fromJSON(const cataclysm::JSON& src, tripoint& dest)
{
	if (3 != src.size() || JSON::array != src.mode()) return false;
	tripoint staging;
	bool ret = fromJSON(src[0], staging.x) && fromJSON(src[1], staging.y) && fromJSON(src[2], staging.z);
	if (ret) dest = staging;
	return ret;
}

JSON toJSON(const tripoint& src)
{
	JSON ret;
	ret.push(std::to_string(src.x));
	ret.push(std::to_string(src.y));
	ret.push(std::to_string(src.z));
	return ret;
}

JSON toJSON(const monster_effect& src) {
	JSON ret(JSON::object);
	if (0 < src.duration) {
		if (auto json = JSON_key(src.type)) {
			ret.set("type", json);
			ret.set("duration", std::to_string(src.duration));
		}
	}
	return ret;
}

bool fromJSON(const JSON& src, monster_effect& dest)
{
	if (!src.has_key("duration") || !src.has_key("type")) return false;
	bool ret = fromJSON(src["type"], dest.type);
	if (!fromJSON(src["duration"], dest.duration)) ret = false;
	return ret;
}

// will need friend-declaration when access controls go up
template<class T>
JSON toJSON(const countdown<T>& src) {
	JSON ret;
	ret.set("x", toJSON(src.x));
	ret.set("remaining", std::to_string(src.remaining));
	return ret;
}

template<class T>
bool fromJSON(const JSON& src, countdown<T>& dest)
{
	if (!src.has_key("remaining")) return false;

	decltype(dest.remaining) tmp;
	if (!fromJSON(src["remaining"], tmp) || 0 >= tmp) return false;
	if (!src.has_key("x") || !fromJSON(src["x"], dest.x)) return false;
	dest.remaining = tmp;
	return true;
}

JSON toJSON(const computer_option& src)
{
	JSON opt;
	opt.set("name", src.name);
	opt.set("action", JSON_key(src.action));
	if (0 < src.security) opt.set("security", std::to_string(src.security));
	return opt;
}

bool fromJSON(const JSON& src, computer_option& dest)
{
	bool ok = true;
	if (!src.has_key("name") || !fromJSON(src["name"], dest.name)) ok = false;
	if (!src.has_key("action") || !fromJSON(src["action"], dest.action)) ok = false;
	if (!ok) return false;
	if (src.has_key("security") && fromJSON(src["security"],dest.security) && 0 > dest.security) dest.security = 0;
	return true;
}

bool fromJSON(const JSON& _in, computer& dest)
{
	if (!_in.has_key("name") || !fromJSON(_in["name"], dest.name)) return false;
	if (_in.has_key("security") && fromJSON(_in["security"], dest.security) && 0 > dest.security) dest.security = 0;
	if (_in.has_key("mission_id")) fromJSON(_in["mission_id"], dest.mission_id);
	if (_in.has_key("options")) _in["options"].decode(dest.options);
	if (_in.has_key("failures")) _in["failures"].decode(dest.failures);
	return true;
}

JSON toJSON(const computer& src)
{
	JSON comp;
	comp.set("name", src.name);
	if (0 < src.security) comp.set("security", std::to_string(src.security));
	if (0 <= src.mission_id) comp.set("mission_id", std::to_string(src.mission_id));
	if (!src.options.empty()) comp.set("options", JSON::encode(src.options));
	if (!src.failures.empty()) comp.set("failures", JSON::encode(src.failures));
	return comp;
}

JSON toJSON(const bionic& src)
{
	JSON _bionic;
	_bionic.set("id", toJSON(src.id));
	if ('a' != src.invlet) {
		const char str[] = { src.invlet, '\x00' };
		_bionic.set("invlet", str);
	}
	if (src.powered) _bionic.set("powered", "true");
	if (0 < src.charge) _bionic.set("charge", std::to_string(src.charge));
	return _bionic;
}

bool fromJSON(const JSON& src, bionic& dest)
{
	if (!src.has_key("id") || !fromJSON(src["id"], dest.id)) return false;
	if (src.has_key("invlet")) fromJSON(src["invlet"], dest.invlet);
	if (src.has_key("powered")) fromJSON(src["powered"], dest.powered);
	if (src.has_key("charge")) fromJSON(src["charge"], dest.charge);
	return true;
}

JSON toJSON(const npc_favor& src) {
	JSON _favor;
	_favor.set("type", toJSON(src.type));
	switch (src.type)
	{
	case FAVOR_CASH: _favor.set("favor", std::to_string(src.value)); break;
	case FAVOR_ITEM: _favor.set("favor", JSON_key(src.item_id)); break;
	case FAVOR_TRAINING: _favor.set("favor", JSON_key(src.skill_id)); break;
	default: throw std::runtime_error("unhandled favor type");
	}
	return _favor;
}

bool fromJSON(const JSON& src, npc_favor& dest)
{
	npc_favor working;
	if (!src.has_key("type") || !fromJSON(src["type"], working.type)) return false;
	if (!src.has_key("favor")) return false;
	auto& x = src["favor"];
	switch (working.type)
	{
	case FAVOR_CASH: if (!fromJSON(x, working.value)) return false;
		break;
	case FAVOR_ITEM: if (!fromJSON(x, working.item_id)) return false;
		break;
	case FAVOR_TRAINING: if (!fromJSON(x, working.skill_id)) return false;
		break;
	default: return false;
	}
	dest = working;
	return true;
}

static void loadFactionID(const JSON& _in, int& dest)
{
	int tmp;
	if (fromJSON(_in, tmp) && faction::from_id(tmp)) dest = tmp;
}

static void loadFactionID(const JSON& _in, faction*& fac)
{
	int tmp;
	if (fromJSON(_in, tmp)) fac = faction::from_id(tmp);
}

bool fromJSON(const JSON& _in, mission& dest)
{
	if (!_in.has_key("type") || !fromJSON(_in["type"], dest.type)) return false;
	if (!_in.has_key("uid") || !fromJSON(_in["uid"], dest.uid) || 0 >= dest.uid) return false;
	if (_in.has_key("description")) fromJSON(_in["description"], dest.description);
	if (_in.has_key("failed")) fromJSON(_in["failed"], dest.failed);
	if (_in.has_key("value")) fromJSON(_in["value"], dest.value);
	if (_in.has_key("reward")) fromJSON(_in["reward"], dest.reward);
	if (_in.has_key("target") && !fromJSON(_in["target"], dest.target)) {
		point tmp;	// V0.2.1- format was point rather than OM_loc
		if (fromJSON(_in["target"], tmp)) dest.target.second = tmp;
	}
	if (_in.has_key("item")) fromJSON(_in["item"], dest.item_id);
	if (_in.has_key("count")) fromJSON(_in["count"], dest.count);
	if (_in.has_key("deadline")) fromJSON(_in["deadline"], dest.deadline);
	if (_in.has_key("npc")) fromJSON(_in["npc"], dest.npc_id);
	if (_in.has_key("by_faction")) loadFactionID(_in["by_faction"], dest.good_fac_id);
	if (_in.has_key("vs_faction")) loadFactionID(_in["vs_faction"], dest.bad_fac_id);
	if (_in.has_key("step")) fromJSON(_in["step"], dest.step);
	if (_in.has_key("next")) fromJSON(_in["next"], dest.follow_up);
	return true;
}

JSON toJSON(const mission& src)
{
	JSON _mission;
	_mission.set("type", toJSON(src.type));
	_mission.set("uid", std::to_string(src.uid));
	if (!src.description.empty()) _mission.set("description", src.description);
	if (src.failed) _mission.set("failed", "true");
	if (src.value) _mission.set("value", std::to_string(src.value));	// for now allow negative mission values \todo audit for negative mission values
	if (JSON_key(src.reward.type)) _mission.set("value", toJSON(src.reward));
	// src.reward
	if (src.target.is_valid()) _mission.set("target", toJSON(src.target));
	if (src.count) {
		if (auto json = JSON_key(src.item_id)) {
			_mission.set("item", json);
			_mission.set("count", std::to_string(src.count));
		}
	}
	if (0 < src.deadline) _mission.set("deadline", std::to_string(src.deadline));
	if (0 < src.npc_id) _mission.set("npc", std::to_string(src.npc_id));	// npc need not be active
	if (0 <= src.good_fac_id) _mission.set("by_faction", std::to_string(src.good_fac_id));
	if (0 <= src.bad_fac_id) _mission.set("vs_faction", std::to_string(src.bad_fac_id));
	if (src.step) _mission.set("step", std::to_string(src.step));
	{
		if (auto json = JSON_key(src.follow_up)) _mission.set("next", json);
	}
	return _mission;
}

bool fromJSON(const JSON& _in, mongroup& dest)
{
	int tmp;
	if (!_in.has_key("type") || !fromJSON(_in["type"], dest.type)) return false;
	if (!_in.has_key("pos") || !fromJSON(_in["pos"], dest.pos)) return false;
	if (!_in.has_key("radius") || !fromJSON(_in["radius"], tmp)) return false;
	if (!_in.has_key("population") || !fromJSON(_in["population"], dest.population)) return false;
	dest.radius = tmp;
	if (_in.has_key("dying")) fromJSON(_in["dying"], dest.dying);
	return true;
}

JSON toJSON(const mongroup& src)
{
	JSON _mongroup;
	_mongroup.set("type", toJSON(src.type));
	_mongroup.set("pos", toJSON(src.pos));
	_mongroup.set("radius", std::to_string(src.radius));
	_mongroup.set("population", std::to_string(src.population));
	if (src.dying) _mongroup.set("dying", "true");
	return _mongroup;
}

bool fromJSON(const JSON& _in, field& dest)
{
	if (!_in.has_key("type") || !fromJSON(_in["type"], dest.type)) return false;
	if (_in.has_key("density")) {
		int tmp;
		fromJSON(_in["density"], tmp);
		dest.density = tmp;
	}
	if (_in.has_key("age")) fromJSON(_in["age"], dest.age);
	return true;
}

JSON toJSON(const field& src)
{
	JSON _field;
	if (const auto json = JSON_key(src.type)) {
		_field.set("type", json);
		_field.set("density", std::to_string((int)src.density));
		_field.set("age", std::to_string(src.age));
	}
	return _field;
}
#endif

// Arrays are not plausible for the final format for the overmap data classes, but
// they are easily distinguished from objects so we have a clear upgrade path.
// Plan is to reserve the object form for when the absolute coordinate system is understood.
#ifndef SOCRATES_DAIMON
bool fromJSON(const JSON& _in, city& dest)
{
	const size_t _size = _in.size();
	if ((2 != _size && 3 != _size) || JSON::array != _in.mode()) return false;
	if (!fromJSON(_in[0], dest.x) || !fromJSON(_in[1], dest.y)) return false;
	if (3 == _size && !fromJSON(_in[2], dest.s)) return false;;
	return true;
}

JSON toJSON(const city& src)
{
	JSON ret(JSON::array);
	ret.push(std::to_string(src.x));
	ret.push(std::to_string(src.y));
	if (0 < src.s) ret.push(std::to_string(src.x));
	return ret;
}

om_note::om_note(std::istream& is)
: x(-1), y(-1), num(-1), text("")
{
		JSON _in(is);
		if (JSON::array != _in.mode() || 4 != _in.size()) throw std::runtime_error("om_note expected to be a length 4 array");
		fromJSON(_in[0], x);
		fromJSON(_in[1], y);
		fromJSON(_in[2], num);
		fromJSON(_in[3], text);
}

std::ostream& operator<<(std::ostream& os, const om_note& src)
{
	return os << '[' << std::to_string(src.x) <<  ',' << std::to_string(src.y) <<  ',' << std::to_string(src.num) << ',' << JSON(src.text) << ']';
}

bool fromJSON(const JSON& _in, radio_tower& dest)
{
	if (JSON::array != _in.mode() || 4 != _in.size()) return false;
	if (!fromJSON(_in[0], dest.x)) return false;
	if (!fromJSON(_in[1], dest.y)) return false;
	if (!fromJSON(_in[2], dest.strength)) return false;
	return fromJSON(_in[3], dest.message);
}

JSON toJSON(const radio_tower& src)
{
	JSON ret(JSON::array);
	ret.push(std::to_string(src.x));
	ret.push(std::to_string(src.y));
	ret.push(std::to_string(src.strength));
	ret.push(src.message);
	return ret;
}

bool fromJSON(const JSON& src, player_activity& dest)
{
	if (!src.has_key("type") || !fromJSON(src["type"], dest.type)) return false;
	if (src.has_key("moves_left")) fromJSON(src["moves_left"], dest.moves_left);
	if (src.has_key("index")) fromJSON(src["index"], dest.index);
	if (src.has_key("values")) src["values"].decode(dest.values);
	if (src.has_key("placement")) fromJSON(src["placement"], dest.placement);
	return true;
}

JSON toJSON(const player_activity& src)
{
	JSON _act(JSON::object);
	if (const auto json = JSON_key(src.type)) {
		_act.set("type", json);
		if (0 != src.moves_left) _act.set("moves_left", std::to_string(src.moves_left));
		if (-1 != src.index) _act.set("index", std::to_string(src.index));
		if (point(-1, -1) != src.placement) _act.set("placement", toJSON(src.placement));
		if (!src.values.empty()) _act.set("values", JSON::encode(src.values));
	}
	return _act;
}

bool fromJSON(const JSON& _in, spawn_point& dest)
{
	if (!_in.has_key("type") || !fromJSON(_in["type"], dest.type)) return true;
	if (_in.has_key("name")) fromJSON(_in["name"], dest.name);
	if (_in.has_key("count")) fromJSON(_in["count"], dest.count);
	if (_in.has_key("pos")) fromJSON(_in["pos"], dest.pos);
	if (_in.has_key("faction_id")) loadFactionID(_in["faction_id"], dest.faction_id);
	if (_in.has_key("mission_id")) fromJSON(_in["mission_id"], dest.mission_id);
	if (_in.has_key("friendly")) fromJSON(_in["friendly"], dest.friendly);
	return true;
}

JSON toJSON(const spawn_point& src)
{
	JSON _spawn(JSON::object);
	if (const auto json = JSON_key(src.type)) {
		_spawn.set("type", json);
		_spawn.set("name", src.name);
		if (0 != src.count) _spawn.set("count", std::to_string(src.count));
		if (point(-1, -1) != src.pos) _spawn.set("pos", toJSON(src.pos));
		if (0 <= src.faction_id) _spawn.set("faction_id", std::to_string(src.faction_id));
		if (0 < src.mission_id) _spawn.set("mission_id", std::to_string(src.mission_id));
		if (src.friendly) _spawn.set("friendly", "true");
	}
	return _spawn;
}

bool fromJSON(const JSON& _in, vehicle_part& dest)
{
	if (!_in.has_key("id") || !fromJSON(_in["id"], dest.id)) return false;
	if (_in.has_key("mount_d")) fromJSON(_in["mount_d"], dest.mount_d);
	if (_in.has_key("precalc_d")) _in["precalc_d"].decode(dest.precalc_d, (sizeof(dest.precalc_d) / sizeof(*dest.precalc_d)));
	if (_in.has_key("hp")) fromJSON(_in["hp"], dest.hp);
	if (_in.has_key("blood")) fromJSON(_in["blood"], dest.blood);
	if (_in.has_key("inside")) fromJSON(_in["inside"], dest.inside);
	if (_in.has_key("amount")) fromJSON(_in["amount"], dest.amount);
	if (_in.has_key("items")) _in["items"].decode(dest.items);
	return true;
}

JSON toJSON(const vehicle_part& src)
{
	JSON _part(JSON::object);
	if (auto json = JSON_key(src.id)) {
		_part.set("id", json);
		_part.set("mount_d", toJSON(src.mount_d));
		_part.set("precalc_d", JSON::encode(src.precalc_d, (sizeof(src.precalc_d) / sizeof(*src.precalc_d))));
		_part.set("hp", std::to_string(src.hp));
		_part.set("blood", std::to_string(src.blood));
		_part.set("inside", src.inside ? "true" : "false");	// \todo establish default value
		_part.set("amount", std::to_string(src.amount));
		if (!src.items.empty()) _part.set("items", JSON::encode(src.items));
	}
	return _part;
}

bool fromJSON(const JSON& src, vehicle& dest)
{
	if (!src.has_key("type") || !fromJSON(src["type"], dest._type)) return false;
	if (src.has_key("name")) fromJSON(src["name"], dest.name);
	if (src.has_key("pos")) fromJSON(src["pos"], dest.pos);
	if (src.has_key("turn_dir")) fromJSON(src["turn_dir"], dest.turn_dir);
	if (src.has_key("velocity")) fromJSON(src["velocity"], dest.velocity);
	if (src.has_key("cruise_velocity")) fromJSON(src["cruise_velocity"], dest.cruise_velocity);
	if (src.has_key("cruise_on")) fromJSON(src["cruise_on"], dest.cruise_on);
	if (src.has_key("skidding")) fromJSON(src["skidding"], dest.skidding);
	if (src.has_key("moves")) fromJSON(src["moves"], dest.moves);
	if (src.has_key("parts")) src["parts"].decode(dest.parts);

	// \todo? auto-convert true/false literals to 1/0 for numeric destinations
	bool b_tmp;
	if (src.has_key("turret_burst") && fromJSON("turret_burst", b_tmp)) dest.turret_mode = b_tmp;

	int tmp;
	if (src.has_key("facing") && fromJSON(src["facing"], tmp)) dest.face.init(tmp);
	if (src.has_key("move_dir") && fromJSON(src["move_dir"], tmp)) dest.move.init(tmp);
	dest.find_external_parts();
	dest.find_exhaust();
	dest.insides_dirty = true;
	dest.precalc_mounts(0, dest.face.dir());
	return true;
}

JSON toJSON(const vehicle& src)
{
	JSON _vehicle(JSON::object);
	if (const auto json = JSON_key(src._type)) {
		_vehicle.set("type", json);
		_vehicle.set("name", src.name);
		_vehicle.set("pos", toJSON(src.pos));
		_vehicle.set("facing", std::to_string(src.face.dir()));
		_vehicle.set("move_dir", std::to_string(src.move.dir()));
		_vehicle.set("turn_dir", std::to_string(src.turn_dir));
		_vehicle.set("velocity", std::to_string(src.velocity));
		if (0 < src.cruise_velocity) _vehicle.set("cruise_velocity", std::to_string(src.cruise_velocity));
		if (!src.cruise_on) _vehicle.set("cruise_on", "false");
		if (src.turret_mode) _vehicle.set("turret_burst", "true");	// appears to be meant to be an enumeration (burst vs. autofire?)
		if (src.skidding) _vehicle.set("skidding", "true");
		_vehicle.set("moves", std::to_string(src.moves));
		if (!src.parts.empty()) _vehicle.set("parts", JSON::encode(src.parts));
	}
	return _vehicle;
}
#endif

bool fromJSON(const JSON& _in, item& dest)
{
	if (!_in.has_key("type") || !fromJSON(_in["type"], dest.type)) return false;
	if (_in.has_key("corpse")) fromJSON(_in["corpse"], dest.corpse);
	if (_in.has_key("curammo")) fromJSON(_in["curammo"], dest.curammo);
	if (_in.has_key("name")) fromJSON(_in["name"], dest.name);
	if (_in.has_key("invlet")) {
		std::string tmp;
		fromJSON(_in["invlet"], tmp);
		dest.invlet = tmp[0];
	}
	if (_in.has_key("charges")) fromJSON(_in["charges"], dest.charges);
	if (_in.has_key("active")) fromJSON(_in["active"], dest.active); {
		int tmp;
		if (_in.has_key("damage") && fromJSON(_in["damage"], tmp)) dest.damage = tmp;
		if (_in.has_key("burnt") && fromJSON(_in["burnt"], tmp)) dest.burnt = tmp;
	}
	if (_in.has_key("bday")) fromJSON(_in["bday"], dest.bday);
	if (_in.has_key("poison")) fromJSON(_in["poison"], dest.poison);
	if (_in.has_key("owned")) fromJSON(_in["owned"], dest.owned);
	if (_in.has_key("mission_id")) fromJSON(_in["mission_id"], dest.mission_id);
	if (_in.has_key("player_id")) fromJSON(_in["player_id"], dest.player_id);
	if (_in.has_key("contents")) _in["contents"].decode(dest.contents);
	return true;
}

JSON toJSON(const item& src) {
	JSON _item(JSON::object);

	if (src.type) {
		if (auto json = JSON_key((itype_id)src.type->id)) _item.set("type", json);
		if (_item.has_key("type")) {
			if (src.corpse) {
				if (auto json2 = JSON_key((mon_id)src.corpse->id)) _item.set("corpse", json2);
			}
			if (src.curammo) {
				if (auto json2 = JSON_key((itype_id)src.curammo->id)) _item.set("curammo", json2);
			}
			if (!src.name.empty()) _item.set("name", src.name.c_str());
			if (src.invlet) _item.set("invlet", std::string(1, src.invlet));
			if (0 <= src.charges) _item.set("charges", std::to_string(src.charges));
			if (src.active) _item.set("active", "true");
			if (src.damage) _item.set("damage", std::to_string((int)src.damage));
			if (src.burnt) _item.set("burnt", std::to_string((int)src.burnt));
			if (0 < src.bday) _item.set("bday", std::to_string(src.bday));
			if (src.poison) _item.set("poison", std::to_string(src.poison));
			if (0 <= src.owned) _item.set("owned", std::to_string(src.owned));
			if (0 <= src.mission_id) _item.set("mission_id", std::to_string(src.mission_id));	// \todo validate this more thoroughly
			if (-1 != src.player_id) _item.set("player_id", std::to_string(src.player_id));

			if (!src.contents.empty()) _item.set("contents", JSON::encode(src.contents));
		}
	};
	return _item;
}

#ifndef SOCRATES_DAIMON
submap::submap(std::istream& is)
: active_item_count(0), field_count(0)	// turn_last_touched omitted, will be caught later
{
	memset(ter, 0, sizeof(ter));
	memset(trp, 0, sizeof(trp));
	memset(rad, 0, sizeof(rad));	// reasonably redundant given below, but matches zero-parameter constructor

	is >> turn_last_touched;
	int turndif = int(messages.turn);
	if (turndif < 0) turndif = 0;

	if ('{' != (is >> std::ws).peek()) throw std::runtime_error("submap data lost: pre-V0.2.0 format?");
	{
		JSON sm(is);
		ter_id terrain;
		int radtmp;
		if (sm.has_key("terrain") && JSON::array==sm["terrain"].mode() && SEEY <= sm["terrain"].size()) {
			const auto& ter_map = sm["terrain"];
			int j = -1;
			while (++j < ter_map.size() && j < SEEY) {
				auto& col = ter_map[j];
				int i = -1;
				while (++i < col.size() && i < SEEX) {
					if (fromJSON(col[i], terrain)) ter[i][j] = terrain;
				}
			}
		} else if (sm.has_key("const_terrain") && fromJSON(sm["const_terrain"], terrain)) {
			for (int j = 0; j < SEEY; j++) {
				for (int i = 0; i < SEEX; i++) {
					ter[i][j] = terrain;
				}
			}
		} else throw std::runtime_error("terrain data missing");

		if (sm.has_key("radiation") && JSON::array == sm["radiation"].mode() && SEEY <= sm["radiation"].size()) {
			const auto& rad_map = sm["radiation"];
			int j = -1;
			while (++j < rad_map.size() && j < SEEY) {
				auto& col = rad_map[j];
				int i = -1;
				while (++i < col.size() && i < SEEX) {
					if (fromJSON(col[i], radtmp)) {
						radtmp -= int(turndif / 100);	// Radiation slowly decays	\todo V 0.2.1+ handle this as a true game time effect; no saveload-purging of radiation
						if (radtmp < 0) radtmp = 0;
						rad[i][j] = radtmp;
					}
				}
			}
		}
		else if (sm.has_key("const_radiation") && fromJSON(sm["const_radiation"], radtmp)) {
			radtmp -= int(turndif / 100);	// Radiation slowly decays	\todo V 0.2.1+ handle this as a true game time effect; no saveload-purging of radiation
			if (radtmp < 0) radtmp = 0;
			for (int j = 0; j < SEEY; j++) {
				for (int i = 0; i < SEEX; i++) {
					rad[i][j] = radtmp;
				}
			}
		} else throw std::runtime_error("radiation data missing");

		if (sm.has_key("spawns")) sm["spawns"].decode(spawns);
		if (sm.has_key("vehicles")) sm["vehicles"].decode(vehicles);
		if (sm.has_key("computer")) fromJSON(sm["computer"], comp);
	}
	// Load items and traps and fields and spawn points and vehicles
	item it_tmp;
	std::string string_identifier;
	int itx, ity;
	do {
		is >> string_identifier; // "----" indicates end of this submap
		int t = 0;
		if (string_identifier == "I") {
			is >> itx >> ity >> std::ws;
			if (fromJSON(JSON(is), it_tmp)) {
				itm[itx][ity].push_back(it_tmp);
				if (it_tmp.active) active_item_count++;
				if (!it_tmp.contents.empty()) for (const auto& it : it_tmp.contents) if (it.active) active_item_count++;
			}
		} else if (string_identifier == "T") {
			is >> itx >> ity;
			// historical encoding was raw integer
			if (strchr("0123456789", (is >> std::ws).peek())) {
				// legacy
				is >> trp[itx][ity];
			} else {
				fromJSON(JSON(is), trp[itx][ity]);
			}
		} else if (string_identifier == "F") {
			is >> itx >> ity;
			fromJSON(JSON(is), fld[itx][ity]);
			field_count++;
		} else if ("----" == string_identifier) {
			is >> std::ws;	// to ensure we don't warn on trailing whitespace at end of file
			break;
		} else {
			debugmsg((std::string("Unrecognized map data key '")+ string_identifier+"'").c_str());
			std::string databuff;
			getline(is, databuff);
			debugmsg((std::string("discarding  '") + databuff + "'").c_str());
		}
	} while (!is.eof());
}


std::ostream& operator<<(std::ostream& os, const submap& src)
{	// partially JSON (need enough to not be finicky, but not so far that faking a database isn't too awful
	os << src.turn_last_touched << std::endl;

	JSON sm(JSON::object);
	JSON _tmp(JSON::array);

	// Dump the terrain.
	const auto first_terrain = src.ter[0][0];
	bool need_full = false;
	for (int j = 0; j < SEEY; j++) {
		JSON tmp2(JSON::array);
		for (int i = 0; i < SEEX; i++) {
			tmp2.push(JSON_key(src.ter[i][j]));
			if (first_terrain != src.ter[i][j]) need_full = true;
		}
		_tmp.push(std::move(tmp2));
	}
	if (need_full) sm.set("terrain", std::move(_tmp));
	else sm.set("const_terrain", JSON_key(first_terrain));

	// Dump the radiation
	_tmp.reset();
	const auto first_radiation = src.rad[0][0];
	need_full = false;
	for (int j = 0; j < SEEY; j++) {
		JSON tmp2(JSON::array);
		for (int i = 0; i < SEEX; i++) {
			tmp2.push(std::to_string(src.rad[i][j]));
			if (first_radiation != src.rad[i][j]) need_full = true;
		}
		_tmp.push(std::move(tmp2));
	}
	if (need_full) sm.set("radiation", std::move(_tmp));
	else sm.set("const_radiation", std::to_string(first_radiation));

	if (!src.spawns.empty()) sm.set("spawns", JSON::encode(src.spawns));
	if (!src.vehicles.empty()) sm.set("vehicles", JSON::encode(src.vehicles));
	if (src.comp.name != "") sm.set("comp", toJSON(src.comp));
	os << sm << std::endl;

	// following are naturally associative arrays w/point keys.  We don't have a particularly clear
	// JSON encoding so leave that as-is for now.

	// Items section; designate it with an I.  Then check itm[][] for each square
	//   in the grid and print the coords and the item's details.
	// Designate it with a C if it's contained in the prior item.
	// Also, this wastes space since we print the coords for each item, when we
	//   could be printing a list of items for each coord (except the empty ones)
	item tmp;
	for (int j = 0; j < SEEY; j++) {
		for (int i = 0; i < SEEX; i++) {
			for (const auto& it : src.itm[i][j])  {
				os << "I " << i I_SEP << j << std::endl;
				os << toJSON(it) << std::endl;
			}
		}
	}
	// Output the traps
	for (int j = 0; j < SEEY; j++) {
		for (int i = 0; i < SEEX; i++) {
			if (src.trp[i][j] != tr_null)
				os << "T " << i I_SEP << j I_SEP << toJSON(src.trp[i][j]) << std::endl;
		}
	}

	// Output the fields
	for (int j = 0; j < SEEY; j++) {
		for (int i = 0; i < SEEX; i++) {
			const field& tmpf = src.fld[i][j];
			if (tmpf.type != fd_null) os << "F " << i << " " << j << " " << toJSON(tmpf) << std::endl;
		}
	}

	return os << "----" << std::endl;
}

bool fromJSON(const JSON& _in, faction& dest)
{
	if (!_in.has_key("id") || !fromJSON(_in["id"], dest.id)) return false;	// \todo do we want to interpolate this key?
	if (_in.has_key("name")) fromJSON(_in["name"], dest.name);
	if (_in.has_key("values")) {
		cataclysm::JSON_parse<faction_value> _parse;
		std::vector<const char*> relay;
		_in["values"].decode(relay);
		if (!relay.empty()) dest.values = _parse(relay);
	}
	if (_in.has_key("goal")) fromJSON(_in["goal"], dest.goal);
	if (_in.has_key("job1")) fromJSON(_in["job1"], dest.job1);
	if (_in.has_key("job2")) fromJSON(_in["job2"], dest.job2);
	if (_in.has_key("u")) {
		const auto& tmp = _in["u"];
		if (tmp.has_key("likes")) fromJSON(tmp["likes"], dest.likes_u);
		if (tmp.has_key("respects")) fromJSON(tmp["respects"], dest.respects_u);
		if (tmp.has_key("known_by")) fromJSON(tmp["known_by"], dest.known_by_u);
	}
	if (_in.has_key("ethics")) {
		const auto& tmp = _in["ethics"];
		if (tmp.has_key("strength")) fromJSON(tmp["strength"], dest.strength);
		if (tmp.has_key("sneak")) fromJSON(tmp["sneak"], dest.sneak);
		if (tmp.has_key("crime")) fromJSON(tmp["crime"], dest.crime);
		if (tmp.has_key("cult")) fromJSON(tmp["cult"], dest.cult);
		if (tmp.has_key("good")) fromJSON(tmp["good"], dest.good);
	}
	if (_in.has_key("om")) fromJSON(_in["om"], dest.om);
	if (_in.has_key("map")) fromJSON(_in["map"], dest.map);
	if (_in.has_key("size")) fromJSON(_in["size"], dest.size);
	if (_in.has_key("power")) fromJSON(_in["power"], dest.power);
	return true;
}

JSON toJSON(const faction& src)
{
	cataclysm::JSON_parse<faction_value> _parse;
	JSON _faction;
	_faction.set("id", std::to_string(src.id));
	_faction.set("name", src.name);
	if (src.values) {
		auto tmp = _parse(src.values);
		if (!tmp.empty()) _faction.set("values", JSON::encode(tmp));
	}
	if (auto json = JSON_key(src.goal)) _faction.set("goal", json);
	if (auto json = JSON_key(src.job1)) _faction.set("job1", json);
	if (auto json = JSON_key(src.job2)) _faction.set("job2", json);
	{
		JSON tmp;
		tmp.set("likes", std::to_string(src.likes_u));
		tmp.set("respects", std::to_string(src.respects_u));
		if (src.known_by_u) tmp.set("known_by", "true");
		_faction.set("u", std::move(tmp));	// ultimately we'd like the same format for the PC, and NPCs
		tmp.set("strength", std::to_string(src.strength));
		tmp.set("sneak", std::to_string(src.sneak));
		tmp.set("crime", std::to_string(src.crime));
		tmp.set("cult", std::to_string(src.cult));
		tmp.set("good", std::to_string(src.good));
		_faction.set("ethics", tmp);
	}
	_faction.set("om", toJSON(src.om));
	_faction.set("map", toJSON(src.map));
	_faction.set("size", std::to_string(src.size));
	_faction.set("power", std::to_string(src.power));
	return _faction;
}

bool fromJSON(const JSON& _in, monster& dest)
{
	if (!_in.has_key("type") || !fromJSON(_in["type"], dest.type)) return false;
	if (_in.has_key("pos")) fromJSON(_in["pos"], dest.pos);
	if (_in.has_key("wand")) fromJSON(_in["wand"], dest.wand);
	if (_in.has_key("inv")) _in["inv"].decode(dest.inv);
	if (_in.has_key("effects")) _in["effects"].decode(dest.effects);
	if (_in.has_key("spawnmap")) fromJSON(_in["spawnmap"], dest.spawnmap);
	if (_in.has_key("spawnpos")) fromJSON(_in["spawnpos"], dest.spawnpos);
	if (_in.has_key("moves")) fromJSON(_in["moves"], dest.moves);
	if (_in.has_key("speed")) fromJSON(_in["speed"], dest.speed);
	if (_in.has_key("hp")) fromJSON(_in["hp"], dest.hp);
	if (_in.has_key("sp_timeout")) fromJSON(_in["sp_timeout"], dest.sp_timeout);
	if (_in.has_key("friendly")) fromJSON(_in["friendly"], dest.friendly);
	if (_in.has_key("anger")) fromJSON(_in["anger"], dest.anger);
	if (_in.has_key("morale")) fromJSON(_in["morale"], dest.morale);
	if (_in.has_key("faction_id")) loadFactionID(_in["faction_id"], dest.faction_id);
	if (_in.has_key("mission_id")) fromJSON(_in["mission_id"], dest.mission_id);	// \todo validate after re-ordering savegame loading
	if (_in.has_key("dead")) fromJSON(_in["dead"], dest.dead);
	if (_in.has_key("made_footstep")) fromJSON(_in["made_footstep"], dest.made_footstep);
	if (_in.has_key("unique_name")) fromJSON(_in["unique_name"], dest.unique_name);
	if (_in.has_key("plans")) _in["plans"].decode(dest.plans);
	return true;
}

JSON toJSON(const monster& src)
{
	JSON _monster(JSON::object);
	if (auto json = JSON_key((mon_id)src.type->id)) {
		_monster.set("type", json);
		_monster.set("pos", toJSON(src.pos));
		if (src.wand.live()) _monster.set("wand", toJSON(src.wand));
		if (!src.inv.empty()) _monster.set("inv", JSON::encode(src.inv));
		if (!src.effects.empty()) _monster.set("effects", JSON::encode(src.effects));
		if (src.is_static_spawn()) {
			_monster.set("spawnmap", toJSON(src.spawnmap));
			_monster.set("spawnpos", toJSON(src.spawnpos));
		}
		_monster.set("moves", std::to_string(src.moves));
		_monster.set("speed", std::to_string(src.speed));
		_monster.set("hp", std::to_string(src.hp));
		if (0 < src.sp_timeout) _monster.set("sp_timeout", std::to_string(src.sp_timeout));
		if (src.friendly) _monster.set("friendly", std::to_string(src.friendly));
		_monster.set("anger", std::to_string(src.anger));
		_monster.set("morale", std::to_string(src.morale));
		if (0 <= src.faction_id && faction::from_id(src.faction_id)) _monster.set("faction_id", std::to_string(src.faction_id));
		if (0 < src.mission_id && mission::from_id(src.mission_id)) _monster.set("mission_id", std::to_string(src.mission_id));
		if (src.dead) _monster.set("dead", "true");
		if (src.made_footstep) _monster.set("made_footstep", "true");
		if (!src.unique_name.empty()) _monster.set("unique_name", src.unique_name);
		if (!src.plans.empty()) _monster.set("effects", JSON::encode(src.plans));
	}
	return _monster;
}
#endif

// usage is when loading artifacts
itype::itype(const cataclysm::JSON& src)
: id(item::types.size()),rarity(0),price(0),name("none"),sym('#'),color(c_white),m1(MNULL),m2(MNULL),volume(0),weight(0),
  melee_dam(0),melee_cut(0),m_to_hit(0),item_flags(0),techniques(0)
{
	int tmp;
	if (src.has_key("rarity")) {
		fromJSON(src["rarity"], tmp);
		rarity = tmp;
	}
	if (src.has_key("price")) fromJSON(src["price"], price);
	if (src.has_key("name")) fromJSON(src["name"], name);
	if (src.has_key("description")) fromJSON(src["description"], description);
	if (src.has_key("sym")) fromJSON(src["sym"], sym);
	if (src.has_key("color")) fromJSON(src["color"], color);
	if (src.has_key("material")) {
		std::vector<material> tmp;
		src["material"].decode(tmp);
		if (!tmp.empty()) {
			m1 = tmp.front();
			if (1 < tmp.size()) m2 = tmp.back();
		}
	}
	if (src.has_key("volume")) fromJSON(src["volume"], volume);
	if (src.has_key("weight")) fromJSON(src["weight"], weight);
	if (src.has_key("melee_dam")) {
		fromJSON(src["melee_dam"], tmp);
		melee_dam = tmp;
	}
	if (src.has_key("melee_cut")) {
		fromJSON(src["melee_cut"], tmp);
		melee_cut = tmp;
	}
	if (src.has_key("m_to_hit")) {
		fromJSON(src["m_to_hit"], tmp);
		m_to_hit = tmp;
	}

	if (src.has_key("item_flags")) {
		cataclysm::JSON_parse<item_flag> _parse;
		std::vector<const char*> relay;
		src["item_flags"].decode(relay);
		if (!relay.empty()) item_flags = _parse(relay);
	}
	if (src.has_key("techniques")) {
		cataclysm::JSON_parse<technique_id> _parse;
		std::vector<const char*> relay;
		src["techniques"].decode(relay);
		if (!relay.empty()) techniques = _parse(relay);
	}
}

void itype::toJSON(JSON& dest) const
{
	// we don't do the id key due to our use case
	if (rarity) dest.set("rarity", std::to_string((int)rarity));
	if (price) dest.set("price", std::to_string(price));
	dest.set("name", name);
	if (!description.empty()) dest.set("description", description);
	dest.set("sym", std::string(1, sym));
	if (auto json = JSON_key(color)) dest.set("color", json);
	if (m1 || m2)
		{
		JSON tmp;
		if (auto json = JSON_key(m1)) tmp.push(json);
		if (auto json = JSON_key(m2)) tmp.push(json);
		dest.set("material", tmp);
		}
	if (volume) dest.set("volume", std::to_string(volume));
	if (weight) dest.set("weight", std::to_string(weight));
	if (melee_dam) dest.set("melee_dam", std::to_string((int)melee_dam));
	if (melee_cut) dest.set("melee_cut", std::to_string((int)melee_cut));
	if (m_to_hit) dest.set("m_to_hit", std::to_string((int)m_to_hit));
	if (item_flags) {
		cataclysm::JSON_parse<item_flag> _parse;
		dest.set("item_flags", JSON::encode(_parse(item_flags)));
	}
	if (techniques) {
		cataclysm::JSON_parse<technique_id> _parse;
		dest.set("techniques", JSON::encode(_parse(techniques)));
	}
}

// usage is when loading artifacts
it_armor::it_armor(const cataclysm::JSON& src)
: itype(src), covers(0), encumber(0), dmg_resist(0), cut_resist(0), env_resist(0), warmth(0), storage(0)
{
	int tmp;
	if (src.has_key("covers")) {
		fromJSON(src["covers"], tmp);
		covers = tmp;
	}
	if (src.has_key("encumber")) {
		fromJSON(src["encumber"], tmp);
		encumber = tmp;
	}
	if (src.has_key("dmg_resist")) {
		fromJSON(src["dmg_resist"], tmp);
		dmg_resist = tmp;
	}
	if (src.has_key("cut_resist")) {
		fromJSON(src["cut_resist"], tmp);
		cut_resist = tmp;
	}
	if (src.has_key("env_resist")) {
		fromJSON(src["env_resist"], tmp);
		env_resist = tmp;
	}
	if (src.has_key("warmth")) {
		fromJSON(src["warmth"], tmp);
		warmth = tmp;
	}
	if (src.has_key("storage")) {
		fromJSON(src["price"], tmp);
		storage = tmp;
	}
}

void it_armor::toJSON(JSON& dest) const
{
	itype::toJSON(dest);
	if (covers) dest.set("covers", std::to_string((int)covers));
	if (encumber) dest.set("encumber", std::to_string((int)encumber));
	if (dmg_resist) dest.set("dmg_resist", std::to_string((int)dmg_resist));
	if (cut_resist) dest.set("cut_resist", std::to_string((int)cut_resist));
	if (env_resist) dest.set("env_resist", std::to_string((int)env_resist));
	if (warmth) dest.set("warmth", std::to_string((int)warmth));
	if (storage) dest.set("storage", std::to_string((int)storage));
}

it_artifact_armor::it_artifact_armor(const cataclysm::JSON& src)
: it_armor(src)
{
	if (src.has_key("effects_worn")) src["effects_worn"].decode(effects_worn);
}

void it_artifact_armor::toJSON(JSON& dest) const
{
	it_armor::toJSON(dest);
	if (!effects_worn.empty()) dest.set("effects_worn", JSON::encode(effects_worn));
}

std::ostream& operator<<(std::ostream& os, const it_artifact_armor& src)
{
	JSON tmp;
	src.toJSON(tmp);
	return os << tmp;
}

it_tool::it_tool(const cataclysm::JSON& src)
: itype(src),ammo(AT_NULL), max_charges(0), def_charges(0), charges_per_use(0), turns_per_charge(0), revert_to(itm_null)
#ifndef SOCRATES_DAIMON
, use(&iuse::none)
#endif
{
	int tmp;
	if (src.has_key("ammo")) fromJSON(src["ammo"], ammo);
	if (src.has_key("max_charges")) fromJSON(src["max_charges"], max_charges);
	if (src.has_key("def_charges")) fromJSON(src["def_charges"], def_charges);
	if (src.has_key("charges_per_use")) {
		fromJSON(src["charges_per_use"], tmp);
		charges_per_use = tmp;
	}
	if (src.has_key("turns_per_charge")) {
		fromJSON(src["turns_per_charge"], tmp);
		turns_per_charge = tmp;
	}
	if (src.has_key("revert_to")) fromJSON(src["revert_to"], revert_to);
}

void it_tool::toJSON(JSON& dest) const
{
	itype::toJSON(dest);
	if (auto json = JSON_key(ammo)) dest.set("ammo", json);
	if (max_charges) dest.set("max_charges", std::to_string(max_charges));
	if (def_charges) dest.set("def_charges", std::to_string(def_charges));
	if (charges_per_use) dest.set("charges_per_use", std::to_string((int)charges_per_use));
	if (turns_per_charge) dest.set("turns_per_charge", std::to_string((int)turns_per_charge));
	if (auto json = JSON_key(revert_to)) dest.set("revert_to", json);
	// function pointers only would need a JSON representation if modding
}

it_artifact_tool::it_artifact_tool(const cataclysm::JSON& src)
: it_tool(src)
{
	if (src.has_key("charge_type")) fromJSON(src["charge_type"], charge_type);
	if (src.has_key("effects_wielded")) src["effects_wielded"].decode(effects_wielded);
	if (src.has_key("effects_activated")) src["effects_activated"].decode(effects_activated);
	if (src.has_key("effects_carried")) src["effects_carried"].decode(effects_carried);
#ifndef SOCRATES_DAIMON
	use = &iuse::artifact;
#endif
}

void it_artifact_tool::toJSON(JSON& dest) const
{
	it_tool::toJSON(dest);
	if (auto json = JSON_key(charge_type)) dest.set("charge_type", json);
	if (!effects_wielded.empty()) dest.set("effects_wielded", JSON::encode(effects_wielded));
	if (!effects_activated.empty()) dest.set("effects_activated", JSON::encode(effects_activated));
	if (!effects_carried.empty()) dest.set("effects_carried", JSON::encode(effects_carried));
}

std::ostream& operator<<(std::ostream& os, const it_artifact_tool& src)
{
	JSON tmp;
	src.toJSON(tmp);
	return os << tmp;
}

// staging these here
std::string it_artifact_tool::save_data()
{
	std::ostringstream data;
	data << "T " << *this;
	return data.str();
}

std::string it_artifact_armor::save_data()
{
	std::ostringstream data;
	data << "A " << *this;
	return data.str();
}

#ifndef SOCRATES_DAIMON
bool fromJSON(const JSON& src, disease& dest)
{
	if (!src.has_key("type") || !fromJSON(src["type"], dest.type)) return false;
	if (src.has_key("duration")) fromJSON(src["duration"], dest.duration);
	if (src.has_key("intensity")) fromJSON(src["intensity"], dest.intensity);
	return true;
}

JSON toJSON(const disease& src)
{
	JSON _disease;
	_disease.set("type", toJSON(src.type));
	_disease.set("duration", std::to_string(src.duration));
	_disease.set("intensity", std::to_string(src.intensity));
	return _disease;
}

JSON toJSON(const addiction& src)
{
	JSON _addiction;
	_addiction.set("type", toJSON(src.type));
	_addiction.set("intensity", std::to_string(src.intensity));
	_addiction.set("sated", std::to_string(src.sated));
	return _addiction;
}


bool fromJSON(const JSON& src, addiction& dest)
{
	if (!src.has_key("type") || !fromJSON(src["type"], dest.type)) return false;
	if (src.has_key("intensity")) fromJSON(src["intensity"], dest.intensity);
	if (src.has_key("sated")) fromJSON(src["sated"], dest.sated);
	return true;
}

JSON toJSON(const morale_point& src)
{
	JSON _morale;
	_morale.set("type", toJSON(src.type));
	if (0 != src.bonus) _morale.set("bonus", std::to_string(src.bonus));
	if (src.item_type) {
		if (auto json2 = JSON_key((itype_id)(src.item_type->id))) _morale.set("item", json2);	// artifacts ruled out until JSON encoding updated
	}
	return _morale;
}

bool fromJSON(const JSON& src, morale_point& dest)
{
	if (!src.has_key("type") || !fromJSON(src["type"], dest.type)) return false;
	if (src.has_key("bonus")) fromJSON(src["bonus"], dest.bonus);
	if (src.has_key("item")) fromJSON(src["item"], dest.item_type);
	return true;
}

inventory::inventory(const JSON& src)
{
	src.decode(items);
}

bool fromJSON(const JSON& src, inventory& dest)
{
	if (JSON::array != src.mode()) return false;
	dest = std::move(inventory(src));
	return true;
}

JSON toJSON(const inventory& src)
{
	JSON ret(JSON::array);
	if (!src.items.empty())
		for (auto& stack : src.items) {
			if (stack.empty()) continue;	// invariant failure
			ret.push(JSON::encode(stack));
		}
	return ret;
}

// We have an improper inheritance player -> npc (ideal difference would be the AI controller class, cf. Rogue Survivor game family
// -- but C++ is too close to the machine for the savefile issues to be easily managed.  Rely on data structures to keep 
// the save of a non-final class to hard drive to disambiguate.

JSON toJSON(const player& src)
{
	JSON ret;
	ret.set("pos", toJSON(src.pos));

	// V 0.2.1+
	auto GPS = overmap::toGPS(src.pos);
	ret.set("GPS_pos", toJSON(GPS));

	if (!src.name.empty()) ret.set("name", src.name);
	if (!src.male) ret.set("male", "false");
	if (src.in_vehicle) {
		ret.set("in_vehicle", "true");
		if (src.driving_recoil) ret.set("driving_recoil", std::to_string(src.driving_recoil));
	}
	if (src.underwater) {
		ret.set("underwater", "true");
		ret.set("oxygen", std::to_string(src.oxygen));	// 2019-08-07: only meaningful if underwater
	}
	// default-zero integer block without undue entanglements
	if (src.hunger) ret.set("hunger", std::to_string(src.hunger));
	if (src.thirst) ret.set("thirst", std::to_string(src.thirst));
	if (src.fatigue) ret.set("fatigue", std::to_string(src.fatigue));
	if (src.health) ret.set("health", std::to_string(src.health));
	if (src.stim) ret.set("stim", std::to_string(src.stim));
	if (src.pain) ret.set("pain", std::to_string(src.pain));
	if (src.pkill) ret.set("pkill", std::to_string(src.pkill));
	if (src.radiation) ret.set("radiation", std::to_string(src.radiation));
	if (src.xp_pool) ret.set("xp_pool", std::to_string(src.xp_pool));
	if (src.cash) ret.set("cash", std::to_string(src.cash));	// questionable (pre-Cataclysm cash...)
	if (src.recoil) ret.set("recoil", std::to_string(src.recoil));

	// integer block
	ret.set("scent", std::to_string(src.scent));
	ret.set("dodges_left", std::to_string(src.dodges_left));
	ret.set("blocks_left", std::to_string(src.blocks_left));
	ret.set("moves", std::to_string(src.moves));
	if (0 <= src.active_mission && src.active_mission<src.active_missions.size()) ret.set("active_mission_id", std::to_string(src.active_mission));

	// enumerations
	if (itm_null != src.last_item) ret.set("last_item", toJSON(src.last_item));
	if (itm_null != src.style_selected) ret.set("style_selected", toJSON(src.style_selected));

	// single objects
	if (src.inv.size()) ret.set("inv", toJSON(src.inv));
	if (src.weapon.type) {
		if (const auto json = JSON_key((itype_id)(src.weapon.type->id))) ret.set("weapon", toJSON(src.weapon));
	}
	if (const auto json = JSON_key(src.activity.type)) ret.set("activity", toJSON(src.activity));
	if (const auto json = JSON_key(src.backlog.type)) ret.set("backlog", toJSON(src.backlog));

	// normal arrays
	if (!src.addictions.empty()) ret.set("addictions", JSON::encode(src.addictions));
	if (!src.my_bionics.empty()) ret.set("bionics", JSON::encode(src.my_bionics));
	if (!src.illness.empty()) ret.set("illness", JSON::encode(src.illness));
	if (!src.morale.empty()) ret.set("morale", JSON::encode(src.morale));
	if (!src.styles.empty()) ret.set("styles", JSON::encode(src.styles));
	if (!src.worn.empty()) ret.set("worn", JSON::encode(src.worn));
	if (!src.active_missions.empty()) ret.set("active_missions", JSON::encode(src.active_missions));
	if (!src.completed_missions.empty()) ret.set("completed_missions", JSON::encode(src.completed_missions));
	if (!src.failed_missions.empty()) ret.set("failed_missions", JSON::encode(src.failed_missions));

	// exotic arrays
	ret.set("traits", JSON::encode<pl_flag>(src.my_traits, PF_MAX2));
	ret.set("mutations", JSON::encode<pl_flag>(src.my_mutations, PF_MAX2));
	ret.set("mutation_category_level", JSON::encode<mutation_category>(src.mutation_category_level, NUM_MUTATION_CATEGORIES));
	ret.set("sklevel", JSON::encode<skill>(src.sklevel, num_skill_types));
	ret.set("sk_exercise", JSON::encode<skill>(src.skexercise, num_skill_types));

	JSON current(JSON::object);
	JSON max(JSON::object);

	current.set("str", std::to_string(src.str_cur));
	max.set("str", std::to_string(src.str_max));
	current.set("dex", std::to_string(src.dex_cur));
	max.set("dex", std::to_string(src.dex_max));
	current.set("int", std::to_string(src.int_cur));
	max.set("int", std::to_string(src.int_max));
	current.set("per", std::to_string(src.per_cur));
	max.set("per", std::to_string(src.per_max));
	if (0 < src.max_power_level) {
		current.set("power_level", std::to_string(src.power_level));
		max.set("power_level", std::to_string(src.max_power_level));
	}
	current.set("hp", JSON::encode<hp_part>(src.hp_cur, num_hp_parts));
	max.set("hp", JSON::encode<hp_part>(src.hp_max, num_hp_parts));

	ret.set("current", current);
	ret.set("max", max);
	return ret;
}

player::player(const JSON& src) : player()
{
 if (src.has_key("pos")) fromJSON(src["pos"], pos);
 if (src.has_key("GPS_pos")) {	// provided by V0.2.1+
	 if (fromJSON(src["GPS_pos"], GPSpos)) {
		 // reality checks possible, but not here
	 }
 }	// missing GPSpos is covered over at game::load
 if (src.has_key("in_vehicle")) fromJSON(src["in_vehicle"], in_vehicle);
 if (src.has_key("activity")) fromJSON(src["activity"], activity);
 if (src.has_key("backlog")) fromJSON(src["backlog"], backlog);
 if (src.has_key("active_missions")) src["active_missions"].decode(active_missions);	// \todo validate after re-ordering savegame loading
 if (src.has_key("completed_missions")) src["completed_missions"].decode(completed_missions);
 if (src.has_key("failed_missions")) src["failed_missions"].decode(failed_missions);
 if (src.has_key("active_mission_id")) {
	 int tmp;
	 if (fromJSON(src["active_mission_id"], tmp)) {
		 if (0 <= tmp && active_missions.size() > tmp) active_mission = tmp;
	 }
 }
 if (src.has_key("name")) fromJSON(src["name"], name);
 if (src.has_key("male")) fromJSON(src["male"], male);
 if (src.has_key("traits")) src["traits"].decode<pl_flag>(my_traits, PF_MAX2);
 if (src.has_key("mutations")) src["mutations"].decode<pl_flag>(my_mutations, PF_MAX2);
 if (src.has_key("mutation_category_level")) src["mutation_category_level"].decode<mutation_category>(mutation_category_level, NUM_MUTATION_CATEGORIES);
 if (src.has_key("bionics")) src["bionics"].decode(my_bionics);
 if (src.has_key("current")) {
	 auto& tmp = src["current"];
	 if (tmp.has_key("str")) fromJSON(tmp["str"], str_cur);
	 if (tmp.has_key("dex")) fromJSON(tmp["dex"], dex_cur);
	 if (tmp.has_key("int")) fromJSON(tmp["int"], int_cur);
	 if (tmp.has_key("per")) fromJSON(tmp["per"], per_cur);
	 if (tmp.has_key("power_level")) fromJSON(tmp["power_level"], power_level);
	 if (tmp.has_key("hp")) tmp["hp"].decode<hp_part>(hp_cur, num_hp_parts);
 }
 if (src.has_key("max")) {
	 auto& tmp = src["max"];
	 if (tmp.has_key("str")) fromJSON(tmp["str"], str_max);
	 if (tmp.has_key("dex")) fromJSON(tmp["dex"], dex_max);
	 if (tmp.has_key("int")) fromJSON(tmp["int"], int_max);
	 if (tmp.has_key("per")) fromJSON(tmp["per"], per_max);
	 if (tmp.has_key("power_level")) fromJSON(tmp["power_level"], max_power_level);
	 if (tmp.has_key("hp")) tmp["hp"].decode<hp_part>(hp_max, num_hp_parts);
 }
 if (src.has_key("hunger")) fromJSON(src["hunger"], hunger);
 if (src.has_key("thirst")) fromJSON(src["thirst"], thirst);
 if (src.has_key("fatigue")) fromJSON(src["fatigue"], fatigue);
 if (src.has_key("health")) fromJSON(src["health"], health);
 if (src.has_key("underwater")) fromJSON(src["underwater"], underwater);
 if (src.has_key("oxygen")) fromJSON(src["oxygen"], oxygen);
 if (src.has_key("recoil")) fromJSON(src["recoil"], recoil);
 if (src.has_key("driving_recoil")) fromJSON(src["driving_recoil"], driving_recoil);
 if (src.has_key("scent")) fromJSON(src["scent"], scent);
 if (src.has_key("dodges_left")) fromJSON(src["dodges_left"], dodges_left);
 if (src.has_key("blocks_left")) fromJSON(src["blocks_left"], blocks_left);
 if (src.has_key("stim")) fromJSON(src["stim"], stim);
 if (src.has_key("pain")) fromJSON(src["pain"], pain);
 if (src.has_key("pkill")) fromJSON(src["pkill"], pkill);
 if (src.has_key("radiation")) fromJSON(src["radiation"], radiation);
 if (src.has_key("cash")) fromJSON(src["cash"], cash);
 if (src.has_key("moves")) fromJSON(src["moves"], moves);
 if (src.has_key("morale")) src["morale"].decode(morale);
 if (src.has_key("xp_pool")) fromJSON(src["xp_pool"], xp_pool);
 if (src.has_key("sklevel")) src["sklevel"].decode<skill>(sklevel, num_skill_types);
 if (src.has_key("skexercise")) src["skexercise"].decode<skill>(skexercise, num_skill_types);
 if (src.has_key("inv_sorted")) fromJSON(src["inv_sorted"], inv_sorted);
 if (src.has_key("inv")) fromJSON(src["inv"], inv);
 if (src.has_key("last_item")) fromJSON(src["last_item"], last_item);
 if (src.has_key("worn")) src["worn"].decode(worn);
 if (src.has_key("styles")) src["styles"].decode(styles);
 if (src.has_key("style_selected")) fromJSON(src["style_selected"], style_selected);
 if (src.has_key("weapon")) fromJSON(src["weapon"], weapon);
 if (src.has_key("illness")) src["illness"].decode(illness);
 if (src.has_key("addictions")) src["addictions"].decode(addictions);
}

JSON toJSON(const npc_opinion& src)
{
	JSON _opinion;
	_opinion.set("trust", std::to_string(src.trust));
	_opinion.set("fear", std::to_string(src.fear));
	_opinion.set("value", std::to_string(src.value));
	_opinion.set("anger", std::to_string(src.anger));
	_opinion.set("owed", std::to_string(src.owed));
	if (!src.favors.empty()) _opinion.set("favors", JSON::encode(src.favors));
	return _opinion;
}

bool fromJSON(const JSON& src, npc_opinion& dest)
{
	if (JSON::object != src.mode()) return false;
	if (src.has_key("trust")) fromJSON(src["trust"], dest.trust);
	if (src.has_key("fear")) fromJSON(src["fear"], dest.fear);
	if (src.has_key("value")) fromJSON(src["value"], dest.value);
	if (src.has_key("anger")) fromJSON(src["anger"], dest.anger);
	if (src.has_key("owed")) fromJSON(src["owed"], dest.owed);
	if (src.has_key("favors")) src["favors"].decode(dest.favors);
	return true;
}

JSON toJSON(const npc_chatbin& src)
{
	JSON _chatbin(JSON::object);
	if (auto json = JSON_key(src.first_topic)) {
		_chatbin.set("first_topic", json);
		// \todo: try to purge invalid mission ids before checking size, etc.
		if (0 <= src.mission_selected && (src.missions.size() > src.mission_selected || src.missions_assigned.size() > src.mission_selected)) {
			_chatbin.set("mission_selected", std::to_string(src.mission_selected));
		}
		_chatbin.set("tempvalue", std::to_string(src.tempvalue));
		if (!src.missions.empty()) _chatbin.set("missions", JSON::encode(src.missions));
		if (!src.missions_assigned.empty()) _chatbin.set("missions_assigned", JSON::encode(src.missions_assigned));
	}
	return _chatbin;
}

bool fromJSON(const JSON& src, npc_chatbin& dest)
{
	if (!src.has_key("first_topic") || !fromJSON(src["first_topic"], dest.first_topic)) return false;

	// \todo: verify whether missions are fully loaded before the chatbins are loaded
	// \todo: verify whether mission_selected and tempvalue are UI-local, thus not needed in the savefile
	if (src.has_key("mission_selected")) fromJSON(src["mission_selected"], dest.mission_selected);
	if (src.has_key("tempvalue")) fromJSON(src["tempvalue"], dest.tempvalue);
	if (src.has_key("missions")) src["missions"].decode(dest.missions);
	if (src.has_key("missions_assigned")) src["missions_assigned"].decode(dest.missions_assigned);
	return true;
}

JSON toJSON(const npc_personality& src)
{
	JSON _personality;
	_personality.set("aggression", std::to_string((int)src.aggression));
	_personality.set("bravery", std::to_string((int)src.bravery));
	_personality.set("collector", std::to_string((int)src.collector));
	_personality.set("altruism", std::to_string((int)src.altruism));
	return _personality;
}

bool fromJSON(const JSON& src, npc_personality& dest)
{
	if (JSON::object != src.mode()) return false;
	int tmp;
	if (src.has_key("aggression") && fromJSON(src["aggression"], tmp)) dest.aggression = (signed char)tmp;
	if (src.has_key("bravery") && fromJSON(src["bravery"], tmp)) dest.bravery = (signed char)tmp;
	if (src.has_key("collector") && fromJSON(src["collector"], tmp)) dest.collector = (signed char)tmp;
	if (src.has_key("altruism") && fromJSON(src["altruism"], tmp)) dest.altruism = (signed char)tmp;
	return true;
}

JSON toJSON(const npc_combat_rules& src)
{
	JSON _engage(JSON::object);
	if (auto json = JSON_key(src.engagement)) {
		_engage.set("engagement", json);
		if (!src.use_guns) _engage.set("use_guns", "false");
		if (!src.use_grenades) _engage.set("use_grenades", "false");
	}
	return _engage;
}

bool fromJSON(const JSON& src, npc_combat_rules& dest)
{
	if (!src.has_key("engagement") || !fromJSON(src["engagement"], dest.engagement)) return false;
	if (src.has_key("use_guns")) fromJSON(src["use_guns"], dest.use_guns);
	if (src.has_key("use_grenades")) fromJSON(src["use_grenades"], dest.use_grenades);
	return true;
}

JSON toJSON(const npc& src)
{
	JSON ret(toJSON(static_cast<const player&>(src)));	// subclass JSON

	// booleans
	if (src.fetching_item) {
		ret.set("fetching_item", "true");
		ret.set("it", toJSON(src.it));
	}
	if (src.has_new_items) ret.set("has_new_items", "true");
	if (src.marked_for_death) ret.set("marked_for_death", "true");
	if (src.dead) ret.set("dead", "true");

	// enums
	if (const auto json = JSON_key(src.attitude)) ret.set("attitude", json);
	if (const auto json = JSON_key(src.myclass)) ret.set("class", json);
	if (const auto json = JSON_key(src.mission)) ret.set("mission", json);

	// integers
	ret.set("id", std::to_string(src.id));
	if (0 < src.patience) ret.set("patience", std::to_string(src.patience));
	if (src.my_fac) ret.set("faction_id", std::to_string(src.my_fac->id));	// \todo XXX should reality-check the faction id

	// toJSON
	ret.set("personality", toJSON(src.personality));
	ret.set("op_of_u", toJSON(src.op_of_u));
	auto tmp = toJSON(src.chatbin);
	if (tmp.has_key("first_topic")) ret.set("chatbin", tmp);
	tmp = toJSON(src.combat_rules);
	if (tmp.has_key("engagement")) ret.set("combat_rules", tmp);
	if (src.wand.live()) ret.set("wand", toJSON(src.wand));
	if (src.pl.live()) ret.set("pl", toJSON(src.pl));
	if (src.has_destination()) ret.set("goal", toJSON(src.goal));

	// JSON::encode
	if (!src.path.empty()) ret.set("path", JSON::encode(src.path));
	if (!src.needs.empty()) ret.set("needs", JSON::encode(src.needs));
	if (src.flags) {
		cataclysm::JSON_parse<npc_flag> _parse;
		ret.set("flags", JSON::encode(_parse(src.flags)));
	}
	return ret;
}

// \todo consider inlining the JSON-based NPC constructor, or providing an alternate implementation for array decode
bool fromJSON(const JSON& src, npc& dest)
{
	dest = npc(src);
	return true;
}

npc::npc(const JSON& src)
: player(src),id(-1), attitude(NPCATT_NULL), myclass(NC_NONE), wand(point(0, 0), 0),
  pl(point(-1, -1), 0), it(-1, -1), goal(_ref<decltype(goal)>::invalid), fetching_item(false), has_new_items(false),
  my_fac(0), mission(NPC_MISSION_NULL), patience(0), marked_for_death(false), dead(false), flags(0)
{
	if (src.has_key("id")) fromJSON(src["id"], id);
	if (src.has_key("attitude")) fromJSON(src["attitude"], attitude);
	if (src.has_key("class")) fromJSON(src["class"], myclass);
	if (src.has_key("wand")) fromJSON(src["wand"], wand);
	if (src.has_key("pl")) fromJSON(src["pl"], pl);
	if (src.has_key("it")) fromJSON(src["it"], it);
	if (src.has_key("goal") && !fromJSON(src["goal"], goal)) {
		// V0.2.1- was point rather than OM_loc
		point tmp;
		if (fromJSON(src["goal"], tmp)) goal.second = tmp;
	}
	if (src.has_key("fetching_item")) fromJSON(src["fetching_item"], fetching_item);
	if (src.has_key("has_new_items")) fromJSON(src["has_new_items"], has_new_items);
	if (src.has_key("path")) src["path"].decode(path);
	if (src.has_key("faction_id")) loadFactionID(src["faction_id"], my_fac);
	if (src.has_key("mission")) fromJSON(src["mission"], mission);
	if (src.has_key("personality")) fromJSON(src["personality"], personality);
	if (src.has_key("op_of_u")) fromJSON(src["op_of_u"], op_of_u);
	if (src.has_key("chatbin")) fromJSON(src["chatbin"], chatbin);
	if (src.has_key("patience")) fromJSON(src["patience"], patience);
	if (src.has_key("combat_rules")) fromJSON(src["combat_rules"], combat_rules);
	if (src.has_key("marked_for_death")) fromJSON(src["marked_for_death"], marked_for_death);
	if (src.has_key("dead")) fromJSON(src["dead"], dead);
	if (src.has_key("needs")) src["needs"].decode(needs);
	if (src.has_key("flags")) {
		cataclysm::JSON_parse<npc_flag> _parse;
		std::vector<const char*> relay;
		src["flags"].decode(relay);
		if (!relay.empty()) flags = _parse(relay);
	}

}
#endif
