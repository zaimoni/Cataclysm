#ifndef _ITYPE_H_
#define _ITYPE_H_

#include "color.h"
#include "skill.h"
#include "artifact.h"
#include "c_bitmap.h"
#include "material_enum.h"
#include "enum_json.h"
#include "is_between.hpp"
#include <vector>
#include <initializer_list>
#ifndef SOCRATES_DAIMON
#include <optional>
#include <any>
#endif
// enum-only headers
#include "bionics_enum.h"
#include "itype_enum.h"
#include "pldata_enum.h"

// ideally this would be in a vector header extension
template<class T>
void EraseAt(std::vector<T>& x, size_t i) {
    x.erase(x.begin() + i);
    if (x.empty()) std::vector<T>().swap(x);    // handles problem with range-based for loops in MSVC++
}

#ifndef SOCRATES_DAIMON
class game;
class item;
class player;
class npc;
class pc;
#endif

namespace cataclysm {

	class JSON;

}

DECLARE_JSON_ENUM_SUPPORT(itype_id)

// C-level dependency on enum order
constexpr inline bool is_hard_liquor(auto x) { return is_between<itm_whiskey, itm_tequila>(x); }

struct item_drop_spec final
{
	itype_id what;
	int qty;
//	int mode; // will need this to specify formula type; for now implicitly requests one_in processing.  Credible scoped enumeration.
	int modifier;

	item_drop_spec(itype_id what, int qty = 1, int modifier = 0) noexcept : what(what), qty(qty), modifier(modifier) {}
	item_drop_spec(const item_drop_spec&) = default;
	~item_drop_spec() = default;
	item_drop_spec& operator=(const item_drop_spec&) = default;

	std::string to_s() const;
};

DECLARE_JSON_ENUM_SUPPORT(ammotype)

DECLARE_JSON_ENUM_BITFLAG_SUPPORT(item_flag)

DECLARE_JSON_ENUM_BITFLAG_SUPPORT(technique_id)

struct style_move
{
 std::string name;
 technique_id tech;
 int level;
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(ammotype t);
// Returns the default ammo for a category of ammo (e.g. "itm_00_shot")
itype_id default_ammo(ammotype guntype);

// forward-declares
struct it_ammo;
struct it_armor;
struct it_artifact_armor;
struct it_artifact_tool;
struct it_bionic;
struct it_book;
struct it_comest;
struct it_container;
struct it_gun;
struct it_gunmod;
struct it_macguffin;
struct it_software;
struct it_style;
struct it_tool;

struct itype
{
 int id;		// ID # that matches its place in master itype list
 				// Used for save files; aligns to itype_id above.
			    // plausibly should be int rather than itype_id due to random artifacts
 unsigned char rarity;	// How often it's found
 unsigned int  price;	// Its value
 
 std::string name;	// Proper name
 std::string description;// Flavor text
 
 char sym;		// Symbol on the map
 nc_color color;	// Color on the map (color.h)
 
 material m1;		// Main material
 material m2;		// Secondary material -- MNULL if made of just 1 thing
 
 unsigned int volume;	// Space taken up by this item
 unsigned int weight;	// Weight in quarter-pounds; is 64 lbs max ok?
 			// Also assumes positive weight.  No helium, guys!
 
 signed char melee_dam;	// Bonus for melee damage; may be a penalty
 signed char melee_cut;	// Cutting damage in melee
 signed char m_to_hit;	// To-hit bonus for melee combat; -5 to 5 is reasonable

 typename cataclysm::bitmap<NUM_ITEM_FLAGS>::type item_flags;
 typename cataclysm::bitmap<NUM_TECHNIQUES>::type techniques;
 
 virtual const it_comest* is_food() const    { return nullptr; }
 virtual const it_ammo* is_ammo() const    { return nullptr; }
 virtual const it_gun* is_gun() const     { return nullptr; }
 virtual const it_gunmod* is_gunmod() const  { return nullptr; }
 virtual const it_bionic* is_bionic() const  { return nullptr; }
 virtual const it_armor* is_armor() const   { return nullptr; }
 virtual const it_book* is_book() const    { return nullptr; }
 virtual const it_tool* is_tool() const    { return nullptr; }
 virtual const it_container* is_container() const { return nullptr; }
 virtual const it_software* is_software() const { return nullptr; }
 virtual const it_macguffin* is_macguffin() const { return nullptr; }
 virtual const it_style* is_style() const   { return nullptr; }
 virtual bool is_artifact() const { return false; }
 virtual const it_artifact_armor* is_artifact_armor() const { return nullptr; }
 virtual const it_artifact_tool* is_artifact_tool() const { return nullptr; }
 virtual bool count_by_charges() const { return false; }
 virtual std::string save_data() { return std::string(); }

 itype();
 itype(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
	 unsigned pitem_flags, unsigned ptechniques = 0);

 virtual ~itype() = default;

 virtual int melee_value(const int skills[num_skill_types]) const;
 virtual bool expires() const { return false; } // does it expire? C:Whales comestibles, but things like Zylon also expire
 virtual bool is_expired(int age) const { return false; }

 static std::string force_sign(int src);
 virtual void info(std::ostream& dest) const {}
protected:	// this is not a final type so these aren't public
 itype(const cataclysm::JSON& src);
 virtual void toJSON(cataclysm::JSON& dest) const;
};

// Includes food drink and drugs
struct it_comest : public itype
{
 signed char quench;	// Many things make you thirstier!
 unsigned char nutr;	// Nutrition imparted
 unsigned char spoils;	// How long it takes to spoil (hours / 600 turns)
 unsigned char addict;	// Addictiveness potential
 unsigned char charges;	// Defaults # of charges (drugs, loaf of bread? etc)
 signed char stim;
 signed char healthy;

 signed char fun;	// How fun its use is

 itype_id container;	// The container it comes in
 itype_id tool;		// Tool needed to consume (e.g. lighter for cigarettes)
 add_type add;				// Effects of addiction

#ifndef SOCRATES_DAIMON
private:
 void (*use_npc)(npc&, item&);	// waterfall/SSADM software lifecycle for these nine
 void (*use_pc)(pc&, item&);
 void (*use_player)(player&, item&);
 void (*use_npc_none)(npc&);
 void (*use_pc_none)(pc&);
 void (*use_player_none)(player&);
 void (*use_npc_type)(npc&, const it_comest& food);
 void (*use_pc_type)(pc&, const it_comest& food);
 void (*use_player_type)(player&, const it_comest& food);

public:
#endif

 it_comest(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,

	 signed char pquench, unsigned char pnutr, unsigned char pspoils,
	 signed char pstim, signed char phealthy, unsigned char paddict,
	 unsigned char pcharges, signed char pfun, itype_id pcontainer,
	 itype_id ptool,
	 add_type padd);

#ifndef SOCRATES_DAIMON
 it_comest(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,

	 signed char pquench, unsigned char pnutr, unsigned char pspoils,
	 signed char pstim, signed char phealthy, unsigned char paddict,
	 unsigned char pcharges, signed char pfun, itype_id pcontainer,
	 itype_id ptool,
	 add_type padd,
	 decltype(use_pc) puse
 );

 it_comest(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,

	 signed char pquench, unsigned char pnutr, unsigned char pspoils,
	 signed char pstim, signed char phealthy, unsigned char paddict,
	 unsigned char pcharges, signed char pfun, itype_id pcontainer,
	 itype_id ptool,
	 add_type padd,
	 decltype(use_player) puse
 );

 it_comest(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,

	 signed char pquench, unsigned char pnutr, unsigned char pspoils,
	 signed char pstim, signed char phealthy, unsigned char paddict,
	 unsigned char pcharges, signed char pfun, itype_id pcontainer,
	 itype_id ptool,
	 add_type padd,
	 decltype(use_player_none) puse
 );

 it_comest(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,

	 signed char pquench, unsigned char pnutr, unsigned char pspoils,
	 signed char pstim, signed char phealthy, unsigned char paddict,
	 unsigned char pcharges, signed char pfun, itype_id pcontainer,
	 itype_id ptool,
	 add_type padd,
	 decltype(use_player_type) puse
 );

 it_comest(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,

	 signed char pquench, unsigned char pnutr, unsigned char pspoils,
	 signed char pstim, signed char phealthy, unsigned char paddict,
	 unsigned char pcharges, signed char pfun, itype_id pcontainer,
	 itype_id ptool,
	 add_type padd,
	 decltype(use_pc_none) pc_use, decltype(use_npc_none) npc_use
 );
#endif

 const it_comest* is_food() const override { return this; }
 bool count_by_charges() const override { return charges > 1; }
 bool expires() const override { return 0 != spoils; }
 bool is_expired(int age) const override { return 0 != spoils && age > spoils * 600 /* HOURS(1) */; }

#ifndef SOCRATES_DAIMON
 void consumed_by(item& it, npc& u) const;
 void consumed_by(item& it, pc& u) const;
 std::optional<std::string> cannot_consume(const item& it, const player& u) const;
#endif

 void info(std::ostream& dest) const override;
};

struct it_ammo : public itype
{
 ammotype type;		// Enum of varieties (e.g. 9mm, shot, etc)
 unsigned char damage;	// Average damage done
 unsigned char pierce;	// Armor piercing; static reduction in armor
 unsigned char range;	// Maximum range
 signed char accuracy;	// Accuracy (low is good)
 unsigned char recoil;	// Recoil; modified by strength
 unsigned char count;	// Default charges

 it_ammo(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, unsigned pitem_flags,

	 ammotype ptype, unsigned char pdamage, unsigned char ppierce,
	 signed char paccuracy, unsigned char precoil, unsigned char prange,
	 unsigned char pcount);

 const it_ammo* is_ammo() const override { return this; }
 bool count_by_charges() const override { return id != itm_gasoline; }

 void info(std::ostream& dest) const override;
};

struct it_gun final : public itype
{
 ammotype ammo;
 skill skill_used;
 signed char dmg_bonus;
 signed char accuracy;
 signed char recoil;
 signed char durability;
 unsigned char burst;
 int clip;
 int reload_time;

 it_gun(int pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1, material pm2,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned pitem_flags,

	skill pskill_used, ammotype pammo, signed char pdmg_bonus,
	signed char paccuracy, signed char precoil, unsigned char pdurability,
        unsigned char pburst, int pclip, int preload_time);

 const it_gun* is_gun() const override { return this; }
};

struct it_gunmod final : public itype
{
 signed char accuracy, damage, loudness, clip, recoil, burst;
 ammotype newtype;
 unsigned acceptable_ammo_types : NUM_AMMO_TYPES;
 bool used_on_pistol;
 bool used_on_shotgun;
 bool used_on_smg;
 bool used_on_rifle;

 it_gunmod(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut,
	 signed char pm_to_hit, unsigned pitem_flags,

	 signed char paccuracy, signed char pdamage, signed char ploudness,
	 signed char pclip, signed char precoil, signed char pburst,
	 ammotype pnewtype, long a_a_t, bool pistol,
	 bool shotgun, bool smg, bool rifle);

 const it_gunmod* is_gunmod() const override { return this; }

 void info(std::ostream& dest) const override;
};

struct it_armor : public itype
{
 unsigned char covers; // Bitfield of enum body_part
 signed char encumber;
 unsigned char dmg_resist;
 unsigned char cut_resist;
 unsigned char env_resist; // Resistance to environmental effects
 signed char warmth;
 unsigned char storage;

 it_armor();
 it_armor(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
	 unsigned pitem_flags,

	 unsigned char pcovers, signed char pencumber,
	 unsigned char pdmg_resist, unsigned char pcut_resist,
	 unsigned char penv_resist, signed char pwarmth,
	 unsigned char pstorage);

 const it_armor* is_armor() const final { return this; }

 void info(std::ostream& dest) const override;
protected:	// this is not a final type so these aren't public
 it_armor(const cataclysm::JSON& src);
 void toJSON(cataclysm::JSON& dest) const override;
};

struct it_book final : public itype
{
 skill type;		// Which skill it upgrades
 unsigned char level;	// The value it takes the skill to
 unsigned char req;	// The skill level required to understand it
 signed char fun;	// How fun reading this is
 unsigned char intel;	// Intelligence required to read, at all
 unsigned char time;	// How long, in 10-turns (aka minutes), it takes to read
			// "To read" means getting 1 skill point, not all of em

 it_book(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
	 unsigned pitem_flags,

	 skill ptype, unsigned char plevel, unsigned char preq,
	 signed char pfun, unsigned char pintel, unsigned char ptime);

 const it_book* is_book() const override { return this; }

 void info(std::ostream& dest) const override;
};
 
enum container_flags {
 con_rigid,
 con_wtight,
 con_seals,
 num_con_flags
};

struct it_container : public itype
{
 unsigned char contains;	// Internal volume
 typename cataclysm::bitmap<num_con_flags>::type flags;

 it_container(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut,
	 signed char pm_to_hit, unsigned pitem_flags,

	 unsigned char pcontains, decltype(flags) pflags);

 const it_container* is_container() const override { return this; }
};

struct it_tool : public itype
{
 ammotype ammo;
 unsigned int max_charges;
 unsigned int def_charges;
 unsigned char charges_per_use;
 unsigned char turns_per_charge;
 itype_id revert_to;
#ifndef SOCRATES_DAIMON
private:
 void (*use_npc)(npc&, item&);	// waterfall/SSADM sofware lifecycle for these nine
 void (*use_pc)(pc&, item&);
 void (*use_player)(player&, item&);
 void (*use_item)(item&);
 void (*off_npc)(npc&, item&);
 void (*off_pc)(pc&, item&);
 void (*off_player)(player&, item&);
 void (*off_item)(item&);
 std::optional<std::any> (*can_use_npc)(const npc&);

public:
#endif

 // The constructors' buildout policy is YAGNI.  We have a combinatoric explosion here;
 // policy is that we want constructors to do their job (i.e., a fluent interface is a non-starter).
 it_tool();
#ifndef SOCRATES_DAIMON
protected:
	it_tool(decltype(use_pc) puse); // assistant for it_artifact_tool default constructor

public:
	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_pc) puse
	);

	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_player) puse
	);

	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_item) puse
	);

	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_pc) puse, decltype(off_pc) poff
	);

	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_player) puse, decltype(off_player) poff
	);

	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_item) puse, decltype(off_item) poff
	);

	it_tool(int pid, unsigned char prarity, unsigned int pprice,
		std::string pname, std::string pdes,
		char psym, nc_color pcolor, material pm1, material pm2,
		unsigned short pvolume, unsigned short pweight,
		signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
		unsigned pitem_flags,

		unsigned int pmax_charges, unsigned int pdef_charges,
		unsigned char pcharges_per_use, unsigned char pturns_per_charge,
		ammotype pammo, itype_id prevert_to,
		decltype(use_pc) use_pc, decltype(use_npc) use_npc, decltype(can_use_npc) can_use_npc
	);

#endif
 it_tool(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
	 unsigned pitem_flags,

	 unsigned int pmax_charges, unsigned int pdef_charges,
	 unsigned char pcharges_per_use, unsigned char pturns_per_charge,
	 ammotype pammo, itype_id prevert_to
 );

 const it_tool* is_tool() const final { return this; }

#ifndef SOCRATES_DAIMON
 void used_by(item& it, npc& u) const;
 void used_by(item& it, pc& u) const;
 void turned_off_by(item& it, npc& u) const;
 void turned_off_by(item& it, pc& u) const;
 std::optional<std::any> is_relevant(const item& it, const npc& _npc) const;
 std::optional<std::string> cannot_use(const item& it, const player& u) const;
#endif

 void info(std::ostream& dest) const override;
protected:	// this is not a final type so these aren't public
	it_tool(const cataclysm::JSON& src);
#ifndef SOCRATES_DAIMON
	it_tool(const cataclysm::JSON& src, decltype(use_pc) puse);
#endif
	void toJSON(cataclysm::JSON& dest) const override;
};
        
struct it_bionic final : public itype
{
 std::vector<bionic_id> options;
 int difficulty;

 it_bionic(int pid, std::string pname, unsigned char prarity, unsigned int pprice,
	 char psym, nc_color pcolor, int pdifficulty,
	 std::string pdes, const std::initializer_list<bionic_id>& _opts);

 const it_bionic* is_bionic() const override { return this; }
};

struct it_macguffin final : public itype
{
 bool readable; // If true, activated with 'R'
#ifndef SOCRATES_DAIMON
private:
 void (*use_pc)(pc&, item&);

public:
#endif

 it_macguffin(int pid, unsigned char prarity, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor, material pm1, material pm2,
	 unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut,
	 signed char pm_to_hit, unsigned pitem_flags,

	 bool preadable
#ifndef SOCRATES_DAIMON
	 , decltype(use_pc) puse
#endif
 );

#ifndef SOCRATES_DAIMON
 void used_by(item& it, pc& u) const;
#endif

 const it_macguffin* is_macguffin() const override { return this; }
};

struct it_software : public itype
{
 software_type swtype; // 2020-04-03 not wired in
 int power; // 2020-04-03 not wired in

 it_software(int pid, unsigned int pprice,
	 std::string pname, std::string pdes,
	 char psym, nc_color pcolor,
	 software_type pswtype, int ppower);

 const it_software* is_software() const override { return this; }
};

struct it_style final : public itype
{
 using base = itype; // Cf. Objective-C

 std::vector<style_move> moves;
 
 it_style(int pid, std::string pname, std::string pdes, char psym, nc_color pcolor,
	 signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
	 const std::initializer_list<style_move>& moves);


 const it_style* is_style() const override { return this; }

 int melee_value(const int skills[num_skill_types]) const override;

 void info(std::ostream& dest) const override;

 const style_move* data(technique_id src) const;
};

struct it_artifact_tool final : public it_tool
{
 art_charge charge_type;
 std::vector<art_effect_passive> effects_wielded;
 std::vector<art_effect_active>  effects_activated;
 std::vector<art_effect_passive> effects_carried;

 it_artifact_tool(const cataclysm::JSON& src);
 friend std::ostream& operator<<(std::ostream& os, const it_artifact_tool& src);
 void toJSON(cataclysm::JSON& dest) const override;

 it_artifact_tool();
 it_artifact_tool(int pid, unsigned int pprice, std::string pname,
	 std::string pdes, char psym, nc_color pcolor, material pm1,
	 material pm2, unsigned short pvolume, unsigned short pweight,
	 signed char pmelee_dam, signed char pmelee_cut,
	 signed char pm_to_hit, unsigned pitem_flags);

 bool is_artifact() const override { return true; }
 const it_artifact_tool* is_artifact_tool() const override { return this; }
 std::string save_data() override;
};

struct it_artifact_armor final : public it_armor
{
 std::vector<art_effect_passive> effects_worn;

 it_artifact_armor() = default;
 it_artifact_armor(int pid, unsigned int pprice, std::string pname,
                   std::string pdes, char psym, nc_color pcolor, material pm1,
                   material pm2, unsigned short pvolume, unsigned short pweight,
                   signed char pmelee_dam, signed char pmelee_cut,
                   signed char pm_to_hit, unsigned pitem_flags,

                   unsigned char pcovers, signed char pencumber,
                   unsigned char pdmg_resist, unsigned char pcut_resist,
                   unsigned char penv_resist, signed char pwarmth,
                   unsigned char pstorage);
 it_artifact_armor(const cataclysm::JSON& src);
 friend std::ostream& operator<<(std::ostream& os, const it_artifact_armor& src);
 void toJSON(cataclysm::JSON& dest) const override;

 bool is_artifact() const override { return true; }
 const it_artifact_armor* is_artifact_armor() const override { return this; }
 std::string save_data() override;
};

// artifact.h is a zero-dependency header so catch these here
DECLARE_JSON_ENUM_SUPPORT(art_charge)
DECLARE_JSON_ENUM_SUPPORT(art_effect_active)
DECLARE_JSON_ENUM_SUPPORT(art_effect_passive)

#endif
