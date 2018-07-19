#include "game.h"
#include "output.h"
#include "keypress.h"
#include <sstream>

#define LESS(a, b) ((a)<(b)?(a):(b))

void game::wish()
{
 const auto t_size = itype::types.size();
 WINDOW* w_list = newwin(25, 30, 0,  0);
 WINDOW* w_info = newwin(25, 50, 0, 30);
 int a = 0, shift = 0, result_selected = 0;
 char ch = '.';
 bool search = false, found = false;
 std::string pattern;
 std::string info;
 std::vector<int> search_results;
 item tmp;
 tmp.corpse = mtype::types[0];
 do {
  werase(w_info);
  werase(w_list);
  mvwprintw(w_list, 0, 0, "Wish for a: ");
  if (search) {
   found = false;
   if (ch == '\n') {
    search = false;
    found = true;
    ch = '.';
   } else if (ch == KEY_BACKSPACE || ch == 127) {
    if (pattern.length() > 0)
     pattern.erase(pattern.end() - 1);
   } else if (ch == '>') {
    search = false;
    if (!search_results.empty()) {
     result_selected++;
     if (result_selected > search_results.size())
      result_selected = 0;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > t_size) {
      a = shift + 23 - t_size;
      shift = t_size - 23;
     }
    }
   } else if (ch == '<') {
    search = false;
    if (!search_results.empty()) {
     result_selected--;
     if (result_selected < 0)
      result_selected = search_results.size() - 1;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > t_size) {
      a = shift + 23 - t_size;
      shift = t_size - 23;
     }
    }
   } else {
    pattern += ch;
    search_results.clear();
   }

   if (search) {
    for (int i = 0; i < t_size; i++) {
     if (itype::types[i]->name.find(pattern) != std::string::npos) {
      shift = i;
      a = 0;
      result_selected = 0;
      if (shift + 23 > t_size) {
       a = shift + 23 - t_size;
       shift = t_size - 23;
      }
      found = true;
      search_results.push_back(i);
     }
    }
    if (search_results.size() > 0) {
     shift = search_results[0];
     a = 0;
    }
   }

  } else {	// Not searching; scroll by keys
   if (ch == 'j') a++;
   if (ch == 'k') a--;
   if (ch == '/') { 
    search = true;
    pattern =  "";
    found = false;
    search_results.clear();
   }
   if (ch == '>' && !search_results.empty()) {
    result_selected++;
    if (result_selected > search_results.size())
     result_selected = 0;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > t_size) {
     a = shift + 23 - t_size;
     shift = t_size - 23;
    }
   } else if (ch == '<' && !search_results.empty()) {
    result_selected--;
    if (result_selected < 0)
     result_selected = search_results.size() - 1;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > t_size) {
     a = shift + 23 - t_size;
     shift = t_size - 23;
    }
   }
  }
  if (!search_results.empty())
   mvwprintz(w_list, 0, 11, c_green, "%s               ", pattern.c_str());
  else if (pattern.length() > 0)
   mvwprintz(w_list, 0, 11, c_red, "%s not found!            ",pattern.c_str());
  if (a < 0) {
   a = 0;
   shift--;
   if (shift < 0) shift = 0;
  }
  if (a > 22) {
   a = 22;
   shift++;
   if (shift + 23 > t_size) shift = t_size - 23;
  }
  for (int i = 1; i < 24 && i-1+shift < t_size; i++) {
   const nc_color col = (a + 1 == i) ? h_white : c_white;
   const itype* const it = itype::types[i - 1 + shift];
   mvwprintz(w_list, i, 0, col, it->name.c_str());
   wprintz(w_list, it->color, "%c%", it->sym);
  }
  tmp.make(itype::types[a + shift]);
  tmp.bday = turn;
  if (tmp.is_tool()) tmp.charges = dynamic_cast<const it_tool*>(tmp.type)->max_charges;
  else if (tmp.is_ammo()) tmp.charges = 100;
  else tmp.charges = -1;
  info = tmp.info(true);
  mvwprintw(w_info, 1, 0, info.c_str());
  wrefresh(w_info);
  wrefresh(w_list);
  ch = search ? getch() : input();
 } while (ch != '\n');
 clear();
 mvprintw(0, 0, "\nWish granted - %d (%d).", tmp.type->id, itm_antibiotics);
 tmp.invlet = nextinv;
 u.i_add(tmp);
 advance_nextinv();
 getch();
 delwin(w_info);
 delwin(w_list);
}

void game::monster_wish()
{
 WINDOW* w_list = newwin(25, 30, 0,  0);
 WINDOW* w_info = newwin(25, 50, 0, 30);
 int a = 0, shift = 1, result_selected = 0;
 char ch = '.';
 bool search = false, found = false, friendly = false;
 std::string pattern;
 std::string info;
 std::vector<int> search_results;
 monster tmp;
 do {
  werase(w_info);
  werase(w_list);
  mvwprintw(w_list, 0, 0, "Spawn a: ");
  if (search) {
   found = false;
   if (ch == '\n') {
    search = false;
    found = true;
    ch = '.';
   } else if (ch == KEY_BACKSPACE || ch == 127) {
    if (pattern.length() > 0)
     pattern.erase(pattern.end() - 1);
   } else if (ch == '>') {
    search = false;
    if (!search_results.empty()) {
     result_selected++;
     if (result_selected > search_results.size())
      result_selected = 0;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > mtype::types.size()) {
      a = shift + 23 - mtype::types.size();
      shift = mtype::types.size() - 23;
     }
    }
   } else if (ch == '<') {
    search = false;
    if (!search_results.empty()) {
     result_selected--;
     if (result_selected < 0)
      result_selected = search_results.size() - 1;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > mtype::types.size()) {
      a = shift + 23 - mtype::types.size();
      shift = mtype::types.size() - 23;
     }
    }
   } else {
    pattern += ch;
    search_results.clear();
   }

   if (search) {
    for (int i = 1; i < mtype::types.size(); i++) {
     if (mtype::types[i]->name.find(pattern) != std::string::npos) {
      shift = i;
      a = 0;
      result_selected = 0;
      if (shift + 23 > mtype::types.size()) {
       a = shift + 23 - mtype::types.size();
       shift = mtype::types.size() - 23;
      }
      found = true;
      search_results.push_back(i);
     }
    }
   }

  } else {	// Not searching; scroll by keys
   if (ch == 'j') a++;
   if (ch == 'k') a--;
   if (ch == 'f') friendly = !friendly;
   if (ch == '/') { 
    search = true;
    pattern =  "";
    found = false;
    search_results.clear();
   }
   if (ch == '>' && !search_results.empty()) {
    result_selected++;
    if (result_selected > search_results.size())
     result_selected = 0;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > mtype::types.size()) {
     a = shift + 23 - mtype::types.size();
     shift = mtype::types.size() - 23;
    }
   } else if (ch == '<' && !search_results.empty()) {
    result_selected--;
    if (result_selected < 0)
     result_selected = search_results.size() - 1;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > mtype::types.size()) {
     a = shift + 23 - mtype::types.size();
     shift = mtype::types.size() - 23;
    }
   }
  }
  if (!search_results.empty())
   mvwprintz(w_list, 0, 11, c_green, "%s               ", pattern.c_str());
  else if (pattern.length() > 0)
   mvwprintz(w_list, 0, 11, c_red, "%s not found!            ",pattern.c_str());
  if (a < 0) {
   a = 0;
   shift--;
   if (shift < 1) shift = 1;
  }
  if (a > 22) {
   a = 22;
   shift++;
   if (shift + 23 > mtype::types.size()) shift = mtype::types.size() - 23;
  }
  for (int i = 1; i < 24; i++) {
   const nc_color col = (i == a + 1 ? h_white : c_white);
   const mtype* const type = mtype::types[i - 1 + shift];
   mvwprintz(w_list, i, 0, col, type->name.c_str());
   wprintz(w_list, type->color, " %c%", type->sym);
  }
  tmp = monster(mtype::types[a + shift]);
  if (friendly) tmp.friendly = -1;
  tmp.print_info(this, w_info);
  wrefresh(w_info);
  wrefresh(w_list);
  ch = (search ? getch() : input());
 } while (ch != '\n');
 clear();
 delwin(w_info);
 delwin(w_list);
 refresh_all();
 wrefresh(w_terrain);
 point spawn = look_around();
 if (spawn.x == -1) return;
 tmp.spawn(spawn.x, spawn.y);
 z.push_back(tmp);
}

void game::mutation_wish()
{
 WINDOW* w_list = newwin(25, 30, 0,  0);
 WINDOW* w_info = newwin(25, 50, 0, 30);
 int a = 0, shift = 0, result_selected = 0;
 long ch = '.';
 bool search = false, found = false;
 std::string pattern;
 std::string info;
 std::vector<int> search_results;
 do {
  werase(w_info);
  werase(w_list);
  mvwprintw(w_list, 0, 0, "Mutate: ");
  if (search) {
   found = false;
   if (ch == '\n') {
    search = false;
    found = true;
    ch = '.';
   } else if (ch == KEY_BACKSPACE || ch == 127) {
    if (pattern.length() > 0)
     pattern.erase(pattern.end() - 1);
   } else if (ch == '>') {
    search = false;
    if (!search_results.empty()) {
     result_selected++;
     if (result_selected > search_results.size())
      result_selected = 0;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > PF_MAX2) {
      a = shift + 23 - PF_MAX2;
      shift = PF_MAX2 - 23;
     }
    }
   } else if (ch == '<') {
    search = false;
    if (!search_results.empty()) {
     result_selected--;
     if (result_selected < 0)
      result_selected = search_results.size() - 1;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > PF_MAX2) {
      a = shift + 23 - PF_MAX2;
      shift = PF_MAX2 - 23;
     }
    }
   } else {
    pattern += ch;
    search_results.clear();
   }

   if (search) {
    for (int i = 0; i < PF_MAX2; i++) {
     if (mutation_branch::traits[i].name.find(pattern) != std::string::npos) {
      shift = i;
      a = 0;
      result_selected = 0;
      if (shift + 23 > PF_MAX2) {
       a = shift + 23 - PF_MAX2;
       shift = PF_MAX2 - 23;
      }
      found = true;
      search_results.push_back(i);
     }
    }
    if (search_results.size() > 0) {
     shift = search_results[0];
     a = 0;
    }
   }

  } else {	// Not searching; scroll by keys
   if (ch == 'j') a++;
   if (ch == 'k') a--;
   if (ch == '/') { 
    search = true;
    pattern =  "";
    found = false;
    search_results.clear();
   }
   if (ch == '>' && !search_results.empty()) {
    result_selected++;
    if (result_selected > search_results.size())
     result_selected = 0;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > PF_MAX2) {
     a = shift + 23 - PF_MAX2;
     shift = PF_MAX2 - 23;
    }
   } else if (ch == '<' && !search_results.empty()) {
    result_selected--;
    if (result_selected < 0)
     result_selected = search_results.size() - 1;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > PF_MAX2) {
     a = shift + 23 - PF_MAX2;
     shift = PF_MAX2 - 23;
    }
   }
  }
  if (!search_results.empty())
   mvwprintz(w_list, 0, 11, c_green, "%s               ", pattern.c_str());
  else if (pattern.length() > 0)
   mvwprintz(w_list, 0, 11, c_red, "%s not found!            ",pattern.c_str());
  if (a < 0) {
   a = 0;
   shift--;
   if (shift < 0) shift = 0;
  }
  if (a > 22) {
   a = 22;
   shift++;
   if (shift + 23 > PF_MAX2) shift = PF_MAX2 - 23;
  }
  for (int i = 1; i < 24; i++) {
   const nc_color col = (i == a + 1 ? h_white : c_white);
   mvwprintz(w_list, i, 0, col, mutation_branch::traits[i-1+shift].name.c_str());
  }
  mvwprintw(w_info, 1, 0, mutation_branch::data[a+shift].valid ? "Valid" : "Nonvalid");
  int line2 = 2;
  mvwprintw(w_info, line2, 0, "Prereqs:");
  for(const auto tmp : mutation_branch::data[a + shift].prereqs) {
   mvwprintw(w_info, line2, 9, mutation_branch::traits[tmp].name.c_str());
   line2++;
  }
  mvwprintw(w_info, line2, 0, "Cancels:");
  for(const auto tmp : mutation_branch::data[a + shift].cancels) {
   mvwprintw(w_info, line2, 9, mutation_branch::traits[tmp].name.c_str());
   line2++;
  }
  mvwprintw(w_info, line2, 0, "Becomes:");
  for(const auto tmp : mutation_branch::data[a + shift].replacements) {
   mvwprintw(w_info, line2, 9, mutation_branch::traits[tmp].name.c_str());
   line2++;
  }
  mvwprintw(w_info, line2, 0, "Add-ons:");
  for(const auto tmp : mutation_branch::data[a + shift].additions) {
   mvwprintw(w_info, line2, 9, mutation_branch::traits[tmp].name.c_str());
   line2++;
  }
  wrefresh(w_info);
  wrefresh(w_list);
  ch = (search ? getch() : input());
 } while (ch != '\n');
 clear();
 if (a+shift == 0) u.mutate(this);
 else u.mutate_towards(this, pl_flag(a + shift));
 delwin(w_info);
 delwin(w_list);
}
