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
 mvwaddstrz(w_con, 0, 1, c_red, "Construction");
 mvwputch(w_con,  0, VBAR_X, c_white, LINE_OXXX);
 mvwputch(w_con, VIEW-1, VBAR_X, c_white, LINE_XXOX);
 for (int i = 1; i < VIEW-1; i++)
  mvwputch(w_con, i, VBAR_X, c_white, LINE_XOXO);

 mvwaddstrz(w_con,  1, VBAR_X + 1, c_white, "Difficulty:");

 wrefresh(w_con);

 const int listing_length = VIEW - TABBED_HEADER_HEIGHT;
 bool update_info = true;
 int select = 0;
 int ch;

 inventory total_inv(u.GPSpos, PICKUP_RANGE);
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
   mvwaddstrz(w_con, 1 + i, 1, col, cur->name.c_str());
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
                mvwaddstrz(w_con, posy, posx, c_white, "OR");
                posx += 3;
            }
            const nc_color col = total_inv.has_amount(tool, 1) ? c_green : c_red;
            const itype* const t_type = item::types[tool];
            const int length = t_type->name.length();
            if (posx + length > SCREEN_WIDTH - 1) {
                posy++;
                posx = VBAR_X + 3;
            }
            mvwaddstrz(w_con, posy, posx, col, t_type->name.c_str());
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
                mvwaddstrz(w_con, posy, posx, c_white, "OR");
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
 inventory total_inv(u.GPSpos, PICKUP_RANGE);
 total_inv.add_stack(u.inv_dump());

 std::vector<std::pair<point,std::pair<int,int>>> valid_delta;

 for (decltype(auto) delta : Direction::vector) {
   const point pt(u.pos + delta);
   const auto loc(u.GPSpos + delta);
   int starting_stage = (con->able_gps) ? ((*(con->able_gps))(loc) ? 0 : -1)
                                        : ((*(con->able))(m, pt) ? 0 : -1);
   if (0 > starting_stage) {
       const ter_id ter = loc.ter();
       int n = -1;
       for (decltype(auto) stage : con->stages) {
           n++;
           if (ter == stage.terrain) {
               starting_stage = n;
               break;
           }
       }
   }

   if (0 <= starting_stage) {
// Make sure we're not trying to continue a construction that we can't finish
    int max_stage = -1;
    for (int i = starting_stage; i < con->stages.size(); i++) {
     if (player_can_build(u, total_inv, con, i, true)) max_stage = i;
    }

    if (max_stage >= starting_stage) {
     valid_delta.push_back(std::pair(delta,std::pair(starting_stage, max_stage)));
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

 std::optional<std::pair<int, int>> stage_origin;
 for (const auto& x : valid_delta) {
     if (x.first == dir) {
         stage_origin = x.second;
         break;
     }
 }
 if (!stage_origin) {
     messages.add("You cannot build there!");
     return;
 }

 const auto dest = u.GPSpos + dir;
 dir += u.pos;

 const int starting_stage = stage_origin->first;
 const int max_stage = stage_origin->second;

 u.assign_activity(ACT_BUILD, con->stages[starting_stage].time * 1000, dir, con->id);

 u.moves = 0;
 std::vector<int> stages;
 for (int i = starting_stage; i <= max_stage; i++)
  stages.push_back(i);
 u.activity.values = std::move(stages);
 u.activity.placement = dir;
 u.activity.gps_placement = dest;
}

void player::complete_construction()
{
    inventory map_inv(GPSpos, PICKUP_RANGE);
    const constructable* const built = constructable::constructions[activity.index];
    construction_stage stage = built->stages[activity.values[0]];

    practice(sk_carpentry, 10 * clamped_lb<1>(built->difficulty));
    for (const auto& comp : stage.components) if (!comp.empty()) consume_items(*this, comp);

    const auto g = game::active();

    // Make the terrain change
    if (stage.terrain != t_null) (_ref<GPS_loc>::invalid != activity.gps_placement ? activity.gps_placement.ter() : g->m.ter(activity.placement)) = stage.terrain;

    // Strip off the first stage in our list...
    EraseAt(activity.values, 0);
    // ...and start the next one, if it exists
    if (!activity.values.empty()) {
        construction_stage next = built->stages[activity.values[0]];
        activity.moves_left = next.time * 1000;
    }
    else // We're finished!
        activity.type = ACT_NULL;

    // This comes after clearing the activity, in case the function interrupts
    // activities
    if (built->done_gps && _ref<GPS_loc>::invalid != activity.gps_placement) (*(built->done_gps))(activity.gps_placement);
    else if (built->done_pl) (*(built->done_pl))(*this);
}
