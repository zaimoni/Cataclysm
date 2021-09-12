#ifndef _ITEM_H_
#define _ITEM_H_

#include "itype.h"

struct mtype;
#ifndef SOCRATES_DAIMON
class player;
#endif

#include <variant>
#include <optional>

class item
{
public:
 static std::vector<itype*> types; // \todo? really should be itype::types but time cost to migrate looks excessive
 static const item null;

 const itype*   type;	// \todo more actively enforce non-null constraint
 const mtype*   corpse;
 const it_ammo* curammo;

 std::vector<item> contents;

 std::string name;
 char invlet;           // Inventory letter
 int charges;
 bool active;           // If true, it has active effects to be processed
 signed char damage;    // How much damage it's sustained; generally, max is 5
 char burnt;		// How badly we're burnt
 unsigned int bday;     // The turn on which it was created
 int owned;		// UID of NPC owner; 0 = player, -1 = unowned
 int poison;		// How badly poisoned is it?

 int mission_id;// Refers to a mission in game's master list
 int player_id;	// Only give a mission to the right player!	(dead field, this would be for multi-PC case)

 item() noexcept;
 item(const itype* it, unsigned int turn) noexcept;
 item(const itype* it, unsigned int turn, char let) noexcept;
 explicit item(unsigned int turn, int id = 0) noexcept;	// corpse constructor; 0 is the value of mon_null
 item(const item& src) = default;
 item(item&& src) = default;
 ~item() = default;
 void make(const itype* it);

 item& operator=(const item& src);
 item& operator=(item&& src) = default;

 friend bool operator==(const item& lhs, const item& rhs);
 friend bool operator!=(const item& lhs, const item& rhs) { return !(lhs==rhs); };

 std::optional<std::pair<itype_id, int> > my_preferred_container() const;
// returns the default container of this item, with this item in it
 item in_its_container() const;

#ifndef SOCRATES_DAIMON
 nc_color color(const player& u) const;
 nc_color color_in_inventory(const player& u) const;
#endif
 std::string tname() const;
 bool burn(int amount = 1); // Returns true if destroyed

// Firearm specifics
#ifndef SOCRATES_DAIMON
 int reload_time(const player &u) const;
#endif
 int clip_size() const;
 int accuracy() const;
 int gun_damage(bool with_ammo = true) const;
 int noise() const;
 int burst_size() const;
 int recoil(bool with_ammo = true) const;
#ifndef SOCRATES_DAIMON
 int range(const player* p = nullptr) const;
#endif
 std::optional<std::string> cannot_reload() const;
 std::optional<std::variant<const it_gun*, const it_tool* > > can_reload() const;
 ammotype ammo_type() const;
 bool gun_uses_ammo_type(ammotype am) const;
 static void uses_ammo_type(itype_id src, std::vector<ammotype>& dest);
 ammotype uses_ammo_type() const;
 ammotype provides_ammo_type() const;
#ifndef SOCRATES_DAIMON
 int pick_reload_ammo(const player &u, bool interactive) const;
 bool reload(player &u, int index);
#endif

 std::string info(bool showtext = false);	// Formatted for human viewing
 char symbol() const { return type->sym; }
 nc_color color() const;
 int price() const;

 bool invlet_is_okay() const;
 bool stacks_with(const item& rhs) const;
 void put_in(item&& payload);
 void put_in(const item& payload) { put_in(item(payload)); }

 int use_charges(itype_id it, int& qty); // 0: no op/continue; 1: done; -1: destroyed, may or may not be done
private:
 int use_charges(int& qty); // Cf. above
public:

 int weight() const;
 int volume() const;
 int volume_contained() const;
 int attack_time() const;
 int damage_bash() const { return type->melee_dam; }
 int damage_cut() const;
 bool has_flag(item_flag f) const;
#ifndef SOCRATES_DAIMON
 bool has_technique(technique_id t, const player* p = nullptr) const;
 std::vector<technique_id> techniques() const;
#endif
 bool count_by_charges() const;
 bool craft_has_charges() const;
 bool rotten() const;

// Our value as a weapon, given particular skills
 int  weapon_value(const int skills[num_skill_types]) const;
// As above, but discounts its use as a ranged weapon
 int  melee_value(const int skills[num_skill_types]) const { return type->melee_value(skills); }
// Returns the data associated with tech, if we are an it_style
 style_move style_data(technique_id tech) const;
#ifndef SOCRATES_DAIMON
 bool is_two_handed(const player& u) const;
#endif
 bool made_of(material mat) const;
 bool conductive() const; // Electricity
 bool destroyed_at_zero_charges() const;
// Most of the is_whatever() functions call the same function in our itype
 bool is_null() const; // True if type is null, or points to the null item (id == 0)
#ifndef SOCRATES_DAIMON
 std::optional<std::variant<const it_comest*, const it_ammo*, const item*> > is_food2(const player& u) const;// Some non-food items are food to certain players
 auto is_food_container2(const player& u) const { return contents.empty() ? std::nullopt : contents[0].is_food2(u); }  // Ditto
 bool is_food(const player& u) const;// Some non-food items are food to certain players
 bool is_food_container(const player& u) const;  // Ditto
#endif
 auto is_food() const { return type->is_food(); };	// Ignoring the ability to eat batteries, etc.
 auto is_food_container() const { return contents.empty() ? nullptr : contents[0].is_food(); }      // Ignoring the ability to eat batteries, etc.
#if DEAD_FUNC
 bool is_drink() const;
#endif
 bool is_weap() const;
 bool is_bashing_weapon() const;
 bool is_cutting_weapon() const;
 auto is_gun() const { return type->is_gun(); }
 auto is_gunmod() const { return type->is_gunmod(); }
 auto is_bionic() const { return type->is_bionic(); }
 auto is_ammo() const { return type->is_ammo(); }
 auto is_armor() const { return type->is_armor(); }
 auto is_book() const { return type->is_book(); }
 auto is_container() const { return type->is_container(); }
 auto is_tool() const { return type->is_tool(); }
 auto is_software() const { return type->is_software(); }
 auto is_macguffin() const { return type->is_macguffin(); }
 auto is_style() const { return type->is_style(); }
 bool is_other() const; // Doesn't belong in other categories
 bool is_artifact() const { return type->is_artifact(); }
 auto is_artifact_armor() const { return type->is_artifact_armor(); }
 auto is_artifact_tool() const { return type->is_artifact_tool(); }

 bool is_mission_item(int _id) const;

 static void init();
};


#endif
