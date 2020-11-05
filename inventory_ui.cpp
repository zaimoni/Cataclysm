#include "game.h"
#include "output.h"
#include "keypress.h"
#include "recent_msg.h"
#include "stl_typetraits.h"
#include <array>

static const std::string CATEGORIES[8] =
 {"FIREARMS:", "AMMUNITION:", "CLOTHING:", "COMESTIBLES:",
  "TOOLS:", "BOOKS:", "WEAPONS:", "OTHER:"};

static void print_inv_statics(game *g, WINDOW* w_inv, std::string title, const std::vector<char>& dropped_items)
{
// Print our header
 mvwprintw(w_inv, 0, 0, title.c_str());

 const int mid_pt = SCREEN_WIDTH / 2;
 const int three_quarters = 3 * SCREEN_WIDTH / 4;

// Print weight
 mvwprintw(w_inv, 0, mid_pt, "Weight: ");
 const int my_w_capacity = g->u.weight_capacity();
 const int my_w_carried = g->u.weight_carried();
 wprintz(w_inv, (my_w_carried >= my_w_capacity / 4) ? c_red : c_ltgray, "%d", my_w_carried);
 wprintz(w_inv, c_ltgray, "/%d/%d", my_w_capacity/4, my_w_capacity);

// Print volume
 mvwprintw(w_inv, 0, three_quarters, "Volume: ");
 const int my_v_capacity = g->u.volume_capacity() - 2;
 const int my_v_carried = g->u.volume_carried();
 wprintz(w_inv, (my_v_carried > my_v_capacity) ? c_red : c_ltgray, "%d", my_v_carried);
 wprintw(w_inv, "/%d", my_v_capacity);

// Print our weapon
 mvwprintz(w_inv, 2, mid_pt, c_magenta, "WEAPON:");
 if (g->u.is_armed()) {
  const bool dropping_weapon = cataclysm::any(dropped_items, g->u.weapon.invlet);
  if (dropping_weapon)
   mvwprintz(w_inv, 3, mid_pt, c_white, "%c + %s", g->u.weapon.invlet, g->u.weapname().c_str());
  else
   mvwprintz(w_inv, 3, mid_pt, g->u.weapon.color_in_inventory(g->u), "%c - %s", g->u.weapon.invlet, g->u.weapname().c_str());
 } else if (g->u.weapon.is_style())
  mvwprintz(w_inv, 3, mid_pt, c_ltgray, "%c - %s", g->u.weapon.invlet, g->u.weapname().c_str());
 else
  mvwprintz(w_inv, 3, mid_pt + 2, c_ltgray, g->u.weapname().c_str());
// Print worn items
 if (!g->u.worn.empty()) {
     mvwprintz(w_inv, 5, mid_pt, c_magenta, "ITEMS WORN:");
     int row = 6;
     for (const item& armor : g->u.worn) {
         const bool dropping_armor = cataclysm::any(dropped_items, armor.invlet);
         mvwprintz(w_inv, row++, mid_pt, (dropping_armor ? c_white : c_ltgray), "%c + %s", armor.invlet, armor.tname().c_str());
     }
 }
}
 
static auto find_firsts(const inventory &inv)
{
 std::array<int, 8> firsts;
 firsts.fill(-1);

 for (size_t i = 0; i < inv.size(); i++) {
  if (firsts[0] == -1 && inv[i].is_gun())
   firsts[0] = i;
  else if (firsts[1] == -1 && inv[i].is_ammo())
   firsts[1] = i;
  else if (firsts[2] == -1 && inv[i].is_armor())
   firsts[2] = i;
  else if (firsts[3] == -1 &&
           (inv[i].is_food() || inv[i].is_food_container()))
   firsts[3] = i;
  else if (firsts[4] == -1 && (inv[i].is_tool() || inv[i].is_gunmod() ||
                               inv[i].is_bionic()))
   firsts[4] = i;
  else if (firsts[5] == -1 && inv[i].is_book())
   firsts[5] = i;
  else if (firsts[6] == -1 && inv[i].is_weap())
   firsts[6] = i;
  else if (firsts[7] == -1 && inv[i].is_other())
   firsts[7] = i;
 }

 return firsts;
}
 
// Display current inventory.
char game::inv(std::string title)
{
 static const std::vector<char> null_vector;
 const int maxitems = VIEW - 5;	// Number of items to show at one time.
 int ch = '.';
 int start = 0, cur_it;
 u.sort_inv();
 u.inv.restack(&u);
 std::unique_ptr<WINDOW, curses_full_delete> w_inv(newwin(VIEW, SCREEN_WIDTH, 0, 0));
 print_inv_statics(this, w_inv.get(), title, null_vector);
// Gun, ammo, weapon, armor, food, tool, book, other
 const auto firsts = find_firsts(u.inv);

 do {
  if (ch == '<' && start > 0) { // Clear lines and shift
   for (int i = 1; i < VIEW; i++) draw_hline(w_inv.get(), i, c_black, ' ', 0, SCREEN_WIDTH / 2);
   start -= maxitems;
   if (start < 0) start = 0;
   mvwprintw(w_inv.get(), maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size()) { // Clear lines and shift
   start = cur_it;
   mvwprintw(w_inv.get(), maxitems + 2, 12, "            ");
   for (int i = 1; i < VIEW; i++) draw_hline(w_inv.get(), i, c_black, ' ', 0, SCREEN_WIDTH / 2);
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv.get(), cur_line, 0, "                                    ");
// Print category header
   for (int i = 0; i < 8; i++) {
    if (cur_it == firsts[i]) {
     mvwprintz(w_inv.get(), cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < u.inv.size()) {
    const item& it = u.inv[cur_it];
    mvwputch (w_inv.get(), cur_line, 0, c_white, it.invlet);
    mvwprintz(w_inv.get(), cur_line, 1, it.color_in_inventory(u), " %s", it.tname().c_str());
    if (u.inv.stack_at(cur_it).size() > 1)
     wprintw(w_inv.get(), " [%d]", u.inv.stack_at(cur_it).size());
    if (it.charges > 0)
     wprintw(w_inv.get(), " (%d)", it.charges);
    else if (it.contents.size() == 1 && it.contents[0].charges > 0)
     wprintw(w_inv.get(), " (%d)", it.contents[0].charges);
   }
   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv.get(), maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size())
   mvwprintw(w_inv.get(), maxitems + 4, 12, "> More items");
  wrefresh(w_inv.get());
  ch = getch();
 } while (ch == '<' || ch == '>');
 erase();
 refresh_all();
 return ch;
}

std::vector<item> game::multidrop()
{
 u.sort_inv();
 u.inv.restack(&u);
 WINDOW* w_inv = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 const int maxitems = VIEW - 5; // Number of items to show at one time.
 std::vector<int> dropping(u.inv.size(), 0);
 int count = 0; // The current count
 std::vector<char> weapon_and_armor; // Always single, not counted
 bool warned_about_bionic = false; // Printed add_msg re: dropping bionics
 print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
// Gun, ammo, weapon, armor, food, tool, book, other
 const auto firsts = find_firsts(u.inv);

 int ch = '.';
 int start = 0;
 size_t cur_it = 0;
 do {
  if (ch == '<' && start > 0) {
   for (int i = 1; i < VIEW; i++) draw_hline(w_inv, i, c_black, ' ', 0, SCREEN_WIDTH / 2);
   start -= maxitems;
   if (start < 0) start = 0;
   mvwprintw(w_inv, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size()) {
   start = cur_it;
   mvwprintw(w_inv, maxitems + 2, 12, "            ");
   for (int i = 1; i < VIEW; i++) draw_hline(w_inv, i, c_black, ' ', 0, SCREEN_WIDTH / 2);
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < VIEW - 2; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                    ");
// Print category header
   for (int i = 0; i < 8; i++) {
    if (cur_it == firsts[i]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < u.inv.size()) {
    const item& it = u.inv[cur_it];
    mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
    char icon = '-';
    if (dropping[cur_it] >= u.inv.stack_at(cur_it).size())
     icon = '+';
    else if (dropping[cur_it] > 0)
     icon = '#';
    nc_color col = (dropping[cur_it] == 0 ? c_ltgray : c_white);
    mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon, it.tname().c_str());
    if (u.inv.stack_at(cur_it).size() > 1)
     wprintz(w_inv, col, " [%d]", u.inv.stack_at(cur_it).size());
    if (it.charges > 0) wprintz(w_inv, col, " (%d)", it.charges);
    else if (it.contents.size() == 1 && it.contents[0].charges > 0)
     wprintw(w_inv, " (%d)", it.contents[0].charges);
   }
   cur_line++;
  }
  if (start > 0) mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size()) mvwprintw(w_inv, maxitems + 4, 12, "> More items");
  wrefresh(w_inv);
  ch = getch();
  if (ch >= '0' && ch <= '9') {
   ch -= '0';
   count *= 10;
   count += ch;
  } else if (u.has_item(ch)) {
   int index = u.inv.index_by_letter(ch);
   if (index == -1) { // Not from inventory
    int found = false;
    for (int i = 0; i < weapon_and_armor.size() && !found; i++) {
     if (weapon_and_armor[i] == ch) {
      EraseAt(weapon_and_armor, i);
      found = true;
      print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
     }
    }
    if (!found) {
     if (ch == u.weapon.invlet && is_between(num_items + 1, u.weapon.type->id, num_all_items-1)) {
      if (!warned_about_bionic)
       messages.add("You cannot drop your %s.", u.weapon.tname().c_str());
      warned_about_bionic = true;
     } else {
      weapon_and_armor.push_back(ch);
      print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
     }
    }
   } else {
    const size_t ub = u.inv.stack_at(index).size();
    if (count == 0) {
     dropping[index] = (0 == dropping[index]) ? ub : 0;
    } else {
     dropping[index] = (count >= ub) ? ub : count;
    }
   }
   count = 0;
  }
 } while (ch != '\n' && ch != KEY_ESCAPE && ch != ' ');
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();

 std::vector<item> ret;

 if (ch != '\n') return ret; // Canceled!

 int current_stack = 0;
 size_t max_size = u.inv.size();
 for (size_t i = 0; i < max_size; i++) {
  for (int j = 0; j < dropping[i]; j++) {
   if (current_stack >= 0) {
    if (u.inv.stack_at(current_stack).size() == 1) {
     ret.push_back(u.inv.remove_item(current_stack));
     current_stack--;
    } else
     ret.push_back(u.inv.remove_item(current_stack));
   }
  }
  current_stack++;
 }

 for (int i = 0; i < weapon_and_armor.size(); i++)
  ret.push_back(u.i_rem(weapon_and_armor[i]));

 return ret;
}

