#include "crafting.h"
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "skill.h"
#include "inventory.h"
#include "rng.h"
#include "recent_msg.h"

static const int CRAFTING_WIN_HEIGHT = VIEW - TABBED_HEADER_HEIGHT;

inventory crafting_inventory(const player& u)  // 2020-05-28 NPC-valid
{
    inventory crafting_inv(u.GPSpos, PICKUP_RANGE);
    crafting_inv += u.inv;
    crafting_inv += u.weapon;
    if (u.has_bionic(bio_tools)) {
        item tools(item::types[itm_toolset], messages.turn);
        tools.charges = u.power_level;
        crafting_inv += tools;
    }
    return crafting_inv;
}

void game::craft()
{
 if (u.morale_level() < MIN_MORALE_CRAFT) {	// See morale.h
  messages.add("Your morale is too low to craft...");
  return;
 }
 static constexpr const char* const labels[] = {"WEAPONS", "FOOD", "ELECTRONICS", "ARMOR", "MISC", nullptr};
 WINDOW *w_head = newwin(TABBED_HEADER_HEIGHT, SCREEN_WIDTH, 0, 0);
 WINDOW *w_data = newwin(CRAFTING_WIN_HEIGHT, SCREEN_WIDTH, TABBED_HEADER_HEIGHT, 0);
 craft_cat tab = CC_WEAPON;
 std::vector<const recipe*> current;
 std::vector<bool> available;
 item tmp;
 int line = 0, xpos, ypos;
 bool redraw = true;
 bool done = false;
 int ch;

 inventory crafting_inv(crafting_inventory(u));

 do {
  if (redraw) { // When we switch tabs, redraw the header
   redraw = false;
   line = 0;
   draw_tabs(w_head, tab - 1, labels); // \todo C:Whales colors were c_ltgray, h_ltgray rather than c_white, h_white
// Set current to all recipes in the current tab; available are possible to make
   pick_recipes(current, available, tab);
  }

// Clear the screen of recipe data, and draw it anew
  werase(w_data);
  mvwaddstrz(w_data, 20, 0, c_white, "Press ? to describe object.  Press <ENTER> to attempt to craft object.");
  wrefresh(w_data);
  for (int i = 0; i < current.size() && i < CRAFTING_WIN_HEIGHT; i++) {
   if (i == line)
    mvwaddstrz(w_data, i, 0, (available[i] ? h_white : h_dkgray),
		item::types[current[i]->result]->name.c_str());
   else
    mvwaddstrz(w_data, i, 0, (available[i] ? c_white : c_dkgray),
		item::types[current[i]->result]->name.c_str());
  }
  if (current.size() > 0) {
   nc_color col = (available[line] ? c_white : c_dkgray);
   mvwprintz(w_data, 0, VBAR_X, col, "Primary skill: %s",
             (current[line]->sk_primary == sk_null ? "N/A" :
              skill_name(current[line]->sk_primary)));
   mvwprintz(w_data, 1, VBAR_X, col, "Secondary skill: %s",
             (current[line]->sk_secondary == sk_null ? "N/A" :
              skill_name(current[line]->sk_secondary)));
   mvwprintz(w_data, 2, VBAR_X, col, "Difficulty: %d", current[line]->difficulty);
   if (current[line]->sk_primary == sk_null)
    mvwaddstrz(w_data, 3, VBAR_X, col, "Your skill level: N/A");
   else
    mvwprintz(w_data, 3, VBAR_X, col, "Your skill level: %d", u.sklevel[current[line]->sk_primary]);
   mvwprintz(w_data, 4, VBAR_X, col, "Time to complete: %s", current[line]->time_desc().c_str());
   mvwaddstrz(w_data, 5, VBAR_X, col, "Tools required:");
   if (current[line]->tools[0].empty()) {
    mvwputch(w_data, 6, VBAR_X, col, '>');
    mvwaddstrz(w_data, 6, VBAR_X + 2, c_green, "NONE");
    ypos = 6;
   } else {
    ypos = 5;
// Loop to print the required tools
    for (decltype(auto) min_term : current[line]->tools) {
        ypos++;
        xpos = VBAR_X + 2;
        mvwputch(w_data, ypos, VBAR_X, col, '>');

        int j = -1;
        for (decltype(auto) tool : min_term) {
            ++j;
            if (0 < j) {
                if (xpos >= SCREEN_WIDTH - 3) {
                    xpos = VBAR_X + 2;
                    ypos++;
                }
                mvwaddstrz(w_data, ypos, xpos, c_white, "OR ");
                xpos += 3;
            }

            const itype_id type = tool.type;
            const int charges = tool.count;
            nc_color toolcol = c_red;

            if (charges <= 0 ? crafting_inv.has_amount(type, 1) : crafting_inv.has_charges(type, charges))
                toolcol = c_green;

            std::string toolname(item::types[type]->name);
            if (0 < charges) toolname += " (" + std::to_string(charges) + " charges)";
            if (xpos + toolname.length() >= SCREEN_WIDTH) {
                xpos = VBAR_X + 2;
                ypos++;
            }
            mvwaddstrz(w_data, ypos, xpos, toolcol, toolname.c_str());
            xpos += toolname.length();
        }
    }
   }
 // Loop to print the required components
   ypos++;
   mvwaddstrz(w_data, ypos, VBAR_X, col, "Components required:");
   for (decltype(auto) min_term : current[line]->components) {
       mvwputch(w_data, ++ypos, VBAR_X, col, '>');
       xpos = VBAR_X + 2;

       int j = -1;
       for (decltype(auto) comp : min_term) {
           ++j;
           if (0 < j) {
               if (xpos >= SCREEN_WIDTH - 3) {
                   ypos++;
                   xpos = VBAR_X + 2;
               }
               mvwaddstrz(w_data, ypos, xpos, c_white, " OR ");
               xpos += 4;
           }

           const int count = comp.count;
           const itype_id type = comp.type;
           const itype* const i_type = item::types[type];
           nc_color compcol = c_red;
           if (i_type->count_by_charges() && count > 0) {
               if (crafting_inv.has_charges(type, count))
                   compcol = c_green;
           }
           else if (crafting_inv.has_amount(type, abs(count)))
               compcol = c_green;

           std::string compname(std::to_string(abs(count)) + "x " + i_type->name);
           if (xpos + compname.length() >= SCREEN_WIDTH) {
               ypos++;
               xpos = VBAR_X + 2;
           }
           mvwaddstrz(w_data, ypos, xpos, compcol, compname.c_str());
           xpos += compname.length();
       }
   }
  }

  wrefresh(w_data);
  ch = input();
  switch (ch) {
  case '<':
   if (tab == CC_WEAPON)
    tab = CC_MISC;
   else
    tab = craft_cat(int(tab) - 1);
   redraw = true;
   break;
  case '>':
   if (tab == CC_MISC)
    tab = CC_WEAPON;
   else
    tab = craft_cat(int(tab) + 1);
   redraw = true;
   break;
  case 'j':
   line++;
   break;
  case 'k':
   line--;
   break;
  case '\n':
   if (!available[line]) popup("You can't do that!");
   else if (item::types[current[line]->result]->m1 == LIQUID &&
            !u.has_watertight_container())
    popup("You don't have anything to store that liquid in!");
   else {
    make_craft(current[line]);
    done = true;
   }
   break;
  case '?':
   tmp = item(item::types[current[line]->result], 0);
   full_screen_popup(tmp.info(true).c_str());
   redraw = true;
   break;
  }
  if (line < 0) line = current.size() - 1;
  else if (line >= current.size()) line = 0;
 } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && !done);

 werase(w_head);
 werase(w_data);
 delwin(w_head);
 delwin(w_data);
 refresh_all();
}

void game::pick_recipes(std::vector<const recipe*> &current,
                        std::vector<bool> &available, craft_cat tab)
{
 const inventory crafting_inv(crafting_inventory(u));

 current.clear();
 available.clear();
 for (const recipe* const tmp : recipe::recipes) {
// Check if the category matches the tab, and we have the requisite skills
  if (    tmp->category == tab
      && (sk_null == tmp->sk_primary   || u.sklevel[tmp->sk_primary] >= tmp->difficulty)
      && (sk_null == tmp->sk_secondary || 0 < u.sklevel[tmp->sk_secondary]))
  current.push_back(tmp);
  available.push_back(false);
 }
 for (int i = 0; i < current.size() && i < CRAFTING_WIN_HEIGHT; i++) {
//Check if we have the requisite tools and components
  bool have_all = true;

  for(decltype(auto) min_term : current[i]->tools) {
      bool have = false;
      for (decltype(auto) tool : min_term) {
          const itype_id type = tool.type;
          const int req = tool.count;	// -1 => 1
          if (req <= 0 ? crafting_inv.has_amount(type, 1) : crafting_inv.has_charges(type, req)) {
              have = true;
              break;
          }
      }
      if (!have) {
          have_all = false;
          break;
      }
  }
  if (!have_all) continue;

  for (decltype(auto) min_term : current[i]->components) {
      bool have = false;
      for (decltype(auto) comp : min_term) {
          const itype_id type = comp.type;
          const int count = comp.count;
          if (item::types[type]->count_by_charges() && count > 0) {
              if (crafting_inv.has_charges(type, count)) {
                  have = true;
                  break;
              }
          } else if (crafting_inv.has_amount(type, abs(count))) {
              have = true;
              break;
          }
      }
      if (!have) {
          have_all = false;
          break;
      }
  }
  if (!have_all) continue;

  available[i] = true;
 }
}

void game::make_craft(const recipe* const making)
{
 u.assign_activity(ACT_CRAFT, making->time, making->id);
 u.moves = 0;
}

void game::complete_craft()
{
 const recipe* const making = recipe::recipes[u.activity.index]; // Which recipe is it?

// # of dice is 75% primary skill, 25% secondary (unless secondary is null)
 int skill_dice = u.sklevel[making->sk_primary] * 3;
 skill_dice += u.sklevel[sk_null == making->sk_secondary ? making->sk_primary : making->sk_secondary];
// Sides on dice is 16 plus your current intelligence
 int skill_sides = 16 + u.int_cur;

 int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
 int diff_sides = 24;	// 16 + 8 (default intelligence)

 int skill_roll = dice(skill_dice, skill_sides);
 int diff_roll  = dice(diff_dice,  diff_sides);

 if (making->sk_primary != sk_null) u.practice(making->sk_primary, making->difficulty * 5 + 20);
 if (making->sk_secondary != sk_null) u.practice(making->sk_secondary, 5);

// Messed up badly; waste some components.
 if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
  messages.add("You fail to make the %s, and waste some materials.", item::types[making->result]->name.c_str());
  for (const auto& comp : making->components) if (!comp.empty()) {
    std::vector<component> wasted(comp);
	for (auto& comp : wasted) comp.count = rng(0, comp.count);
    consume_items(u, wasted);
  }
  for(const auto& tools : making->tools) if (!tools.empty()) consume_tools(u, tools);
  u.activity.type = ACT_NULL;
  return;
  // Messed up slightly; no components wasted.
 } else if (diff_roll > skill_roll) {
  messages.add("You fail to make the %s, but don't waste any materials.", item::types[making->result]->name.c_str());
  u.activity.type = ACT_NULL;
  return;
 }
// If we're here, the craft was a success!
// Use up the components and tools
 for (const auto& comp : making->components) if (!comp.empty()) consume_items(u, comp);
 for (const auto& tools : making->tools) if (!tools.empty()) consume_tools(u, tools);

  // Set up the new item, and pick an inventory letter
 item newit(item::types[making->result], messages.turn);
 if (!newit.craft_has_charges()) newit.charges = 0;
 const bool inv_ok = u.assign_invlet(newit);
 //newit = newit.in_its_container();
 if (newit.made_of(LIQUID))
  handle_liquid(newit, false, false);
 else {
// We might not have space for the item
  if (!inv_ok || u.volume_carried()+newit.volume() > u.volume_capacity()) {
   messages.add("There's no room in your inventory for the %s, so you drop it.", newit.tname().c_str());
   u.GPSpos.add(std::move(newit));
  } else if (u.weight_carried() + newit.volume() > u.weight_capacity()) {
   messages.add("The %s is too heavy to carry, so you drop it.", newit.tname().c_str());
   u.GPSpos.add(std::move(newit));
  } else {
   messages.add("%c - %s", newit.invlet, newit.tname().c_str());
   u.i_add(std::move(newit));
  }
 }
}

void consume_items(player& u, const std::vector<component>& components)
{
// For each set of components in the recipe, fill you_have with the list of all
// matching ingredients the player has.
 std::vector<component> player_has;
 std::vector<component> map_has;
 std::vector<component> mixed;
 std::vector<component> player_use;
 std::vector<component> map_use;
 std::vector<component> mixed_use;
 inventory map_inv(u.GPSpos, PICKUP_RANGE);

 for(const component& comp : components) {
  const itype_id type = comp.type;
  const int count = abs(comp.count);
  bool pl = false, mp = false;

  if (item::types[type]->count_by_charges() && count > 0) {
   if (u.has_charges(type, count)) {
    player_has.push_back(comp);
    pl = true;
   }
   if (map_inv.has_charges(type, count)) {
    map_has.push_back(comp);
    mp = true;
   }
   if (!pl && !mp && u.charges_of(type) + map_inv.charges_of(type) >= count)
    mixed.push_back(comp);
  } else { // Counting by units, not charges
   if (u.has_amount(type, count)) {
    player_has.push_back(comp);
    pl = true;
   }
   if (map_inv.has_amount(type, count)) {
    map_has.push_back(comp);
    mp = true;
   }
   if (!pl && !mp && u.amount_of(type) + map_inv.amount_of(type) >= count)
    mixed.push_back(comp);
  }
 }

 if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
  if (player_has.size() == 1)
   player_use.push_back(player_has[0]);
  else if (map_has.size() == 1)
   map_use.push_back(map_has[0]);
  else
   mixed_use.push_back(mixed[0]);

 } else { // Let the player pick which component they want to use
  std::vector<std::string> options; // List for the menu_vec below
// Populate options with the names of the items
  for(const component& comp : map_has)
   options.push_back(item::types[comp.type]->name + " (nearby)");
  for(const component& comp : player_has)
   options.push_back(item::types[comp.type]->name);
  for(const component& comp : mixed)
   options.push_back(item::types[comp.type]->name + " (on person & nearby)");

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get the selection via a menu popup
  int selection = menu_vec("Use which component?", options) - 1;
  if (selection < map_has.size())
   map_use.push_back(map_has[selection]);
  else if (selection < map_has.size() + player_has.size()) {
   selection -= map_has.size();
   player_use.push_back(player_has[selection]);
  } else {
   selection -= map_has.size() + player_has.size();
   mixed_use.push_back(mixed[selection]);
  }
 }

 for(const component& comp : player_use) {
  if (item::types[comp.type]->count_by_charges() && 0 < comp.count)
   u.use_charges(comp.type, comp.count);
  else
   u.use_amount(comp.type, abs(comp.count), (comp.count < 0));
 }
 for(const component& comp : map_use) {
  if (item::types[comp.type]->count_by_charges() && 0 < comp.count)
   u.GPSpos.use_charges(PICKUP_RANGE, comp.type, comp.count);
  else
   u.GPSpos.use_amount(PICKUP_RANGE, comp.type, abs(comp.count), (comp.count < 0));
 }
 for(const component& comp : mixed_use) {
  if (item::types[comp.type]->count_by_charges() && 0 < comp.count) {
      if (const unsigned int from_map = comp.count - u.use_charges(comp.type, comp.count)) {
          u.GPSpos.use_charges(PICKUP_RANGE, comp.type, from_map);
      }
  } else {
   const bool in_container = (comp.count < 0);
   const int from_map = abs(comp.count) - u.amount_of(comp.type);
   u.use_amount(comp.type, u.amount_of(comp.type), in_container);
   u.GPSpos.use_amount(PICKUP_RANGE, comp.type, from_map, in_container);
  }
 }
}

void consume_tools(player& u, const std::vector<component>& tools)
{
 bool found_nocharge = false;
 inventory map_inv(u.GPSpos, PICKUP_RANGE);
 std::vector<component> player_has;
 std::vector<component> map_has;
// Use charges of any tools that require charges used
 for (int i = 0; i < tools.size() && !found_nocharge; i++) {
  const itype_id type = tools[i].type;
  const int count = tools[i].count;
  if (count > 0) {
   if (u.has_charges(type, count))
    player_has.push_back(tools[i]);
   if (map_inv.has_charges(type, count))
    map_has.push_back(tools[i]);
  } else if (u.has_amount(type, 1) || map_inv.has_amount(type, 1))
   found_nocharge = true;
 }
 if (found_nocharge) return; // Default to using a tool that doesn't require charges

 if (player_has.size() == 1 && map_has.size() == 0) {
  const component& use = player_has[0];
  u.use_charges(use.type, use.count);
 } else if (map_has.size() == 1 && player_has.size() == 0) {
  const component& use = map_has[0];
  u.GPSpos.use_charges(PICKUP_RANGE, use.type, use.count);
 } else { // Variety of options, list them and pick one
// Populate the list
  std::vector<std::string> options;
  for(const component& tool : map_has)
   options.push_back(item::types[tool.type]->name + " (nearby)");

  for(const component& tool : player_has)
   options.push_back(item::types[tool.type]->name);

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get selection via a popup menu
  int selection = menu_vec("Use which tool?", options) - 1;
  if (selection < map_has.size()) {
   const component& use = map_has[selection];
   u.GPSpos.use_charges(PICKUP_RANGE, use.type, use.count);
  } else {
   const component& use = player_has[selection - map_has.size()];
   u.use_charges(use.type, use.count);
  }
 }
}
