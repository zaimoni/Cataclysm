#include "item.h"
#include "game.h"
#include "recent_msg.h"

#include <sstream>

std::string default_technique_name(technique_id tech);

item::item()
{
 name = "";
 charges = -1;
 bday = 0;
 invlet = 0;
 damage = 0;
 burnt = 0;
 poison = 0;
 type = NULL;
 curammo = NULL;
 corpse = NULL;
 active = false;
 owned = -1;
 mission_id = -1;
 player_id = -1;
}

item::item(const itype* const it, unsigned int turn)
{
 type = it;
 bday = turn;
 name = "";
 invlet = 0;
 damage = 0;
 burnt = 0;
 poison = 0;
 active = false;
 curammo = NULL;
 corpse = NULL;
 owned = -1;
 mission_id = -1;
 player_id = -1;
 if (it == NULL) return;
 if (it->is_gun()) charges = 0;
 else if (it->is_ammo()) charges = dynamic_cast<const it_ammo*>(it)->count;
 else if (it->is_food()) {
  const it_comest* const comest = dynamic_cast<const it_comest*>(it);
  charges = (1 == comest->charges) ? -1 : comest->charges;
 } else if (it->is_tool()) {
  const it_tool* const tool = dynamic_cast<const it_tool*>(it);
  charges = (0 == tool->max_charges) ? -1 : tool->def_charges;;
 } else charges = -1;
}

item::item(const itype* const it, unsigned int turn, char let)
{
 type = it;
 bday = turn;
 name = "";
 damage = 0;
 burnt = 0;
 poison = 0;
 active = false;
 if (it->is_gun()) charges = 0;
 else if (it->is_ammo()) charges = dynamic_cast<const it_ammo*>(it)->count;
 else if (it->is_food()) {
  const it_comest* const comest = dynamic_cast<const it_comest*>(it);
  charges = (1 == comest->charges) ? -1 : comest->charges;
 } else if (it->is_tool()) {
  const it_tool* const tool = dynamic_cast<const it_tool*>(it);
  charges = (0 == tool->max_charges) ? -1 : tool->def_charges;
 } else charges = -1;
 curammo = NULL;
 corpse = NULL;
 owned = -1;
 invlet = let;
 mission_id = -1;
 player_id = -1;
}

// corpse constructor
item::item(unsigned int turn, int id)
{
	name = "";
	charges = -1;
	invlet = 0;
	damage = 0;
	burnt = 0;
	poison = 0;
	curammo = NULL;
	active = false;
	owned = -1;
	mission_id = -1;
	player_id = -1;

	type = item::types[itm_corpse];
	corpse = mtype::types[id];
	bday = turn;
}

item::item(const std::string& itemdata)
{
 load_info(itemdata);
}

void item::make(itype* it)
{
 type = it;
 contents.clear();
}

bool item::is_null() const
{
 return (type == NULL || type->id == 0);
}

item item::in_its_container() const
{
 if (is_software()) {
  item ret(item::types[itm_usb_drive], 0);
  ret.contents.push_back(*this);
  ret.invlet = invlet;
  return ret;
 }
 if (!is_food()) return *this;
 const it_comest* const food = dynamic_cast<const it_comest*>(type);
 if (!food) return *this;	// XXX dead code \todo verify
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
 if ((corpse == NULL && rhs.corpse != NULL) ||
     (corpse != NULL && rhs.corpse == NULL)   )
  return false;

 if (corpse != NULL && rhs.corpse != NULL &&
     corpse->id != rhs.corpse->id)
  return false;
  
 if (contents.size() != rhs.contents.size())
  return false;

 bool stacks = (type == rhs.type   && damage == rhs.damage  &&
	 active == rhs.active && charges == rhs.charges &&
	 (!goes_bad() || bday == rhs.bday));

 for (int i = 0; i < contents.size() && stacks; i++)
   stacks &= contents[i].stacks_with(rhs.contents[i]);

 return stacks;
}
 
void item::put_in(item payload)
{
 contents.push_back(payload);
}

std::string item::save_info() const
{
 if (type == NULL)
  debugmsg("Tried to save an item with NULL type!");
 int ammotmp = 0;
/* TODO: This causes a segfault sometimes, even though we check to make sure
 * curammo isn't NULL.  The crashes seem to occur most frequently when saving an
 * NPC, or when saving map data containing an item an NPC has dropped.
 */
 if (curammo != NULL)
  ammotmp = curammo->id;
 if (ammotmp < 0 || ammotmp > num_items)
  ammotmp = 0; // Saves us from some bugs
 std::stringstream dump;// (std::stringstream::in | std::stringstream::out);
 dump << " " << int(invlet) << " " << int(type->id) << " " <<  int(charges) <<
         " " << int(damage) << " " << int(burnt) << " " << poison << " " <<
         ammotmp << " " << owned << " " << int(bday);
 if (active)
  dump << " 1";
 else
  dump << " 0";
 if (corpse != NULL)
  dump << " " << corpse->id;
 else
  dump << " -1";
 dump << " " << mission_id << " " << player_id;

 std::string name_copy(name);

 size_t pos = name_copy.find_first_of("\n");
 while (pos != std::string::npos)  {
  name_copy.replace(pos, 1, "@@");
  pos = name_copy.find_first_of("\n");
 }
 dump << " '" << name_copy << "'";
 return dump.str();
}

void item::load_info(const std::string& data)
{
 std::stringstream dump;
 dump << data;
 int idtmp, ammotmp, lettmp, damtmp, burntmp, acttmp, corp;
 dump >> lettmp >> idtmp >> charges >> damtmp >> burntmp >> poison >> ammotmp >>
         owned >> bday >> acttmp >> corp >> mission_id >> player_id;
 corpse = (-1 != corp) ? mtype::types[corp] : NULL;
 getline(dump, name);
 if (name == " ''") name = "";
 else {
  size_t pos = name.find_first_of("@@");
  while (pos != std::string::npos)  {
   name.replace(pos, 2, "\n");
   pos = name.find_first_of("@@");
  }
  name = name.substr(2, name.size() - 3); // s/^ '(.*)'$/\1/
 }
 make(item::types[idtmp]);
 invlet = char(lettmp);
 damage = damtmp;
 burnt = burntmp;
 active = (acttmp == 1);
 curammo = (0 < ammotmp) ? dynamic_cast<it_ammo*>(item::types[ammotmp]) : NULL;
}
 
std::string item::info(bool showtext)
{
 std::stringstream dump;
 dump << " Volume: " << volume() << "    Weight: " << weight() << "\n" <<
         " Bash: " << int(type->melee_dam) <<
         (has_flag(IF_SPEAR) ? "  Pierce: " : "  Cut: ") <<
         int(type->melee_cut) << "  To-hit bonus: " <<
         (type->m_to_hit > 0 ? "+" : "" ) << int(type->m_to_hit) << "\n" <<
         " Moves per attack: " << attack_time() << "\n";

 if (is_food()) {

  const it_comest* const food = dynamic_cast<const it_comest*>(type);
  dump << " Nutrition: " << int(food->nutr) << "\n Quench: " <<
          int(food->quench) << "\n Enjoyability: " << int(food->fun) <<
          "\n Healthiness: " << int(food->healthy);

 } else if (is_food_container()) {

  const it_comest* food = dynamic_cast<const it_comest*>(contents[0].type);
  dump << " Nutrition: " << int(food->nutr) << "\n Quench: " <<
          int(food->quench) << "\n Enjoyability: " << int(food->fun);

 } else if (is_ammo()) {

  const it_ammo* const ammo = dynamic_cast<const it_ammo*>(type);
  dump << " Type: " << ammo_name(ammo->type) << "\n Damage: " <<
           int(ammo->damage) << "\n Armor-pierce: " << int(ammo->pierce) <<
           "\n Range: " << int(ammo->range) << "\n Accuracy: " <<
           int(100 - ammo->accuracy) << "\n Recoil: " << int(ammo->recoil);

 } else if (is_gun()) {

  const it_gun* const gun = dynamic_cast<const it_gun*>(type);
  int ammo_dam = 0, ammo_recoil = 0;
  bool has_ammo = (curammo != NULL && charges > 0);
  if (has_ammo) {
   ammo_dam = curammo->damage;
   ammo_recoil = curammo->recoil;
  }
   
  dump << " Skill used: " << skill_name(gun->skill_used) << "\n Ammunition: " <<
          clip_size() << " rounds of " << ammo_name(ammo_type());

  dump << "\n Damage: ";
  if (has_ammo) dump << ammo_dam;
  dump << (gun_damage(false) >= 0 ? "+" : "" ) << gun_damage(false);
  if (has_ammo) dump << " = " << gun_damage();

  dump << "\n Accuracy: " << int(100 - accuracy());

  dump << "\n Recoil: ";
  if (has_ammo) dump << ammo_recoil;
  dump << (recoil(false) >= 0 ? "+" : "" ) << recoil(false);
  if (has_ammo) dump << " = " << recoil();

  dump << "\n Reload time: " << int(gun->reload_time);
  if (has_flag(IF_RELOAD_ONE)) dump << " per round";

  if (burst_size() == 0) {
   if (gun->skill_used == sk_pistol && has_flag(IF_RELOAD_ONE)) dump << "\n Revolver.";
   else dump << "\n Semi-automatic.";
  } else dump << "\n Burst size: " << burst_size();
  if (contents.size() > 0) dump << "\n";
  for(const auto& it : contents) dump << "\n+" << it.tname();

 } else if (is_gunmod()) {

  const it_gunmod* const mod = dynamic_cast<const it_gunmod*>(type);
  if (mod->accuracy != 0) dump << " Accuracy: " << (mod->accuracy > 0 ? "+" : "") << int(mod->accuracy);
  if (mod->damage != 0) dump << "\n Damage: " << (mod->damage > 0 ? "+" : "") << int(mod->damage);
  if (mod->clip != 0) dump << "\n Clip: " << (mod->clip > 0 ? "+" : "") << int(mod->damage) << "%";
  if (mod->recoil != 0) dump << "\n Recoil: " << int(mod->recoil);
  if (mod->burst != 0) dump << "\n Burst: " << (mod->clip > 0 ? "+" : "") << int(mod->clip);
  if (mod->newtype != AT_NULL) dump << "\n " << ammo_name(mod->newtype);
  dump << "\n Used on: ";
  if (mod->used_on_pistol) dump << "Pistols.  ";
  if (mod->used_on_shotgun) dump << "Shotguns.  ";
  if (mod->used_on_smg) dump << "SMGs.  ";
  if (mod->used_on_rifle) dump << "Rifles.";

 } else if (is_armor()) {

  const it_armor* armor = dynamic_cast<const it_armor*>(type);
  dump << " Covers: ";
  if (armor->covers & mfb(bp_head)) dump << "The head. ";
  if (armor->covers & mfb(bp_eyes)) dump << "The eyes. ";
  if (armor->covers & mfb(bp_mouth)) dump << "The mouth. ";
  if (armor->covers & mfb(bp_torso)) dump << "The torso. ";
  if (armor->covers & mfb(bp_hands)) dump << "The hands. ";
  if (armor->covers & mfb(bp_legs)) dump << "The legs. ";
  if (armor->covers & mfb(bp_feet)) dump << "The feet. ";
  dump << "\n Encumberment: "			<< int(armor->encumber) <<
          "\n Bashing protection: "		<< int(armor->dmg_resist) <<
          "\n Cut protection: "			<< int(armor->cut_resist) <<
          "\n Environmental protection: "	<< int(armor->env_resist) <<
          "\n Warmth: "				<< int(armor->warmth) <<
          "\n Storage: "			<< int(armor->storage);

 } else if (is_book()) {

  const it_book* const book = dynamic_cast<const it_book*>(type);
  if (book->type == sk_null) dump << " Just for fun.\n";
  else {
   dump << " Can bring your " << skill_name(book->type) << " skill to " <<
           int(book->level) << std::endl;
   if (book->req == 0) dump << " It can be understood by beginners.\n";
   else
    dump << " Requires " << skill_name(book->type) << " level " <<
            int(book->req) << " to understand.\n";
  }
  dump << " Requires intelligence of " << int(book->intel) << std::endl;
  if (book->fun != 0)
   dump << " Reading this book affects your morale by " <<
           (book->fun > 0 ? "+" : "") << int(book->fun) << std::endl;
  dump << " This book takes " << int(book->time) << " minutes to read.";

 } else if (is_tool()) {

  const it_tool* const tool = dynamic_cast<const it_tool*>(type);
  dump << " Maximum " << tool->max_charges << " charges";
  if (tool->ammo == AT_NULL) dump << ".";
  else dump << " of " << ammo_name(tool->ammo) << ".";

 } else if (is_style()) {

  dump << "\n";
  for(const auto& m : dynamic_cast<const it_style*>(type)->moves) {
   dump << " " << default_technique_name(m.tech) << ". Requires Unarmed Skill of " << m.level << "\n";
  }

 } else if (type->techniques != 0) {

  dump << "\n";
  for (int i = 1; i < NUM_TECHNIQUES; i++) {
   if (type->techniques & mfb(i))
    dump << " " << default_technique_name( technique_id(i) ) << "; ";
  }

 }

 if (showtext) {
  dump << "\n\n" << type->description << "\n";
  if (contents.size() > 0) {
   if (is_gun()) {
    for (int i = 0; i < contents.size(); i++)
     dump << "\n " << contents[i].type->description;
   } else
    dump << "\n " << contents[0].type->description;
   dump << "\n";
  }
 }
 return dump.str();
}

nc_color item::color(player *u) const
{
 nc_color ret = c_ltgray;

 if (active) // Active items show up as yellow
  ret = c_yellow;
 else if (is_gun()) { // Guns are green if you are carrying ammo for them
  ammotype amtype = ammo_type();
  if (u->has_ammo(amtype).size() > 0)
   ret = c_green;
 } else if (is_ammo()) { // Likewise, ammo is green if you have guns that use it
  ammotype amtype = ammo_type();
  if (u->weapon.is_gun() && u->weapon.ammo_type() == amtype)
   ret = c_green;
  else {
   for (size_t i = 0; i < u->inv.size(); i++) {
    if (u->inv[i].is_gun() && u->inv[i].ammo_type() == amtype) {
     ret = c_green;
	 break;
    }
   }
  }
 } else if (is_book()) {
  const it_book* const tmp = dynamic_cast<const it_book*>(type);
  if (tmp->type !=sk_null && tmp->intel <= u->int_cur + u->sklevel[tmp->type] &&
      (tmp->intel == 0 || !u->has_trait(PF_ILLITERATE)) &&
      tmp->req <= u->sklevel[tmp->type] && tmp->level > u->sklevel[tmp->type])
   ret = c_ltblue;
 }
 return ret;
}

nc_color item::color_in_inventory(const player *u) const	// retain unused paramweter for later use
{
// Items in our inventory get colorized specially
 return active ? c_yellow : c_white;
}

std::string item::tname() const
{
 std::stringstream ret;

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
  if (corpse == NULL || corpse->id == mon_null) ret << "human blood";
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

 const it_comest* food = NULL;
 if (is_food()) food = dynamic_cast<const it_comest*>(type);
 else if (is_food_container()) food = dynamic_cast<const it_comest*>(contents[0].type);
 if (food != NULL && food->spoils != 0 && int(messages.turn) - bday > food->spoils * 600)
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
 if (type->id == itm_corpse) {
  int ret;
  switch (corpse->size) {
   case MS_TINY:   ret =    5;	break;
   case MS_SMALL:  ret =   60;	break;
   case MS_MEDIUM: ret =  520;	break;
   case MS_LARGE:  ret = 2000;	break;
   case MS_HUGE:   ret = 4000;	break;
  }
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
 if (type->id == itm_corpse) {
  switch (corpse->size) {
   case MS_TINY:   return   2;
   case MS_SMALL:  return  40;
   case MS_MEDIUM: return  75;
   case MS_LARGE:  return 160;
   case MS_HUGE:   return 600;
  }
 }
 int ret = type->volume;
 if (is_gun()) {
  for (const auto& it : contents) ret += it.volume();
 }
 return type->volume;
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

bool item::has_technique(technique_id tech, const player *p) const
{
 if (is_style()) {
  const it_style* const style = dynamic_cast<const it_style*>(type);
  for (const auto& m : style->moves) {
   if (m.tech == tech && (p == NULL || p->sklevel[sk_unarmed] >= m.level)) return true;
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

bool item::rotten(game *g)
{
 if (!is_food() || g == NULL) return false;
 const it_comest* const food = dynamic_cast<const it_comest*>(type);
 return (food->spoils != 0 && int(messages.turn) - bday > food->spoils * 600);
}

bool item::goes_bad() const
{
 if (!is_food()) return false;
 return 0 != dynamic_cast<const it_comest*>(type)->spoils;
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

 my_value += int(type->melee_dam * (1   + .3 * skills[sk_bashing] +
                                          .1 * skills[sk_melee]    ));
 //debugmsg("My value: (+bash) %d", my_value);

 my_value += int(type->melee_cut * (1   + .4 * skills[sk_cutting] +
                                          .1 * skills[sk_melee]    ));
 //debugmsg("My value: (+cut) %d", my_value);

 my_value += int(type->m_to_hit  * (1.2 + .3 * skills[sk_melee]));
 //debugmsg("My value: (+hit) %d", my_value);

 return my_value;
}

int item::melee_value(const int skills[num_skill_types]) const
{
 int my_value = 0;
 my_value += int(type->melee_dam * (1   + .3 * skills[sk_bashing] +
                                          .1 * skills[sk_melee]    ));

 my_value += int(type->melee_cut * (1   + .4 * skills[sk_cutting] +
                                          .1 * skills[sk_melee]    ));

 my_value += int(type->m_to_hit  * (1.2 + .3 * skills[sk_melee]));

 if (is_style())
  my_value += 15 * skills[sk_unarmed] + 8 * skills[sk_melee];

 return my_value;
}

style_move item::style_data(technique_id tech)
{
 style_move ret;

 if (!is_style()) return ret;

 const it_style* const style = dynamic_cast<const it_style*>(type);

 for(const auto& m : style->moves) {
  if (m.tech == tech) return m;
 }

 return ret;
}
 
bool item::is_two_handed(player *u)
{
 if (is_gun() && (dynamic_cast<const it_gun*>(type))->skill_used != sk_pistol) return true;
 return (weight() > u->str_cur * 4);
}

bool item::made_of(material mat) const
{
 if (type->id == itm_corpse) return (corpse->mat == mat);
 return (type->m1 == mat || type->m2 == mat);
}

bool item::conductive()
{
 if ((type->m1 == IRON || type->m1 == STEEL || type->m1 == SILVER ||
      type->m1 == MNULL) &&
     (type->m2 == IRON || type->m2 == STEEL || type->m2 == SILVER ||
      type->m2 == MNULL))
  return true;
 if (type->m1 == MNULL && type->m2 == MNULL)
  return true;
 return false;
}

bool item::destroyed_at_zero_charges()
{
 return (is_ammo() || is_food());
}

bool item::is_gun() const
{
 return type->is_gun();
}

bool item::is_gunmod() const
{
 return type->is_gunmod();
}

bool item::is_bionic() const
{
 return type->is_bionic();
}

bool item::is_ammo() const
{
 return type->is_ammo();
}

bool is_flammable(material m)
{
	return (m == COTTON || m == WOOL || m == PAPER || m == WOOD || m == MNULL);
}

bool item::is_food(player *u) const
{
 if (u == NULL) return is_food();
 if (type->is_food()) return true;
 if (u->has_bionic(bio_batteries) && is_ammo() && (dynamic_cast<const it_ammo*>(type))->type == AT_BATT) return true;
 if (u->has_bionic(bio_furnace) && is_flammable(type->m1) && is_flammable(type->m2) && type->id != itm_corpse) return true;
 return false;
}

bool item::is_food_container(player *u) const
{
 return (contents.size() >= 1 && contents[0].is_food(u));
}

bool item::is_food_container() const
{
 return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_drink()
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

bool item::is_bashing_weapon()
{
 return (type->melee_dam >= 8);
}

bool item::is_cutting_weapon()
{
 return (type->melee_cut >= 8 && !has_flag(IF_SPEAR));
}

bool item::is_armor() const
{
 return type->is_armor();
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

bool item::is_container() const
{
 return type->is_container();
}

bool item::is_tool() const
{
 return type->is_tool();
}

bool item::is_software() const
{
 return type->is_software();
}

bool item::is_macguffin() const
{
 return type->is_macguffin();
}

bool item::is_style() const
{
 return type->is_style();
}

bool item::is_other() const
{
 return (!is_gun() && !is_ammo() && !is_armor() && !is_food() &&
         !is_food_container() && !is_tool() && !is_gunmod() && !is_bionic() &&
         !is_book() && !is_weap());
}

bool item::is_artifact() const
{
 return type->is_artifact();
}

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

int item::clip_size() const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 if (gun->ammo != AT_40MM && charges > 0 && curammo->type == AT_40MM) return 1; // M203 mod in use
 int ret = gun->clip;
 for (const auto& it : contents) {
  if (it.is_gunmod()) {
   int bonus = (ret * (dynamic_cast<const it_gunmod*>(it.type))->clip) / 100;
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
  if (it.is_gunmod()) ret -= (dynamic_cast<const it_gunmod*>(it.type))->accuracy;
 }
 ret += damage * 2;
 return ret;
}

int item::gun_damage(bool with_ammo) const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->dmg_bonus;
 if (with_ammo && curammo != NULL) ret += curammo->damage;
 for(const auto& it : contents) {
  if (it.is_gunmod()) ret += (dynamic_cast<const it_gunmod*>(it.type))->damage;
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
  if (it.is_gunmod()) ret += (dynamic_cast<const it_gunmod*>(it.type))->loudness;
 }
 return ret;
}

int item::burst_size() const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->burst;
 for(const auto& it : contents) {
  if (it.is_gunmod()) ret += (dynamic_cast<const it_gunmod*>(it.type))->burst;
 }
 return (ret < 0) ? 0 : ret;
}

int item::recoil(bool with_ammo) const
{
 if (!is_gun()) return 0;
 const it_gun* const gun = dynamic_cast<const it_gun*>(type);
 int ret = gun->recoil;
 if (with_ammo && curammo != NULL) ret += curammo->recoil;
 for (const auto& it : contents) {
  if (it.is_gunmod()) ret += (dynamic_cast<const it_gunmod*>(it.type))->recoil;
 }
 return ret;
}

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
 

ammotype item::ammo_type() const
{
 if (is_gun()) {
  ammotype ret = dynamic_cast<const it_gun*>(type)->ammo;
  for (int i = 0; i < contents.size(); i++) {
   if (contents[i].is_gunmod()) {
    const it_gunmod* const mod = dynamic_cast<const it_gunmod*>(contents[i].type);
    if (mod->newtype != AT_NULL)
     ret = mod->newtype;
   }
  }
  return ret;
 } else if (is_tool()) return dynamic_cast<const it_tool*>(type)->ammo;
 else if (is_ammo()) return dynamic_cast<const it_ammo*>(type)->type;
 return AT_NULL;
}
 
int item::pick_reload_ammo(const player &u, bool interactive) const
{
 if (!type->is_gun() && !type->is_tool()) {
  debugmsg("RELOADING NON-GUN NON-TOOL");
  return -1;
 }

 std::vector<int> am;	// List of indicies of valid ammo

 if (type->is_gun()) {
  if (charges > 0) {
   const itype_id aid = itype_id(curammo->id);
   const auto& inv = u.inv;
   for (size_t i = 0; i < inv.size(); i++) {
    if (inv[i].type->id == aid) am.push_back(i);
   }
  } else {
   bool has_m203 = false;
   for (int i = 0; i < contents.size() && !has_m203; i++) {
     if (contents[i].type->id == itm_m203)
       has_m203 = true;
   }
   am = u.has_ammo(ammo_type());
   if (has_m203) {
	for (const auto grenade : u.has_ammo(AT_40MM)) am.push_back(grenade);
   }
  }
 } else {
  am = u.has_ammo(ammo_type());
 }

 int index = -1;

 if (am.size() > 1 && interactive) {// More than one option; list 'em and pick
  WINDOW* w_ammo = newwin(am.size() + 1, SCREEN_WIDTH, 0, 0);
  if (charges == 0) {
   char ch;
   clear();
   mvwprintw(w_ammo, 0, 0, "\
Choose ammo type:         Damage     Armor Pierce     Range     Accuracy");
   for (int i = 0; i < am.size(); i++) {
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
   while ((ch < 'a' || ch - 'a' > am.size() - 1) && ch != ' ' && ch != 27);
   werase(w_ammo);
   delwin(w_ammo);
   erase();
   index = (ch == ' ' || ch == 27) ? -1 : am[ch - 'a'];
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
 } else if (am.size() == 1 || !interactive)
  index = am[0];
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
 } else if (is_tool()) {
  single_load = false;
  max_load = dynamic_cast<const it_tool*>(type)->max_charges;
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

bool item::burn(int amount)
{
 burnt += amount;
 return (burnt >= volume() * 3);
}

std::string default_technique_name(technique_id tech)
{
 switch (tech) {
  case TEC_SWEEP: return "Sweep attack";
  case TEC_PRECISE: return "Precision attack";
  case TEC_BRUTAL: return "Knock-back attack";
  case TEC_GRAB: return "Grab";
  case TEC_WIDE: return "Hit all adjacent monsters";
  case TEC_RAPID: return "Rapid attack";
  case TEC_FEINT: return "Feint";
  case TEC_THROW: return "Throw";
  case TEC_BLOCK: return "Block";
  case TEC_BLOCK_LEGS: return "Leg block";
  case TEC_WBLOCK_1: return "Weak block";
  case TEC_WBLOCK_2: return "Parry";
  case TEC_WBLOCK_3: return "Shield";
  case TEC_COUNTER: return "Counter-attack";
  case TEC_BREAK: return "Grab break";
  case TEC_DISARM: return "Disarm";
  case TEC_DEF_THROW: return "Defensive throw";
  case TEC_DEF_DISARM: return "Defense disarm";
  default: return "A BUG!";
 }
 return "A BUG!";
}
