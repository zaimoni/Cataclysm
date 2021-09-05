#include "item.h"
#ifndef SOCRATES_DAIMON
#include "game.h"
#else
#include "mtype.h"
#include "bodypart_enum.h"
#include "stl_typetraits.h"
#endif
#include "skill.h"
#include "recent_msg.h"

#include <sstream>

#ifndef NDEBUG
// #define ITEM_CONSTRUCTOR_INVARIANTS 1
#endif

std::string default_technique_name(technique_id tech); // itypedef.cpp

#ifdef ITEM_CONSTRUCTOR_INVARIANTS
#include "json.h"
cataclysm::JSON toJSON(const item& src);	// saveload.cpp

static void approve(const itype* type)	// need a stack trace from this
{
	if (!type) throw std::logic_error("null-type");
	if (!item::types.empty() && (0 >= type->id || item::types.size() <= type->id)) throw std::logic_error("type id out of range");
	if (0 < type->id && num_all_items > type->id && !JSON_key((itype_id)(type->id))) throw std::logic_error("non-artifact does not have JSON-valid type");
}
#endif

item::item() noexcept
: type(nullptr),corpse(nullptr),curammo(nullptr),invlet(0),charges(-1),active(false),
  damage(0),burnt(0),bday(0),owned(-1),poison(0),mission_id(-1),player_id(-1)
{
}

static void _bootstrap_item_charges(int& charges, const itype* const it)
{
	if (it->is_gun()) charges = 0;
	else if (it->is_ammo()) charges = dynamic_cast<const it_ammo*>(it)->count;
	else if (it->is_food()) {
		const it_comest* const comest = dynamic_cast<const it_comest*>(it);
		charges = (1 == comest->charges) ? -1 : comest->charges;
	}
	else if (const auto tool = it->is_tool()) {
		charges = (0 == tool->max_charges) ? -1 : tool->def_charges;;
	}
}

item::item(const itype* const it, unsigned int turn) noexcept
: type(it), corpse(nullptr), curammo(nullptr), invlet(0), charges(-1), active(false),
  damage(0), burnt(0), bday(turn), owned(-1), poison(0), mission_id(-1), player_id(-1)
{
#ifdef ITEM_CONSTRUCTOR_INVARIANTS
 approve(type);
#endif
 _bootstrap_item_charges(charges, it);
}

item::item(const itype* const it, unsigned int turn, char let) noexcept
: type(it),corpse(nullptr),curammo(nullptr),invlet(let),charges(-1),active(false),
  damage(0),burnt(0),bday(turn),owned(-1),poison(0),mission_id(-1),player_id(-1)
{
#ifdef ITEM_CONSTRUCTOR_INVARIANTS
 approve(type);
#endif
 _bootstrap_item_charges(charges, it);
}

// corpse constructor
item::item(unsigned int turn, int id) noexcept
: type(item::types[itm_corpse]), corpse(mtype::types[id]), curammo(nullptr), invlet(0), charges(-1), active(false),
  damage(0), burnt(0), bday(turn), owned(-1), poison(0), mission_id(-1), player_id(-1)
{
}

DEFINE_ACID_ASSIGN_W_MOVE(item)

void item::make(const itype* it)
{
#ifdef ITEM_CONSTRUCTOR_INVARIANTS
 approve(it);
#endif
 const bool was_null = is_null();
 type = it;
 contents.clear();
 if (was_null) _bootstrap_item_charges(charges, it);
}

// intentionally ignores as ui: invlet, name
bool operator==(const item& lhs, const item& rhs)
{
    return lhs.type == rhs.type && lhs.corpse == rhs.corpse && lhs.curammo == rhs.curammo && lhs.charges == rhs.charges && lhs.active == rhs.active
        && lhs.damage == rhs.damage && lhs.burnt == rhs.burnt && lhs.bday == rhs.bday && lhs.owned == rhs.owned && lhs.poison == rhs.poison
        && lhs.mission_id == rhs.mission_id && lhs.player_id == rhs.player_id && lhs.contents==rhs.contents;
}


int item::use_charges(int& qty) {
    if (0 >= charges) return 0; // no change
    if (charges <= qty) {
        qty -= charges;
        if (destroyed_at_zero_charges()) return -1; // item destroyed; must check qty separately
        else charges = 0;
        if (0 >= qty) return 1; // done
    } else {
        charges -= qty;
        return 1;   // done
    }
    return 0;
}

int item::use_charges(const itype_id it, int& qty)
{
    // Check contents first
    auto k = contents.size();
    while (0 < k) {
        auto& inside = contents[--k];
        if (it != inside.type->id) continue;
        if (auto code = inside.use_charges(qty)) {
            if (0 > code) {
                EraseAt(contents, k);
                if (0 < qty) continue;
            }
            return 1;
        }
    }
    // Now check the item itself
    if (it == type->id) return use_charges(qty);
    return 0;
}

bool item::is_null() const
{
 return (type == nullptr || type->id == 0);
}

item item::in_its_container() const
{
 if (is_software()) {
  item ret(item::types[itm_usb_drive], 0, invlet);
  ret.contents.push_back(*this);
  return ret;
 }
 const it_comest* const food = dynamic_cast<const it_comest*>(type);
 if (!food || !food->container) return *this;
 item ret(item::types[food->container], bday);
 ret.contents.push_back(*this);
 ret.invlet = invlet;
 return ret;
}

bool item::invlet_is_okay() const
{
 return ((invlet >= 'a' && invlet <= 'z') || (invlet >= 'A' && invlet <= 'Z'));
}

bool item::stacks_with(const item& rhs) const
{
 if ((corpse == nullptr && rhs.corpse != nullptr) ||
     (corpse != nullptr && rhs.corpse == nullptr)   )
  return false;

 if (corpse != nullptr && rhs.corpse != nullptr &&
     corpse->id != rhs.corpse->id)
  return false;

 if (type != rhs.type) return false;
 if (damage != rhs.damage) return false;
 if (active != rhs.active) return false;
 if (charges != rhs.charges) return false;
 if (type->expires() && bday != rhs.bday) return false;

 int i = contents.size();
 if (rhs.contents.size() != i) return false;

 while (0 <= --i) {
     if (!contents[i].stacks_with(rhs.contents[i])) return false;
 }

 return true;
}
 
void item::put_in(item payload)
{
 contents.push_back(payload);
}
 
std::string item::info(bool showtext)
{
 std::ostringstream dump;
 dump << " Volume: " << volume() << "    Weight: " << weight() << "\n" <<
         " Bash: " << int(type->melee_dam) <<
         (has_flag(IF_SPEAR) ? "  Pierce: " : "  Cut: ") << int(type->melee_cut) <<
         "  To-hit bonus: " << itype::force_sign(type->m_to_hit) << "\n" <<
         " Moves per attack: " << attack_time() << "\n";

 if (is_food()) {
  type->info(dump);
 } else if (is_food_container()) {
  contents[0].type->info(dump);
 } else if (is_ammo()) {
  type->info(dump);
 } else if (is_gun()) {
  const it_gun* const gun = dynamic_cast<const it_gun*>(type);
  int ammo_dam = 0, ammo_recoil = 0;
  bool has_ammo = (curammo != nullptr && charges > 0);
  if (has_ammo) {
   ammo_dam = curammo->damage;
   ammo_recoil = curammo->recoil;
  }
   
  dump << " Skill used: " << skill_link(gun->skill_used) << "\n Ammunition: " <<
          clip_size() << " rounds of " << ammo_name(ammo_type());

  dump << "\n Damage: ";
  if (has_ammo) dump << ammo_dam;
  dump << itype::force_sign(gun_damage(false));
  if (has_ammo) dump << " = " << gun_damage();

  dump << "\n Accuracy: " << int(100 - accuracy());

  dump << "\n Recoil: ";
  if (has_ammo) dump << ammo_recoil;
  dump << itype::force_sign(recoil(false));
  if (has_ammo) dump << " = " << recoil();

  dump << "\n Reload time: " << int(gun->reload_time);
  if (has_flag(IF_RELOAD_ONE)) dump << " per round";

  if (const int burst_s = burst_size()) dump << "\n Burst size: " << burst_s;
  else {
      if (gun->skill_used == sk_pistol && has_flag(IF_RELOAD_ONE)) dump << "\n Revolver.";
      else dump << "\n Semi-automatic.";
  }
  if (!contents.empty()) {
      dump << "\n";
      for (const auto& it : contents) dump << "\n+" << it.tname();
  }
 } else if (is_gunmod()) {
  type->info(dump);
 } else if (is_armor()) {
  type->info(dump);
 } else if (is_book()) {
  type->info(dump);
 } else if (is_tool()) {
  type->info(dump);
 } else if (is_style()) {
  type->info(dump);
 } else if (type->techniques != 0) {
  dump << "\n";
  for (int i = 1; i < NUM_TECHNIQUES; i++) {
   if (type->techniques & mfb(i))
    dump << " " << default_technique_name( technique_id(i) ) << "; ";
  }
 }

 if (showtext) {
  dump << "\n\n" << type->description << "\n";
  if (!contents.empty()) {
   if (is_gun()) {
    for(const auto& it : contents) dump << "\n " << it.type->description;
   } else
    dump << "\n " << contents[0].type->description;
   dump << "\n";
  }
 }
 return dump.str();
}

#ifndef SOCRATES_DAIMON
nc_color item::color(const player& u) const
{
 nc_color ret = c_ltgray;

 if (active) // Active items show up as yellow
  ret = c_yellow;
 else if (is_gun()) { // Guns are green if you are carrying ammo for them
  ammotype amtype = ammo_type();
  if (u.has_ammo(amtype)) ret = c_green;
 } else if (is_ammo()) { // Likewise, ammo is green if you have guns that use it
  ammotype amtype = ammo_type();
  if (u.weapon.gun_uses_ammo_type(amtype)) ret = c_green;
  else {
   for (size_t i = 0; i < u.inv.size(); i++) {
    if (u.inv[i].gun_uses_ammo_type(amtype)) {
     ret = c_green;
	 break;
    }
   }
  }
 } else if (is_book()) {
  const it_book* const tmp = dynamic_cast<const it_book*>(type);
  if (tmp->type !=sk_null && tmp->intel <= u.int_cur + u.sklevel[tmp->type] &&
      (tmp->intel == 0 || !u.has_trait(PF_ILLITERATE)) &&
      tmp->req <= u.sklevel[tmp->type] && tmp->level > u.sklevel[tmp->type])
   ret = c_ltblue;
 }
 return ret;
}

nc_color item::color_in_inventory(const player& u) const	// retain unused paramweter for later use
{
// Items in our inventory get colorized specially
 return active ? c_yellow : c_white;
}
#endif

std::string item::tname() const
{
 std::ostringstream ret;

 if (damage != 0) {
  std::string damtext;
  switch (type->m1) {
   case VEGGY:
   case FLESH:
    damtext = "partially eaten ";
    break;
   case COTTON:
   case WOOL:
    if (damage == -1) damtext = "reinforced ";
    if (damage ==  1) damtext = "ripped ";
    if (damage ==  2) damtext = "torn ";
    if (damage ==  3) damtext = "shredded ";
    if (damage ==  4) damtext = "tattered ";
    break;
   case LEATHER:
    if (damage == -1) damtext = "reinforced ";
    if (damage ==  1) damtext = "scratched ";
    if (damage ==  2) damtext = "cut ";
    if (damage ==  3) damtext = "torn ";
    if (damage ==  4) damtext = "tattered ";
    break;
   case KEVLAR:
    if (damage == -1) damtext = "reinforced ";
    if (damage ==  1) damtext = "marked ";
    if (damage ==  2) damtext = "dented ";
    if (damage ==  3) damtext = "scarred ";
    if (damage ==  4) damtext = "broken ";
    break;
   case PAPER:
    if (damage ==  1) damtext = "torn ";
    if (damage >=  2) damtext = "shredded ";
    break;
   case WOOD:
    if (damage ==  1) damtext = "scratched ";
    if (damage ==  2) damtext = "chipped ";
    if (damage ==  3) damtext = "cracked ";
    if (damage ==  4) damtext = "splintered ";
    break;
   case PLASTIC:
   case GLASS:
    if (damage ==  1) damtext = "scratched ";
    if (damage ==  2) damtext = "cut ";
    if (damage ==  3) damtext = "cracked ";
    if (damage ==  4) damtext = "shattered ";
    break;
   case IRON:
    if (damage ==  1) damtext = "lightly rusted ";
    if (damage ==  2) damtext = "rusted ";
    if (damage ==  3) damtext = "very rusty ";
    if (damage ==  4) damtext = "thoroughly rusted ";
    break;
   default:
    damtext = "damaged ";
  }
  ret << damtext;
 }

 if (volume() >= 4 && burnt >= volume() * 2) ret << "badly burnt ";
 else if (burnt > 0) ret << "burnt ";

 if (type->id == itm_corpse) {
  ret << corpse->name << " corpse";
  if (name != "") ret << " of " << name;
  return ret.str();
 } else if (type->id == itm_blood) {
  if (corpse == nullptr || corpse->id == mon_null) ret << "human blood";
  else ret << corpse->name << " blood";
  return ret.str();
 }

 {
 const auto n = contents.size();
 if (is_gun() && 0 < n) {
  ret << type->name;
  for (int i = 0; i < contents.size(); i++)
   ret << "+";
 } else if (1 == n) ret << type->name << " of " << contents[0].tname();
 else if (0 < n) ret << type->name << ", full";
 else ret << type->name;
 }

 const it_comest* food = nullptr;
 if (is_food()) food = dynamic_cast<const it_comest*>(type);
 else if (is_food_container()) food = dynamic_cast<const it_comest*>(contents[0].type);
 if (food != nullptr && food->is_expired(int(messages.turn) - bday))
  ret << " (rotten)";

 if (owned > 0) ret << " (owned)";
 return ret.str();
}

nc_color item::color() const
{
 if (type->id == itm_corpse) return corpse->color;
 return type->color;
}

int item::price() const
{
 int ret = type->price;
 for(const auto& it : contents) ret += it.price();
 return ret;
}

int item::weight() const
{
 static const int base_corpse_weight[mtype::MS_MAX] = {5, 60, 520, 2000, 4000};

 if (type->id == itm_corpse) {
  int ret = base_corpse_weight[corpse->size];
  if (made_of(VEGGY))
   ret /= 10;
  else if (made_of(IRON) || made_of(STEEL) || made_of(STONE))
   ret *= 5;
  return ret;
 }
 int ret = type->weight;
 if (is_ammo()) { 
  ret *= charges;
  ret /= 100;
 }
 for(const auto& it : contents) ret += it.weight();
 return ret;
}

int item::volume() const
{
 static const int corpse_volume[mtype::MS_MAX] = {2, 40, 75, 160, 600};

 if (type->id == itm_corpse) return corpse_volume[corpse->size];
 int ret = type->volume;
 if (is_gun()) {
  for (const auto& it : contents) ret += it.volume();
 }
 return type->volume;	// \todo: determine whether the above wasted CPU should be used here, or deleted
}

int item::volume_contained() const
{
 int ret = 0;
 for (const auto& it : contents) ret += it.volume();
 return ret;
}

int item::attack_time() const
{
 int ret = 65 + 4 * volume() + 2 * weight();
 return ret;
}

int item::damage_cut() const
{
 if (is_gun()) {
  for (const auto& it : contents) if (it.type->id == itm_bayonet) return it.type->melee_cut;
 }
 return type->melee_cut;
}

bool item::has_flag(item_flag f) const
{
 if (is_gun()) {
  for(const auto& it : contents) if (it.has_flag(f)) return true;
 }
 return (type->item_flags & mfb(f));
}

#ifndef SOCRATES_DAIMON
bool item::has_technique(technique_id tech, const player *p) const
{
 if (const auto style = is_style()) {
  for (const auto& m : style->moves) {
   if (m.tech == tech && (p == nullptr || p->sklevel[sk_unarmed] >= m.level)) return true;
  }
 }
 return (type->techniques & mfb(tech));
}

std::vector<technique_id> item::techniques() const
{
 std::vector<technique_id> ret;
 for (int i = 0; i < NUM_TECHNIQUES; i++) {
  if (has_technique( technique_id(i) ))
   ret.push_back( technique_id(i) );
 }
 return ret;
}
#endif

// sole caller is UI, so this doesn't need to be efficient 2021-01-05
bool item::rotten() const
{
    return is_food() && type->is_expired(int(messages.turn) - bday);
}

bool item::count_by_charges() const
{
 if (is_ammo()) return true;
 if (is_food()) return 1 < dynamic_cast<const it_comest*>(type)->charges;
 return false;
}

bool item::craft_has_charges() const
{
 if (count_by_charges()) return true;
 else if (ammo_type() == AT_NULL) return true;
 return false;
}

int item::weapon_value(const int skills[num_skill_types]) const
{
 int my_value = 0;
 if (is_gun()) {
  int gun_value = 14;
  const it_gun* const gun = dynamic_cast<const it_gun*>(type);
  gun_value += gun->dmg_bonus;
  gun_value += int(gun->burst / 2);
  gun_value += int(gun->clip / 3);
  gun_value -= int(gun->accuracy / 5);
  gun_value *= (.5 + (.3 * skills[sk_gun]));
  gun_value *= (.3 + (.7 * skills[gun->skill_used]));
  my_value += gun_value;
 }

 my_value += type->melee_value(skills);

 return my_value;
}

style_move item::style_data(technique_id tech) const
{
    if (const auto style = is_style()) {
        for (const auto& m : style->moves) if (m.tech == tech) return m;
    }

    return style_move();
}
 
#ifndef SOCRATES_DAIMON
bool item::is_two_handed(const player& u) const
{
 if (is_gun() && (dynamic_cast<const it_gun*>(type))->skill_used != sk_pistol) return true;
 return (weight() > u.str_cur * 4);
}
#endif

bool item::made_of(material mat) const
{
 if (type->id == itm_corpse) return (corpse->mat == mat);
 return (type->m1 == mat || type->m2 == mat);
}

bool item::conductive() const
{
 if ((type->m1 == IRON || type->m1 == STEEL || type->m1 == SILVER || type->m1 == MNULL) &&
     (type->m2 == IRON || type->m2 == STEEL || type->m2 == SILVER || type->m2 == MNULL))
  return true;
 return false;
}

bool item::destroyed_at_zero_charges() const
{
 return (is_ammo() || is_food());
}

bool is_flammable(material m)
{
	return (m == COTTON || m == WOOL || m == PAPER || m == WOOD || m == MNULL);
}

#ifndef SOCRATES_DAIMON
bool item::is_food(const player& u) const
{
 if (type->is_food()) return true;
 if (u.has_bionic(bio_batteries) && is_ammo() && (dynamic_cast<const it_ammo*>(type))->type == AT_BATT) return true;
 if (u.has_bionic(bio_furnace) && is_flammable(type->m1) && is_flammable(type->m2) && type->id != itm_corpse) return true;
 return false;
}

bool item::is_food_container(const player& u) const
{
 return (contents.size() >= 1 && contents[0].is_food(u));
}
#endif

bool item::is_food_container() const
{
 return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_drink() const
{
 return type->is_food() && type->m1 == LIQUID;
}

bool item::is_weap() const
{
 if (is_gun() || is_food() || is_ammo() || is_food_container() || is_armor() ||
     is_book() || is_tool())
  return false;
 return (type->melee_dam > 7 || type->melee_cut > 5);
}

bool item::is_bashing_weapon() const
{
 return (type->melee_dam >= 8);
}

bool item::is_cutting_weapon() const
{
 return (type->melee_cut >= 8 && !has_flag(IF_SPEAR));
}

bool item::is_book() const
{
/*
 if (type->is_macguffin()) {
  it_macguffin* mac = dynamic_cast<it_macguffin*>(type);
  return mac->readable;
 }
*/
 return type->is_book();
}

bool item::is_other() const
{
 return (!is_gun() && !is_ammo() && !is_armor() && !is_food() &&
         !is_food_container() && !is_tool() && !is_gunmod() && !is_bionic() &&
         !is_book() && !is_weap());
}

bool item::is_mission_item(int _id) const
{
    if (mission_id == _id) return true;
    for (const auto& it : contents) if (it.mission_id == _id) return true;
    return false;
}

#ifndef SOCRATES_DAIMON
int item::reload_time(const player &u) const
{
 int ret = 0;

 if (is_gun()) {
  const it_gun* const reloading = dynamic_cast<const it_gun*>(type);
  ret = reloading->reload_time;
  double skill_bonus = double(u.sklevel[reloading->skill_used]) * .075;
  if (skill_bonus > .75) skill_bonus = .75;
  ret -= double(ret) * skill_bonus;
 } else if (is_tool()) ret = 100 + volume() + weight();

 if (has_flag(IF_STR_RELOAD)) ret -= u.str_cur * 20;
 if (ret < 25) ret = 25;
 ret += u.encumb(bp_hands) * 30;
 return ret;
}
#endif

int item::clip_size() const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 if (gun->ammo != AT_40MM && charges > 0 && curammo->type == AT_40MM) return 1; // M203 mod in use
 int ret = gun->clip;
 for (const auto& it : contents) {
  if (const auto mod = it.is_gunmod()) {
   int bonus = (ret * mod->clip) / 100;
   ret += bonus;
  }
 }
 return ret;
}

int item::accuracy() const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->accuracy;
 for(const auto& it : contents)  {
  if (const auto mod = it.is_gunmod()) ret -= mod->accuracy;
 }
 ret += damage * 2;
 return ret;
}

int item::gun_damage(bool with_ammo) const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->dmg_bonus;
 if (with_ammo && curammo != nullptr) ret += curammo->damage;
 for(const auto& it : contents) {
  if (const auto mod = it.is_gunmod()) ret += mod->damage;
 }
 ret -= damage * 2;
 return ret;
}

int item::noise() const
{
 if (!is_gun()) return 0;
 int ret = curammo ? curammo->damage : 0;
 if (ret >= 5) ret += 20;
 for (const auto& it : contents) {
  if (const auto mod = it.is_gunmod()) ret += mod->loudness;
 }
 return ret;
}

int item::burst_size() const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->burst;
 for(const auto& it : contents) {
  if (const auto mod = it.is_gunmod()) ret += mod->burst;
 }
 return (ret < 0) ? 0 : ret;
}

int item::recoil(bool with_ammo) const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->recoil;
 if (with_ammo && curammo != nullptr) ret += curammo->recoil;
 for (const auto& it : contents) {
  if (const auto mod = it.is_gunmod()) ret += mod->recoil;
 }
 return ret;
}

#ifndef SOCRATES_DAIMON
int item::range(const player *p) const	// return value can be negative at this time
{
 if (!is_gun()) return 0;
 if (!curammo) return 0;
 int ret = curammo->range;

 if (p) {
	 if (has_flag(IF_STR8_DRAW)) {
		 if (p->str_cur < 4) return 0;
		 else if (p->str_cur < 8) ret -= 2 * (8 - p->str_cur);
	 }
	 else if (has_flag(IF_STR10_DRAW)) {
		 if (p->str_cur < 5) return 0;
		 else if (p->str_cur < 10) ret -= 2 * (10 - p->str_cur);
	 }
 }

 return ret;
}
#endif

ammotype item::ammo_type() const
{
 if (is_gun()) {
  ammotype ret = dynamic_cast<const it_gun*>(type)->ammo;
  for (int i = 0; i < contents.size(); i++) {
   if (const auto mod = contents[i].is_gunmod()) {
    if (mod->newtype != AT_NULL) ret = mod->newtype;
   }
  }
  return ret;
 } else if (const auto tool = is_tool()) return tool->ammo;
 else if (is_ammo()) return dynamic_cast<const it_ammo*>(type)->type;
 return AT_NULL;
}

template<itype_id N>
static bool contains(const std::vector<item>& contents)
{
    for (auto& it : contents) if (N == it.type->id) return true;
    return false;
}

bool item::gun_uses_ammo_type(ammotype am) const
{
    if (!is_gun()) return false;
    if (const auto uses_am = uses_ammo_type(); uses_am && uses_am == am) return true;
    if (AT_40MM == am && is_gun() && contains<itm_m203>(contents)) return true;
    return false;
}

void item::uses_ammo_type(itype_id src, std::vector<ammotype>& dest)
{
    const auto it = item(item::types[src],0);
    if (auto am = it.uses_ammo_type()) dest.push_back(am);
    else if (const auto mod = it.is_gunmod()) {
        if (mod->newtype) dest.push_back(mod->newtype);
        if (itm_m203 == src) dest.push_back(AT_40MM);
    }
}

ammotype item::uses_ammo_type() const
{
    if (is_gun()) {
        ammotype ret = dynamic_cast<const it_gun*>(type)->ammo;
        for (const auto& it : contents) {
            if (const auto mod = it.is_gunmod()) {
                if (mod->newtype != AT_NULL) ret = mod->newtype;
            }
        }
        return ret;
    }
    else if (const auto tool = is_tool()) return tool->ammo;
    return AT_NULL;
}

ammotype item::provides_ammo_type() const
{
    if (is_ammo()) return dynamic_cast<const it_ammo*>(type)->type;
    return AT_NULL;
}

#ifndef SOCRATES_DAIMON
int item::pick_reload_ammo(const player &u, bool interactive) const
{
    const auto reload_gun = type->is_gun();
 if (!reload_gun && !type->is_tool()) {
  debugmsg("RELOADING NON-GUN NON-TOOL");
  return -1;
 }

 std::vector<int> am;	// List of indicies of valid ammo

 if (reload_gun) {
  if (charges > 0) {
   const itype_id aid = itype_id(curammo->id);
   const auto& inv = u.inv;
   for (size_t i = 0; i < inv.size(); i++) {
    if (inv[i].type->id == aid) am.push_back(i);
   }
  } else {
   am = u.have_ammo(ammo_type());
   if (contains<itm_m203>(contents)) for (const auto grenade : u.have_ammo(AT_40MM)) am.push_back(grenade);
  }
 } else {
  am = u.have_ammo(ammo_type());
 }

 const size_t ub = am.size();
 if (0 >= ub) return -1; // arguably an invariant violation

 int index = -1;
 if (1 == ub || !interactive) index = am[0];
 else {// More than one option; list 'em and pick
  if (charges == 0) {
   // \todo? could get weird above lower-case z
   WINDOW* w_ammo = newwin(am.size() + 1, SCREEN_WIDTH, 0, 0);
   int ch;
   clear();
   // \todo? likely want extra width before the damage column
   mvwprintw(w_ammo, 0, 0, "\
Choose ammo type:         Damage     Armor Pierce     Range     Accuracy");
   for (int i = 0; i < ub; i++) {
    const it_ammo* ammo_type = dynamic_cast<const it_ammo*>(u.inv[am[i]].type);
    mvwaddch(w_ammo, i + 1, 1, i + 'a');
    mvwprintw(w_ammo, i + 1, 3, "%s (%d)", u.inv[am[i]].tname().c_str(),
                                           u.inv[am[i]].charges);
    mvwprintw(w_ammo, i + 1, 27, "%d", ammo_type->damage);
    mvwprintw(w_ammo, i + 1, 38, "%d", ammo_type->pierce);
    mvwprintw(w_ammo, i + 1, 55, "%d", ammo_type->range);
    mvwprintw(w_ammo, i + 1, 65, "%d", 100 - ammo_type->accuracy);
   }
   refresh();
   wrefresh(w_ammo);
   do ch = getch();
   while ((ch < 'a' || ch - 'a' > ub - 1) && ch != ' ' && ch != KEY_ESCAPE);
   werase(w_ammo);
   delwin(w_ammo);
   erase();
   index = (ch == ' ' || ch == KEY_ESCAPE) ? -1 : am[ch - 'a'];
  } else {
   int smallest = 500;
   for (int i = 0; i < am.size(); i++) {
    //if (u.inv[am[i]].type->id == curammo->id &&
        if (u.inv[am[i]].charges < smallest) {
     smallest = u.inv[am[i]].charges;
     index = am[i];
    }
   }
  }
 }
 return index;
}

bool item::reload(player &u, int index)
{
 bool single_load = false;
 int max_load = 1;
 if (is_gun()) {
  single_load = has_flag(IF_RELOAD_ONE);
  if (u.inv[index].ammo_type() == AT_40MM && ammo_type() != AT_40MM)
   max_load = 1;
  else
   max_load = clip_size();
 } else if (const auto tool = is_tool()) {
  single_load = false;
  max_load = tool->max_charges;
 }
 if (index > -1) {
// If the gun is currently loaded with a different type of ammo, reloading fails
  if (is_gun() && charges > 0 && curammo->id != u.inv[index].type->id)
   return false;
  if (is_gun()) {
   if (!u.inv[index].is_ammo()) {
    debugmsg("Tried to reload %s with %s!", tname().c_str(),
             u.inv[index].tname().c_str());
    return false;
   }
   curammo = dynamic_cast<const it_ammo*>((u.inv[index].type));
  }
  if (single_load || max_load == 1) {	// Only insert one cartridge!
   charges++;
   u.inv[index].charges--;
  } else {
   charges += u.inv[index].charges;
   u.inv[index].charges = 0;
   if (charges > max_load) {
 // More bullets than the clip holds, put some back
    u.inv[index].charges += charges - max_load;
    charges = max_load;
   }
  }
  if (0 == u.inv[index].charges) u.i_remn(index);
  return true;
 } else return false;
}
#endif

bool item::burn(int amount)
{
 burnt += amount;
 return (burnt >= volume() * 3);
}
