#include "mission.h"
#include "game.h"
#include "recent_msg.h"
#include "setvector.h"

std::vector <mission_type> mission_type::types; // The list of mission templates

struct mission_place {	// Return true if [posx,posy] is valid in overmap
	static bool never(game *g, int posx, int posy) { return false; }
	static bool always(game *g, int posx, int posy) { return true; }
	static bool near_town(game *g, int posx, int posy);
};

/* mission_start functions are first run when a mission is accepted; this
* initializes the mission's key values, like the target and description.
* These functions are also run once a turn for each active mission, to check
* if the current goal has been reached.  At that point they either start the
* goal, or run the appropriate mission_end function.
*/
struct mission_start {
	static void standard(game *, mission *); // Standard for its goal type
	static void infect_npc(game *, mission *); // DI_INFECTION, remove antibiotics
	static void place_dog(game *, mission *); // Put a dog in a house!
	static void place_zombie_mom(game *, mission *); // Put a zombie mom in a house!
	static void place_npc_software(game *, mission *); // Put NPC-type-dependent software
	static void reveal_hospital(game *, mission *); // Reveal the nearest hospital
	static void find_safety(game *, mission *); // Goal is set to non-spawn area
	static void place_book(game *, mission *); // Place a book to retrieve
};

struct mission_end {	// These functions are run when a mission ends
	static void standard(game *, mission *) {}; // Nothing special happens
	static void heal_infection(game *, mission *);
};

struct mission_fail {
	static void standard(game *, mission *) {}; // Nothing special happens
	static void kill_npc(game *, mission *); // Kill the NPC who assigned it!
};

bool mission_place::near_town(game *g, int posx, int posy)
{
	return (g->cur_om.dist_from_city(point(posx, posy)) <= 40);
}

/* These functions are responsible for making changes to the game at the moment
* the mission is accepted by the player.  They are also responsible for
* updating *miss with the target and any other important information.
*/

void mission_start::standard(game *g, mission *miss)
{
}

void mission_start::infect_npc(game *g, mission *miss)
{
	npc* const p = g->find_npc(miss->npc_id);
	if (p == NULL) {
		debugmsg("mission_start::infect_npc() couldn't find an NPC!");
		return;
	}
	p->add_disease(DI_INFECTION, -1, g);
	for (size_t i = 0; i < p->inv.size(); i++) {
		if (p->inv[i].type->id == itm_antibiotics) {
			p->inv.remove_stack(i);
			i--;
		}
	}
}

void mission_start::place_dog(game *g, mission *miss)
{
	int city_id = g->cur_om.closest_city(g->om_location());
	point house = g->cur_om.random_house_in_city(city_id);
	const npc* const dev = g->find_npc(miss->npc_id);
	if (dev == NULL) {
		debugmsg("Couldn't find NPC! %d", miss->npc_id);
		return;
	}
	g->u.i_add(item(item::types[itm_dog_whistle], 0));
	messages.add("%s gave you a dog whistle.", dev->name.c_str());

	miss->target = house;
	// Make it seen on our map
	for (int x = house.x - 6; x <= house.x + 6; x++) {
		for (int y = house.y - 6; y <= house.y + 6; y++) g->cur_om.seen(x, y) = true;
	}

	tinymap doghouse;
	doghouse.load(g, house.x * 2, house.y * 2);
	doghouse.add_spawn(mon_dog, 1, SEEX, SEEY, true, -1, miss->uid);
	doghouse.save(&(g->cur_om), int(messages.turn), house.x * 2, house.y * 2);
}

void mission_start::place_zombie_mom(game *g, mission *miss)
{
	int city_id = g->cur_om.closest_city(g->om_location());
	point house = g->cur_om.random_house_in_city(city_id);

	miss->target = house;
	// Make it seen on our map
	for (int x = house.x - 6; x <= house.x + 6; x++) {
		for (int y = house.y - 6; y <= house.y + 6; y++) g->cur_om.seen(x, y) = true;
	}

	tinymap zomhouse;
	zomhouse.load(g, house.x * 2, house.y * 2);
	zomhouse.add_spawn(mon_zombie, 1, SEEX, SEEY, false, -1, miss->uid, random_first_name(false));
	zomhouse.save(&(g->cur_om), int(messages.turn), house.x * 2, house.y * 2);
}

void mission_start::place_npc_software(game *g, mission *miss)
{
	npc* dev = g->find_npc(miss->npc_id);
	if (dev == NULL) {
		debugmsg("Couldn't find NPC! %d", miss->npc_id);
		return;
	}
	g->u.i_add(item(item::types[itm_usb_drive], 0));
	messages.add("%s gave you a USB drive.", dev->name.c_str());

	oter_id ter = ot_house_north;

	switch (dev->myclass) {
	case NC_HACKER:
		miss->item_id = itm_software_hacking;
		break;
	case NC_DOCTOR:
		miss->item_id = itm_software_medical;
		ter = ot_s_pharm_north;
		miss->follow_up = MISSION_GET_ZOMBIE_BLOOD_ANAL;
		break;
	case NC_SCIENTIST:
		miss->item_id = itm_software_math;
		break;
	default:
		miss->item_id = itm_software_useless;
	}

	int dist = 0;
	point place;
	if (ter == ot_house_north) {
		int city_id = g->cur_om.closest_city(g->om_location());
		place = g->cur_om.random_house_in_city(city_id);
	}
	else
		place = g->cur_om.find_closest(g->om_location(), ter, 4, dist, false);
	miss->target = place;
	// Make it seen on our map
	for (int x = place.x - 6; x <= place.x + 6; x++) {
		for (int y = place.y - 6; y <= place.y + 6; y++)
			g->cur_om.seen(x, y) = true;
	}
	tinymap compmap;
	compmap.load(g, place.x * 2, place.y * 2);
	point comppoint;

	switch (g->cur_om.ter(place.x, place.y)) {
	case ot_house_north:
	case ot_house_east:
	case ot_house_west:
	case ot_house_south: {
		std::vector<point> valid;
		for (int x = 0; x < SEEX * 2; x++) {
			for (int y = 0; y < SEEY * 2; y++) {
				if (compmap.ter(x, y) == t_floor) {
					bool okay = false;
					for (int x2 = x - 1; x2 <= x + 1 && !okay; x2++) {
						for (int y2 = y - 1; y2 <= y + 1 && !okay; y2++) {
							if (compmap.ter(x2, y2) == t_bed || compmap.ter(x2, y2) == t_dresser) {
								okay = true;
								valid.push_back(point(x, y));
							}
						}
					}
				}
			}
		}
		if (valid.empty())
			comppoint = point(rng(6, SEEX * 2 - 7), rng(6, SEEY * 2 - 7));
		else
			comppoint = valid[rng(0, valid.size() - 1)];
	} break;
	case ot_s_pharm_north: {
		bool found = false;
		for (int x = SEEX * 2 - 1; x > 0 && !found; x--) {
			for (int y = SEEY * 2 - 1; y > 0 && !found; y--) {
				if (compmap.ter(x, y) == t_floor) {
					found = true;
					comppoint = point(x, y);
				}
			}
		}
	} break;
	case ot_s_pharm_east: {
		bool found = false;
		for (int x = 0; x < SEEX * 2 && !found; x++) {
			for (int y = SEEY * 2 - 1; y > 0 && !found; y--) {
				if (compmap.ter(x, y) == t_floor) {
					found = true;
					comppoint = point(x, y);
				}
			}
		}
	} break;
	case ot_s_pharm_south: {
		bool found = false;
		for (int x = 0; x < SEEX * 2 && !found; x++) {
			for (int y = 0; y < SEEY * 2 && !found; y++) {
				if (compmap.ter(x, y) == t_floor) {
					found = true;
					comppoint = point(x, y);
				}
			}
		}
	} break;
	case ot_s_pharm_west: {
		bool found = false;
		for (int x = SEEX * 2 - 1; x > 0 && !found; x--) {
			for (int y = 0; y < SEEY * 2 && !found; y++) {
				if (compmap.ter(x, y) == t_floor) {
					found = true;
					comppoint = point(x, y);
				}
			}
		}
	} break;
	}

	std::stringstream compname;
	compname << dev->name << "'s Terminal";
	compmap.ter(comppoint.x, comppoint.y) = t_console;
	computer* const tmpcomp = compmap.add_computer(comppoint.x, comppoint.y, compname.str(), 0);
	tmpcomp->mission_id = miss->uid;
	tmpcomp->add_option("Download Software", COMPACT_DOWNLOAD_SOFTWARE, 0);

	compmap.save(&(g->cur_om), int(messages.turn), place.x * 2, place.y * 2);
}

void mission_start::reveal_hospital(game *g, mission *miss)
{
	const npc* const dev = g->find_npc(miss->npc_id);
	if (dev != NULL) {
		g->u.i_add(item(item::types[itm_vacutainer], 0));
		messages.add("%s gave you a vacutainer.", dev->name.c_str());
	}
	int dist = 0;
	point place = g->cur_om.find_closest(g->om_location(), ot_hospital, 1, dist, false);
	for (int x = place.x - 3; x <= place.x + 3; x++) {
		for (int y = place.y - 3; y <= place.y + 3; y++) g->cur_om.seen(x, y) = true;
	}
	miss->target = place;
}

void mission_start::find_safety(game *g, mission *miss)
{
	point place = g->om_location();
	bool done = false;
	for (int radius = 0; radius <= 20 && !done; radius++) {
		for (int dist = 0 - radius; dist <= radius && !done; dist++) {
			int offset = rng(0, 3); // Randomizes the direction we check first
			for (int i = 0; i <= 3 && !done; i++) { // Which direction?
				point check = place;
				switch ((offset + i) % 4) {
				case 0: check.x += dist; check.y -= radius; break;
				case 1: check.x += dist; check.y += radius; break;
				case 2: check.y += dist; check.x -= radius; break;
				case 3: check.y += dist; check.x += radius; break;
				}
				if (g->cur_om.is_safe(check.x, check.y)) {
					miss->target = check;
					done = true;
				}
			}
		}
	}
	if (!done) { // Couldn't find safety; so just set the target to far away
		switch (rng(0, 3)) {
		case 0: miss->target = point(place.x - 20, place.y - 20); break;
		case 1: miss->target = point(place.x - 20, place.y + 20); break;
		case 2: miss->target = point(place.x + 20, place.y - 20); break;
		case 3: miss->target = point(place.x + 20, place.y + 20); break;
		}
	}
}

void mission_start::place_book(game *g, mission *miss)	// XXX doesn't look like it works \todo fix
{
}

void mission_end::heal_infection(game *g, mission *miss)
{
	bool found_npc = false;
	for (int i = 0; i < g->active_npc.size() && !found_npc; i++) {
		if (g->active_npc[i].id == miss->npc_id) {
			g->active_npc[i].rem_disease(DI_INFECTION);
			found_npc = true;
		}
	}

	for (int i = 0; !found_npc && i < g->cur_om.npcs.size(); i++) {
		if (g->cur_om.npcs[i].id == miss->npc_id) {
			g->cur_om.npcs[i].rem_disease(DI_INFECTION);
			found_npc = true;
		}
	}
}

void mission_fail::kill_npc(game *g, mission *miss)
{
	for (int i = 0; i < g->active_npc.size(); i++) {
		if (g->active_npc[i].id == miss->npc_id) {
			g->active_npc[i].die(g, false);
			g->active_npc.erase(g->active_npc.begin() + i);
			return;
		}
	}
	for (int i = 0; i < g->cur_om.npcs.size(); i++) {
		if (g->cur_om.npcs[i].id == miss->npc_id) {
			g->cur_om.npcs[i].marked_for_death = true;
			return;
		}
	}
}

void mission_type::init()
{
 #define MISSION(name, goal, diff, val, urgent, place, start, end, fail) \
 id++; types.push_back( \
mission_type(id, name, goal, diff, val, urgent, place, start, end, fail) )

 #define ORIGINS(...) SET_VECTOR(types[id].origins, __VA_ARGS__)
 #define ITEM(itid)     types[id].item_id = itid

// DEADLINE defines the low and high end time limits, in hours
// Omitting DEADLINE means the mission never times out
 #define DEADLINE(low, high) types[id].deadline_low  = low  * 600;\
                             types[id].deadline_high = high * 600
 //#define NPCS   (...) setvector(missions[id].npc


// The order of missions should match enum mission_id in mission.h
 int id = -1;

 MISSION("Null mission", MGOAL_NULL, 0, 0, false,
         &mission_place::never, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);

 MISSION("Find Antibiotics", MGOAL_FIND_ITEM, 2, 1500, true,
	&mission_place::always, &mission_start::infect_npc,
	&mission_end::heal_infection, &mission_fail::kill_npc);
  ORIGINS(ORIGIN_OPENER_NPC);
  ITEM(itm_antibiotics);
  DEADLINE(24, 48); // 1 - 2 days

 MISSION("Retrieve Software", MGOAL_FIND_ANY_ITEM, 2, 800, false,
	&mission_place::near_town, &mission_start::place_npc_software,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION("Analyze Zombie Blood", MGOAL_FIND_ITEM, 8, 2500, false,
	&mission_place::always, &mission_start::reveal_hospital,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM(itm_software_blood_data);

 MISSION("Find Lost Dog", MGOAL_FIND_MONSTER, 3, 1000, false,
	&mission_place::near_town, &mission_start::place_dog,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC);

 MISSION("Kill Zombie Mom", MGOAL_KILL_MONSTER, 5, 1200, true,
	&mission_place::near_town, &mission_start::place_zombie_mom,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION("Reach Safety", MGOAL_GO_TO, 1, 0, false,
	&mission_place::always, &mission_start::find_safety,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_NULL);

 MISSION("Find a Book", MGOAL_FIND_ANY_ITEM, 2, 800, false,
	&mission_place::always, &mission_start::place_book,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_ANY_NPC);
}
