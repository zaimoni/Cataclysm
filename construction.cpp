#include "construction.h"
#include "game.h"
#include "output.h"
#include "keypress.h"
#include "inventory.h"
#include "mapdata.h"
#include "skill.h"
#include "crafting.h" // For the use_comps use_tools functions
#include "stl_typetraits.h"
#include "line.h"
#include "rng.h"
#include "recent_msg.h"
#include "zero.h"
#include <stdexcept>

std::vector<constructable*> constructable::constructions; // The list of constructions

struct construct // Construction functions.
{
	// Bools - able to build at the given point?
	static bool able_empty(map&, point); // Able if tile is empty

	static bool able_window(map&, point); // Any window tile
	static bool able_window_pane(map&, point); // Only intact windows
	static bool able_broken_window(map&, point); // Able if tile is broken window

	static bool able_door(map&, point); // Any door tile
	static bool able_door_broken(map&, point); // Broken door

	static bool able_wall(map&, point); // Able if tile is wall
	static bool able_wall_wood(map&, point); // Only player-built walls

	static bool able_between_walls(map&, point); // Flood-fill contained by walls

	static bool able_dig(map&, point); // Able if diggable terrain
	static bool able_pit(map&, point); // Able only on pits

	static bool able_tree(map&, point); // Able on trees
	static bool able_log(map&, point); // Able on logs

										 // Does anything special happen when we're finished?
	static void done_nothing(game *, point) { }
	static void done_window_pane(game *, point);
	static void done_vehicle(game *, point);
	static void done_tree(game *, point);
	static void done_log(game *, point);

};

static bool will_flood_stop(map *m, bool (&fill)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                            int x, int y)
{
 if (x == 0 || y == 0 || x == SEEX * MAPSIZE - 1 || y == SEEY * MAPSIZE - 1)
  return false;

 fill[x][y] = true;
 bool skip_north = (fill[x][y - 1] || m->has_flag(supports_roof, x, y - 1)),
      skip_south = (fill[x][y + 1] || m->has_flag(supports_roof, x, y + 1)),
      skip_east  = (fill[x + 1][y] || m->has_flag(supports_roof, x + 1, y)),
      skip_west  = (fill[x - 1][y] || m->has_flag(supports_roof, x - 1, y));

 return ((skip_north || will_flood_stop(m, fill, x    , y - 1)) &&
         (skip_east  || will_flood_stop(m, fill, x + 1, y    )) &&
         (skip_south || will_flood_stop(m, fill, x    , y + 1)) &&
         (skip_west  || will_flood_stop(m, fill, x - 1, y    ))   );
}

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

void constructable::init()
{
 int id = -1;
 int tl, cl;

/* CONSTRUCT( name, time, able, done )
 * Name is the name as it appears in the menu; 30 characters or less, please.
 * time is the time in MINUTES that it takes to finish this construction.
 *  note that 10 turns = 1 minute.
 * able is a function which returns true if you can build it on a given tile
 *  See construction.h for options, and this file for definitions.
 * done is a function which runs each time the construction finishes.
 *  This is useful, for instance, for removing the trap from a pit, or placing
 *  items after a deconstruction.
 */

 constructions.push_back(new constructable(++id, "Dig Pit", 0, &construct::able_dig, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_pit_shallow, 10));
   constructions.back()->stages.back().tools.push_back({ itm_shovel });
  constructions.back()->stages.push_back(construction_stage(t_pit, 10));
   constructions.back()->stages.back().tools.push_back({ itm_shovel });

 constructions.push_back(new constructable(++id, "Spike Pit", 0, &construct::able_pit, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_pit_spiked, 5));
   constructions.back()->stages.back().components.push_back({ { itm_spear_wood, 4 } });

 constructions.push_back(new constructable(++id, "Fill Pit", 0, &construct::able_pit, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_pit_shallow, 5));
   constructions.back()->stages.back().tools.push_back({ itm_shovel});
  constructions.back()->stages.push_back(construction_stage(t_dirt, 5));
   constructions.back()->stages.back().tools.push_back({ itm_shovel});

 constructions.push_back(new constructable(++id, "Chop Down Tree", 0, &construct::able_tree, &construct::done_tree));
  constructions.back()->stages.push_back(construction_stage(t_dirt, 10));
   constructions.back()->stages.back().tools.push_back({ itm_ax, itm_chainsaw_on});

 constructions.push_back(new constructable(++id, "Chop Up Log", 0, &construct::able_log, &construct::done_log));
  constructions.back()->stages.push_back(construction_stage(t_dirt, 20));
   constructions.back()->stages.back().tools.push_back({ itm_ax, itm_chainsaw_on});

 constructions.push_back(new constructable(++id, "Clean Broken Window", 0, &construct::able_broken_window, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_window_empty, 5));

 constructions.push_back(new constructable(++id, "Remove Window Pane",  1, &construct::able_window_pane, &construct::done_window_pane));
  constructions.back()->stages.push_back(construction_stage(t_window_empty, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_rock, itm_hatchet});
   constructions.back()->stages.back().tools.push_back({ itm_screwdriver, itm_knife_butter, itm_toolset });

 constructions.push_back(new constructable(++id, "Repair Door", 1, &construct::able_door_broken, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_door_c, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 3 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 12 } });

 constructions.push_back(new constructable(++id, "Board Up Door", 0, &construct::able_door, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_door_boarded, 8));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 4 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 8 } });

 constructions.push_back(new constructable(++id, "Board Up Window", 0, &construct::able_window, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_window_boarded, 5));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 4 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 8 } });

 constructions.push_back(new constructable(++id, "Build Wall", 2, &construct::able_empty, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_wall_half, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 10 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 20 } });
  constructions.back()->stages.push_back(construction_stage(t_wall_wood, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 10 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 20 } });

 constructions.push_back(new constructable(++id, "Build Window", 3, &construct::able_wall_wood, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_window_empty, 10));
   constructions.back()->stages.back().tools.push_back({ itm_saw });
  constructions.back()->stages.push_back(construction_stage(t_window, 5));
  constructions.back()->stages.back().components.push_back({ { itm_glass_sheet, 1 } });

 constructions.push_back(new constructable(++id, "Build Door", 4, &construct::able_wall_wood, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_door_frame, 15));
   constructions.back()->stages.back().tools.push_back({ itm_saw });
  constructions.back()->stages.push_back(construction_stage(t_door_b, 15));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 4 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 12 } });
  constructions.back()->stages.push_back(construction_stage(t_door_c, 15));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 4 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 12 } });

/*  Removed until we have some way of auto-aligning fences!
 constructions.push_back(new constructable(++id, "Build Fence", 1, 15, &construct::able_empty));
  constructions[id]->stages.push_back(construction_stage(t_fence_h, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet });
   constructions.back()->stages.back().components({ { itm_2x4, 5, itm_nail, 8 } });
*/

 constructions.push_back(new constructable(++id, "Build Roof", 4, &construct::able_between_walls, &construct::done_nothing));
  constructions.back()->stages.push_back(construction_stage(t_floor, 40));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 8 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 40 } });

 constructions.push_back(new constructable(++id, "Start vehicle construction", 0, &construct::able_empty, &construct::done_vehicle));
  constructions.back()->stages.push_back(construction_stage(t_null, 10));
  constructions.back()->stages.back().components.push_back({ { itm_frame, 1 } });

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

bool construct::able_empty(map& m, point p)
{
 return m.move_cost(p) == 2;
}

bool construct::able_tree(map& m, point p)
{
 return m.ter(p) == t_tree;
}

bool construct::able_log(map& m, point p)
{
 return m.ter(p) == t_log;
}

bool construct::able_window(map& m, point p)
{
 const auto t = m.ter(p);
 return t_window_frame == t || t_window_empty == t || t_window == t;
}

bool construct::able_window_pane(map& m, point p)
{
 return m.ter(p) == t_window;
}

bool construct::able_broken_window(map& m, point p)
{
 return m.ter(p) == t_window_frame;
}

bool construct::able_door(map& m, point p)
{
 const auto t = m.ter(p);
 return t_door_c == t || t_door_b == t || t_door_o == t || t_door_locked == t;
}

bool construct::able_door_broken(map& m, point p)
{
 return m.ter(p) == t_door_b;
}

bool construct::able_wall(map& m, point p)
{
 const auto t = m.ter(p);
 return t_wall_h == t || t_wall_v == t || t_wall_wood == t;
}

bool construct::able_wall_wood(map& m, point p)
{
 return m.ter(p) == t_wall_wood;
}

bool construct::able_between_walls(map& m, point p)
{
 bool fill[SEEX * MAPSIZE][SEEY * MAPSIZE];
 for (int x = 0; x < SEEX * MAPSIZE; x++) {
  for (int y = 0; y < SEEY * MAPSIZE; y++)
   fill[x][y] = false;
 }

 return will_flood_stop(&m, fill, p.x, p.y); // See bottom of file
}

bool construct::able_dig(map& m, point p)
{
 return m.has_flag(diggable, p);
}

bool construct::able_pit(map& m, point p)
{
 return (m.ter(p) == t_pit);//|| m.ter(p) == t_pit_shallow);
}

void construct::done_window_pane(game *g, point p)
{
 g->m.add_item(g->u.pos, item::types[itm_glass_sheet], 0);
}

void construct::done_tree(game *g, point p)
{
 mvprintz(0, 0, c_red, "Press a direction for the tree to fall in:");
 point dest(-2, -2);
 while (-2 == dest.x) dest = get_direction(input());
 ((dest *= 3) += p) += point(rng(-1, 1), rng(-1, 1));
 std::vector<point> tree = line_to(p, dest, rng(1, 8));
 for (int i = 0; i < tree.size(); i++) {
  g->m.destroy(g, tree[i].x, tree[i].y, true);
  g->m.ter(tree[i]) = t_log;
 }
}

void construct::done_log(game *g, point p)
{
 int num_sticks = rng(10, 20);
 for (int i = 0; i < num_sticks; i++)
  g->m.add_item(p, item::types[itm_2x4], int(messages.turn));
}

void construct::done_vehicle(game *g, point p)
{
    if (!map::in_bounds(p)) {
#ifndef NDEBUG
        throw std::logic_error("tried to construct vehicle outside of reality bubble");
#else
        debuglog("tried to construct vehicle outside of reality bubble");
#endif
        return;
    }

    std::string name = string_input_popup(20, "Enter new vehicle name");
	if (auto veh = g->m.add_vehicle(veh_custom, p, 270)) {
		veh->name = std::move(name);
		veh->install_part(0, 0, vp_frame_v2);
		return;
	}
    debugmsg("error constructing vehicle");
}
