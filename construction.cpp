#include "construction.h"
#include "game.h"
#include "keypress.h"
#include "stl_typetraits.h"

static bool player_can_build(player& p, inventory inv, const constructable* con,
                             int level = -1, bool cont = false)
{
 if (p.sklevel[sk_carpentry] < con->difficulty) return false;

 if (level < 0) level = con->stages.size();

 int start = cont ? level : 0;
 for (int i = start; i < con->stages.size() && i <= level; i++) {
  construction_stage stage = con->stages[i];
  for (decltype(auto) min_term : stage.tools) {
    bool has_tool = false;
	for (const auto tool : min_term) {
     if (inv.has_amount(tool, 1)) {
	  has_tool = true;
	  break;
	 }
    }
    if (!has_tool) return false;
  }
  for (decltype(auto) min_term : stage.components) {
    bool has_component = false;
	for(const auto& part : min_term) {
	 if (item::types[part.type]->is_ammo() ? inv.has_charges(part.type, part.count) : inv.has_amount(part.type, part.count)) {
       has_component = true;
	   break;
	 }
    }
    if (!has_component) return false;
  }
 }
 return true;
}

void game::construction_menu()
{
 const auto c_size = constructable::constructions.size();
 WINDOW *w_con = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 wborder(w_con, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w_con, 0, 1, c_red, "Construction");
 mvwputch(w_con,  0, VBAR_X, c_white, LINE_OXXX);
 mvwputch(w_con, VIEW-1, VBAR_X, c_white, LINE_XXOX);
 for (int i = 1; i < VIEW-1; i++)
  mvwputch(w_con, i, VBAR_X, c_white, LINE_XOXO);

 mvwprintz(w_con,  1, VBAR_X + 1, c_white, "Difficulty:");

 wrefresh(w_con);

 const int listing_length = VIEW - TABBED_HEADER_HEIGHT;
 bool update_info = true;
 int select = 0;
 int ch;

 inventory total_inv;
 total_inv.form_from_map(m, u.pos, PICKUP_RANGE);
 total_inv.add_stack(u.inv_dump());
 if (u.has_bionic(bio_tools)) {
  item tools(item::types[itm_toolset], messages.turn);
  tools.charges = u.power_level;
  total_inv += tools;
 }

 do {
// Erase existing list of constructions
  for (int i = 1; i < VIEW-1; i++) {
   for (int j = 1; j < VBAR_X - 1; j++)
    mvwputch(w_con, i, j, c_black, 'x');
  }
// Determine where in the master list to start printing
  int offset = clamped(select - listing_length / 2, 0, c_size - listing_length);
// Print the constructions between offset and max (or how many will fit)
  for (int i = 0; i < listing_length && i + offset < c_size; i++) {
   int current = i + offset;
   const constructable* const cur = constructable::constructions[current];
   nc_color col = (player_can_build(u, total_inv, cur, 0) ? c_white : c_dkgray);
   if (current == select) col = hilite(col);
   mvwprintz(w_con, 1 + i, 1, col, cur->name.c_str());
  }

  if (update_info) {
   update_info = false;
   const constructable* const current_con = constructable::constructions[select];
// Print difficulty
   int pskill = u.sklevel[sk_carpentry], diff = current_con->difficulty;
   mvwprintz(w_con, 1, VBAR_X + 13, (pskill >= diff ? c_white : c_red), "%d   ", diff);
// Clear out lines for tools & materials
   for (int i = 2; i < VIEW - 1; i++) {
    for (int j = VBAR_X + 1; j < SCREEN_WIDTH - 1; j++)
     mvwputch(w_con, i, j, c_black, 'x');
   }

// Print stages and their requirements
   int posx = VBAR_X + 3, posy = 2;
   for (int n = 0; n < current_con->stages.size(); n++) {
    const nc_color color_stage = (player_can_build(u, total_inv, current_con, n) ? c_white : c_dkgray);
	const construction_stage& stage = current_con->stages[n];
	mvwprintz(w_con, posy, VBAR_X + 1, color_stage, "Stage %d: %s", n + 1,
              stage.terrain == t_null? "" : ter_t::list[stage.terrain].name.c_str());
    posy++;
// Print tools (historically fails if more than three tool minterms)
    for (decltype(auto) min_term : stage.tools) {
        posy++;
        posx = VBAR_X + 3;
        int j = -1;
        for (const itype_id tool : min_term) {
            ++j;
            if (0 < j) { // "OR" if there's more
                if (posx > SCREEN_WIDTH - 3) {
                    posy++;
                    posx = VBAR_X + 3;
                }
                mvwprintz(w_con, posy, posx, c_white, "OR");
                posx += 3;
            }
            const nc_color col = total_inv.has_amount(tool, 1) ? c_green : c_red;
            const itype* const t_type = item::types[tool];
            const int length = t_type->name.length();
            if (posx + length > SCREEN_WIDTH - 1) {
                posy++;
                posx = VBAR_X + 3;
            }
            mvwprintz(w_con, posy, posx, col, t_type->name.c_str());
            posx += length + 1; // + 1 for an empty space
        }
    }

    // Print components (fails if more than three component minterms)
    posy++;
    posx = VBAR_X + 3;
    for (decltype(auto) min_term : stage.components) {
        posx = VBAR_X + 3;
        int j = -1;
        for (decltype(auto) comp : min_term) {
            ++j;
            if (0 < j) { // "OR" if there's more
                if (posx > SCREEN_WIDTH - 3) {
                    posy++;
                    posx = VBAR_X + 3;
                }
                mvwprintz(w_con, posy, posx, c_white, "OR");
                posx += 3;
            }
            nc_color col = c_red;
            if (item::types[comp.type]->is_ammo() ? total_inv.has_charges(comp.type, comp.count) : total_inv.has_amount(comp.type, comp.count)) {
                col = c_green;
            }
            int length = item::types[comp.type]->name.length();
            if (posx + length > SCREEN_WIDTH - 1) {
                posy++;
                posx = VBAR_X + 3;
            }
            mvwprintz(w_con, posy, posx, col, "%s x%d", item::types[comp.type]->name.c_str(), comp.count);
            posx += length + 3; // + 2 for " x", + 1 for an empty space
            posx += 1 + int_log10(comp.count);    // Add more space for the length of the count
        }
        posy++;
    }
   }
   wrefresh(w_con);
  } // Finished updating
 
  ch = input();
  switch (ch) {
   case 'j':
    update_info = true;
    if (select < c_size - 1) select++;
    else select = 0;
    break;
   case 'k':
    update_info = true;
    if (select > 0) select--;
    else select = c_size - 1;
    break;
   case '\n':
   case 'l':
    {
    const constructable* const current_con = constructable::constructions[select];
    if (player_can_build(u, total_inv, current_con, 0)) {
     place_construction(current_con);
     ch = 'q';
    } else {
     popup("You can't build that!");
     for (int i = 1; i < VIEW - 1; i++)
      mvwputch(w_con, i, VBAR_X, c_white, LINE_XOXO);
     update_info = true;
    }
	}
    break;
  }
 } while (ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);
 refresh_all();
}

void game::place_construction(const constructable * const con)
{
 refresh_all();
 inventory total_inv;
 total_inv.form_from_map(m, u.pos, PICKUP_RANGE);
 total_inv.add_stack(u.inv_dump());

 std::vector<point> valid;
 for (decltype(auto) delta : Direction::vector) {
   const point pt(u.pos + delta);
   bool place_okay = (*(con->able))(m, pt);
   for (int i = 0; i < con->stages.size() && !place_okay; i++) {
    if (m.ter(pt) == con->stages[i].terrain) place_okay = true;
   }

   if (place_okay) {
// Make sure we're not trying to continue a construction that we can't finish
    int starting_stage = 0, max_stage = 0;
    for (int i = 0; i < con->stages.size(); i++) {
     if (m.ter(pt) == con->stages[i].terrain) starting_stage = i + 1;
     if (player_can_build(u, total_inv, con, i, true)) max_stage = i;
    }

    if (max_stage >= starting_stage) {
     valid.push_back(pt);
     m.drawsq(w_terrain, u, pt.x, pt.y, true, false);
     wrefresh(w_terrain);
    }
   }
 }
 mvprintz(0, 0, c_red, "Pick a direction in which to construct:");
 point dir = get_direction(input());
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 dir += u.pos;
 if (!cataclysm::any(valid, dir)) {
  messages.add("You cannot build there!");
  return;
 }

// Figure out what stage to start at, and what stage is the maximum
 int starting_stage = 0, max_stage = 0;
 for (int i = 0; i < con->stages.size(); i++) {
  if (m.ter(dir) == con->stages[i].terrain)
   starting_stage = i + 1;
  if (player_can_build(u, total_inv, con, i, true))
   max_stage = i;
 }

 u.assign_activity(ACT_BUILD, con->stages[starting_stage].time * 1000, dir, con->id);

 u.moves = 0;
 std::vector<int> stages;
 for (int i = starting_stage; i <= max_stage; i++)
  stages.push_back(i);
 u.activity.values = std::move(stages);
}

void game::complete_construction()
{
 inventory map_inv;
 map_inv.form_from_map(m, u.pos, PICKUP_RANGE);
 const constructable* const built = constructable::constructions[u.activity.index];
 construction_stage stage = built->stages[u.activity.values[0]];

 u.practice(sk_carpentry, built->difficulty * 10);
 if (built->difficulty < 1) u.practice(sk_carpentry, 10);
 for(const auto& comp : stage.components) if (!comp.empty()) consume_items(m, u, comp);
 
// Make the terrain change
 if (stage.terrain != t_null) m.ter(u.activity.placement) = stage.terrain;

// Strip off the first stage in our list...
 EraseAt(u.activity.values, 0);
// ...and start the next one, if it exists
 if (!u.activity.values.empty()) {
  construction_stage next = built->stages[u.activity.values[0]];
  u.activity.moves_left = next.time * 1000;
 } else // We're finished!
  u.activity.type = ACT_NULL;

// This comes after clearing the activity, in case the function interrupts
// activities
 (*(built->done))(this, u.activity.placement);
}
