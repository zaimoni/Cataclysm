#include "construction.h"
#ifndef SOCRATES_DAIMON
#include "game.h"
#include "keypress.h"
#include "rng.h"
#include <stdexcept>
#endif

std::vector<constructable*> constructable::constructions; // The list of constructions

#ifndef SOCRATES_DAIMON
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
#endif

void constructable::init()
{
 int id = -1;

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

#ifndef SOCRATES_DAIMON
#define HANDLERS(A,B) , A, B
#else
#define HANDLERS(A,B)
#endif

 constructions.push_back(new constructable(++id, "Dig Pit", 0 HANDLERS(&construct::able_dig, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_pit_shallow, 10));
   constructions.back()->stages.back().tools.push_back({ itm_shovel });
  constructions.back()->stages.push_back(construction_stage(t_pit, 10));
   constructions.back()->stages.back().tools.push_back({ itm_shovel });

 constructions.push_back(new constructable(++id, "Spike Pit", 0 HANDLERS(&construct::able_pit, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_pit_spiked, 5));
   constructions.back()->stages.back().components.push_back({ { itm_spear_wood, 4 } });

 constructions.push_back(new constructable(++id, "Fill Pit", 0 HANDLERS(&construct::able_pit, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_pit_shallow, 5));
   constructions.back()->stages.back().tools.push_back({ itm_shovel});
  constructions.back()->stages.push_back(construction_stage(t_dirt, 5));
   constructions.back()->stages.back().tools.push_back({ itm_shovel});

 constructions.push_back(new constructable(++id, "Chop Down Tree", 0 HANDLERS(&construct::able_tree, &construct::done_tree)));
  constructions.back()->stages.push_back(construction_stage(t_dirt, 10));
   constructions.back()->stages.back().tools.push_back({ itm_ax, itm_chainsaw_on});

 constructions.push_back(new constructable(++id, "Chop Up Log", 0 HANDLERS(&construct::able_log, &construct::done_log)));
  constructions.back()->stages.push_back(construction_stage(t_dirt, 20));
   constructions.back()->stages.back().tools.push_back({ itm_ax, itm_chainsaw_on});

 constructions.push_back(new constructable(++id, "Clean Broken Window", 0 HANDLERS(&construct::able_broken_window, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_window_empty, 5));

 constructions.push_back(new constructable(++id, "Remove Window Pane",  1 HANDLERS(&construct::able_window_pane, &construct::done_window_pane)));
  constructions.back()->stages.push_back(construction_stage(t_window_empty, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_rock, itm_hatchet});
   constructions.back()->stages.back().tools.push_back({ itm_screwdriver, itm_knife_butter, itm_toolset });

 constructions.push_back(new constructable(++id, "Repair Door", 1 HANDLERS(&construct::able_door_broken, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_door_c, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 3 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 12 } });

 constructions.push_back(new constructable(++id, "Board Up Door", 0 HANDLERS(&construct::able_door, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_door_boarded, 8));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 4 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 8 } });

 constructions.push_back(new constructable(++id, "Board Up Window", 0 HANDLERS(&construct::able_window, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_window_boarded, 5));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 4 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 8 } });

 constructions.push_back(new constructable(++id, "Build Wall", 2 HANDLERS(&construct::able_empty, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_wall_half, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 10 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 20 } });
  constructions.back()->stages.push_back(construction_stage(t_wall_wood, 10));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 10 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 20 } });

 constructions.push_back(new constructable(++id, "Build Window", 3 HANDLERS(&construct::able_wall_wood, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_window_empty, 10));
   constructions.back()->stages.back().tools.push_back({ itm_saw });
  constructions.back()->stages.push_back(construction_stage(t_window, 5));
  constructions.back()->stages.back().components.push_back({ { itm_glass_sheet, 1 } });

 constructions.push_back(new constructable(++id, "Build Door", 4 HANDLERS(&construct::able_wall_wood, &construct::done_nothing)));
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

 constructions.push_back(new constructable(++id, "Build Roof", 4 HANDLERS(&construct::able_between_walls, &construct::done_nothing)));
  constructions.back()->stages.push_back(construction_stage(t_floor, 40));
   constructions.back()->stages.back().tools.push_back({ itm_hammer, itm_hatchet, itm_nailgun });
   constructions.back()->stages.back().components.push_back({ { itm_2x4, 8 } });
   constructions.back()->stages.back().components.push_back({ { itm_nail, 40 } });

 constructions.push_back(new constructable(++id, "Start vehicle construction", 0 HANDLERS(&construct::able_empty, &construct::done_vehicle)));
  constructions.back()->stages.push_back(construction_stage(t_null, 10));
  constructions.back()->stages.back().components.push_back({ { itm_frame, 1 } });

}

#ifndef SOCRATES_DAIMON
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

 return will_flood_stop(&m, fill, p.x, p.y);
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
 for (const auto& pt : line_to(p, dest, rng(1, 8))) { // rng values not necessarily "legal" from map::sees
	 g->m.destroy(g, pt.x, pt.y, true);
	 g->m.ter(pt) = t_log;
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
#endif