#ifndef _ITEM_H_
#define _ITEM_H_

#include "itype.h"
#include "mtype.h"

class player;
class npc;

class item
{
public:
 static std::vector<itype*> types;
 static const item null;

 const itype*   type;
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
 int player_id;	// Only give a mission to the right player!

 item();
 item(const itype* it, unsigned int turn);
 item(const itype* it, unsigned int turn, char let);
 explicit item(unsigned int turn, int id = 0);	// corpse constructor; 0 is the value of mon_null
 item(std::string itemdata, game *g);
 ~item() = default;
 void make(itype* it);

// returns the default container of this item, with this item in it
 item in_its_container();

 nc_color color(player *u) const;
 nc_color color_in_inventory(player *u);
 std::string tname(game *g = NULL) const; // g needed for rotten-test
 void use(player &u);
 bool burn(int amount = 1); // Returns true if destroyed

// Firearm specifics
 int reload_time(const player &u) const;
 int clip_size();
 int accuracy();
 int gun_damage(bool with_ammo = true);
 int noise();
 int burst_size();
 int recoil(bool with_ammo = true);
 int range(const player *p = NULL) const;
 ammotype ammo_type() const;
 int pick_reload_ammo(player &u, bool interactive) const;
 bool reload(player &u, int index);

 std::string save_info();	// Formatted for save files
 void load_info(std::string data, game *g);
 std::string info(bool showtext = false);	// Formatted for human viewing
 char symbol() const { return type->sym; }
 nc_color color() const;
 int price() const;

 bool invlet_is_okay();
 bool stacks_with(item rhs);
 void put_in(item payload);

 int weight() const;
 int volume() const;
 int volume_contained();
 int attack_time();
 int damage_bash() const { return type->melee_dam; }
 int damage_cut() const;
 bool has_flag(item_flag f) const;
 bool has_technique(technique_id t, player *p = NULL);
 std::vector<technique_id> techniques();
 bool goes_bad();
 bool count_by_charges() const;
 bool craft_has_charges();
 bool rotten(game *g);

// Our value as a weapon, given particular skills
 int  weapon_value(const int skills[num_skill_types]) const;
// As above, but discounts its use as a ranged weapon
 int  melee_value (const int skills[num_skill_types]) const;
// Returns the data associated with tech, if we are an it_style
 style_move style_data(technique_id tech);
 bool is_two_handed(player *u);
 bool made_of(material mat) const;
 bool conductive(); // Electricity
 bool destroyed_at_zero_charges();
// Most of the is_whatever() functions call the same function in our itype
 bool is_null() const; // True if type is NULL, or points to the null item (id == 0)
 bool is_food(player *u) const;// Some non-food items are food to certain players
 bool is_food_container(player *u) const;  // Ditto
 bool is_food() const { return type->is_food(); };	// Ignoring the ability to eat batteries, etc.
 bool is_food_container() const;      // Ignoring the ability to eat batteries, etc.
 bool is_drink();
 bool is_weap() const;
 bool is_bashing_weapon();
 bool is_cutting_weapon();
 bool is_gun() const;
 bool is_gunmod() const;
 bool is_bionic() const;
 bool is_ammo() const;
 bool is_armor() const;
 bool is_book() const;
 bool is_container() const;
 bool is_tool() const;
 bool is_software() const;
 bool is_macguffin() const;
 bool is_style() const;
 bool is_other() const; // Doesn't belong in other categories
 bool is_artifact() const;

 static void init();
};


#endif
