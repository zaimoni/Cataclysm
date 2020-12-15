#include "mission.h"
#include "game.h"
#include "rng.h"
#include "recent_msg.h"
#include "line.h"
#include <sstream>

std::vector <mission_type> mission_type::types; // The list of mission templates

struct mission_place {	// Return true if [posx,posy] is valid in overmap
	static bool never(game *g, int posx, int posy, int npc_id) { return false; }
	static bool always(game *g, int posx, int posy, int npc_id) { return true; }
	static bool near_town(game *g, int posx, int posy, int npc_id);
	static bool no_antibiotics(game* g, int posx, int posy, int npc_id);
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

bool mission_place::near_town(game *g, int posx, int posy, int npc_id)
{
	return (g->cur_om.dist_from_city(point(posx, posy)) <= 40);
}

bool mission_place::no_antibiotics(game* g, int posx, int posy, int npc_id)
{
	const auto p = npc::find_alive(npc_id);
	if (!p) return false;
	for (size_t i = 0; i < p->inv.size(); i++) {
		if (p->inv[i].type->id == itm_antibiotics) return false;
	}
	return true;
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
	const auto p = npc::find_alive(miss->npc_id);
	if (!p) throw std::string(__FUNCTION__)+" couldn't find an NPC!";
	if (!mission_place::no_antibiotics(g, 0, 0, miss->npc_id)) throw std::string(__FUNCTION__) + " NPC trivializes mission.";
	p->add_disease(DI_INFECTION, -1);
}

void mission_start::place_dog(game *g, mission *miss)
{
	const auto city_id = g->cur_om.closest_city(g->om_location().second);
	if (!city_id.second) throw std::string(__FUNCTION__) + " couldn't find mission city";
	const auto dev = npc::find_alive_r(miss->npc_id);
	if (!dev) throw std::string(__FUNCTION__) + " couldn't find an NPC!";
	if (!city_id.first->random_house_in_city(city_id.second, miss->target)) throw std::string(__FUNCTION__) + " couldn't find house within mission city";
	g->u.i_add(item(item::types[itm_dog_whistle], 0));
	messages.add("%s gave you a dog whistle.", dev->name.c_str());

	// Make it seen on our map
	OM_loc scan(g->cur_om.pos, point(0, 0));
	for (scan.second.x = miss->target.second.x - 6; scan.second.x <= miss->target.second.x + 6; scan.second.x++) {
		for (scan.second.y = miss->target.second.y - 6; scan.second.y <= miss->target.second.y + 6; scan.second.y++) overmap::expose(scan);
	}

	tinymap doghouse;
	doghouse.load(miss->target);
	doghouse.add_spawn(mon_dog, 1, SEEX, SEEY, true, -1, miss->uid);
	doghouse.save(miss->target, int(messages.turn));
}

void mission_start::place_zombie_mom(game *g, mission *miss)
{
	const auto city_id = g->cur_om.closest_city(g->om_location().second);
	if (!city_id.second) throw std::string(__FUNCTION__) + " couldn't find mission city";
	if (!city_id.first->random_house_in_city(city_id.second, miss->target)) throw std::string(__FUNCTION__) + " couldn't find house within mission city";

	// Make it seen on our map
	OM_loc scan(g->cur_om.pos, point(0, 0));
	for (scan.second.x = miss->target.second.x - 6; scan.second.x <= miss->target.second.x + 6; scan.second.x++) {
		for (scan.second.y = miss->target.second.y - 6; scan.second.y <= miss->target.second.y + 6; scan.second.y++) overmap::expose(scan);
	}

	tinymap zomhouse;
	zomhouse.load(miss->target);
	zomhouse.add_spawn(mon_zombie, 1, SEEX, SEEY, false, -1, miss->uid, random_first_name(false));
	zomhouse.save(miss->target, int(messages.turn));
}

void mission_start::place_npc_software(game *g, mission *miss)
{
	const auto dev = npc::find_alive_r(miss->npc_id);
	if (!dev) throw std::string(__FUNCTION__) + " couldn't find an NPC!";

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

	OM_loc place;
	if (ter == ot_house_north) {
		const auto city_id = g->cur_om.closest_city(g->om_location().second);
		if (!city_id.second) throw std::string(__FUNCTION__) + " couldn't find mission city";
		if (!city_id.first->random_house_in_city(city_id.second, place)) throw std::string(__FUNCTION__) + " couldn't find house within mission city";
	}
	else if (!g->cur_om.find_closest(g->om_location().second, ter, 4, place)) throw std::string(__FUNCTION__) + " couldn't find mission location";
	g->u.i_add(item(item::types[itm_usb_drive], 0));
	messages.add("%s gave you a USB drive.", dev->name.c_str());

	miss->target = place;
	// Make it seen on our map
	OM_loc scan(g->cur_om.pos, point(0, 0));
	for (scan.second.x = place.second.x - 6; scan.second.x <= place.second.x + 6; scan.second.x++) {
		for (scan.second.y = place.second.y - 6; scan.second.y <= place.second.y + 6; scan.second.y++) overmap::expose(scan);
	}
	tinymap compmap;
	compmap.load(place);
	point comppoint;

	switch (overmap::ter(place)) {
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
							const auto t = compmap.ter(x2, y2);
							if (t_bed == t || t_dresser == t) {
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

	std::ostringstream compname;
	compname << dev->name << "'s Terminal";
	compmap.ter(comppoint) = t_console;
	computer* const tmpcomp = compmap.add_computer(comppoint.x, comppoint.y, compname.str(), 0);
	tmpcomp->mission_id = miss->uid;
	tmpcomp->add_option("Download Software", COMPACT_DOWNLOAD_SOFTWARE, 0);

	compmap.save(place, int(messages.turn));
}

void mission_start::reveal_hospital(game *g, mission *miss)
{
	const auto dev = npc::find_alive_r(miss->npc_id);
	if (!dev) throw std::string(__FUNCTION__) + " couldn't find an NPC!";
	OM_loc dest;
	if (!g->cur_om.find_closest(g->om_location().second, ot_hospital, 1, dest)) throw std::string(__FUNCTION__) + " couldn't find a hospital!";

	g->u.i_add(item(item::types[itm_vacutainer], 0));
	messages.add("%s gave you a vacutainer.", dev->name.c_str());

	OM_loc scan(g->cur_om.pos, point(0, 0));
	for (scan.second.x = dest.second.x - 3; scan.second.x <= dest.second.x + 3; scan.second.x++) {
		for (scan.second.y = dest.second.y - 3; scan.second.y <= dest.second.y + 3; scan.second.y++) overmap::expose(scan);
	}
	miss->target = dest;
}

void mission_start::find_safety(game *g, mission *miss)
{
	static constexpr const point scan_dirs[] = {Direction::NW, Direction::NE, Direction::SE, Direction::SW };
	auto place = g->om_location();
	for (int radius = 0; radius <= 20; radius++) {
		for (int dist = 0 - radius; dist <= radius; dist++) {
			int offset = rng(0, 3); // Randomizes the direction we check first
			for (int i = 0; i <= 3; i++) { // Which direction?
				auto check = place;
				switch ((offset + i) % 4) {
				case 0: check.second.x += dist; check.second.y -= radius; break;
				case 1: check.second.x += dist; check.second.y += radius; break;
				case 2: check.second.y += dist; check.second.x -= radius; break;
				case 3: check.second.y += dist; check.second.x += radius; break;
				}
				if (overmap::is_safe(check)) {
					miss->target = check;
					return;
				}
			}
		}
	}
	// Couldn't find safety; so just set the target to far away
	miss->target = OM_loc(g->cur_om.pos, place.second + 20*scan_dirs[rng(0, 3)]);
}

void mission_start::place_book(game *g, mission *miss)	// XXX doesn't look like it works \todo fix
{
}

void mission_end::heal_infection(game *g, mission *miss)
{
	if (auto _npc = npc::find_alive(miss->npc_id)) _npc->rem_disease(DI_INFECTION);
}

void mission_fail::kill_npc(game *g, mission *miss) { npc::die(miss->npc_id); }

void mission_type::init()
{
// The order of missions should match enum mission_id in mission.h
  int id = -1;
  types.push_back(mission_type(++id, "Null mission", MGOAL_NULL, 0, 0, false,
	  &mission_place::never, &mission_start::standard,
	  &mission_end::standard, &mission_fail::standard));
  static_assert(0 == MISSION_NULL);

  types.push_back(mission_type(++id, "Find Antibiotics", MGOAL_FIND_ITEM, 2, 1500, true,
	  &mission_place::no_antibiotics, &mission_start::infect_npc,
	  &mission_end::heal_infection, &mission_fail::kill_npc, { ORIGIN_OPENER_NPC }, itm_antibiotics,
	  HOURS(24), HOURS(48))); // 1-2 days
  static_assert(MISSION_GET_ANTIBIOTICS == MISSION_NULL + 1);

  types.push_back(mission_type(++id, "Retrieve Software", MGOAL_FIND_ANY_ITEM, 2, 800, false,
	  &mission_place::near_town, &mission_start::place_npc_software,
	  &mission_end::standard, &mission_fail::standard, { ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC }));
  static_assert(MISSION_GET_SOFTWARE == MISSION_GET_ANTIBIOTICS + 1);

  types.push_back(mission_type(++id, "Analyze Zombie Blood", MGOAL_FIND_ITEM, 8, 2500, false,
	  &mission_place::always, &mission_start::reveal_hospital,
	  &mission_end::standard, &mission_fail::standard, { ORIGIN_SECONDARY }, itm_software_blood_data));
  static_assert(MISSION_GET_ZOMBIE_BLOOD_ANAL == MISSION_GET_SOFTWARE + 1);

  types.push_back(mission_type(++id, "Find Lost Dog", MGOAL_FIND_MONSTER, 3, 1000, false,
	  &mission_place::near_town, &mission_start::place_dog,
	  &mission_end::standard, &mission_fail::standard, { ORIGIN_OPENER_NPC }));
  static_assert(MISSION_RESCUE_DOG == MISSION_GET_ZOMBIE_BLOOD_ANAL + 1);

  types.push_back(mission_type(++id, "Kill Zombie Mom", MGOAL_KILL_MONSTER, 5, 1200, true,
	  &mission_place::near_town, &mission_start::place_zombie_mom,
	  &mission_end::standard, &mission_fail::standard, { ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC }));
  static_assert(MISSION_KILL_ZOMBIE_MOM == MISSION_RESCUE_DOG + 1);

  types.push_back(mission_type(++id, "Reach Safety", MGOAL_GO_TO, 1, 0, false,
	  &mission_place::always, &mission_start::find_safety,
	  &mission_end::standard, &mission_fail::standard, { ORIGIN_NULL }));
  static_assert(MISSION_REACH_SAFETY == MISSION_KILL_ZOMBIE_MOM + 1);

  // **** NOTE: disabled until finished ****
/*  types.push_back(mission_type(++id, "Find a Book", MGOAL_FIND_ANY_ITEM, 2, 800, false,
	  &mission_place::always, &mission_start::place_book,
	  &mission_end::standard, &mission_fail::standard), { ORIGIN_ANY_NPC }); */
  static_assert(MISSION_GET_BOOK == MISSION_REACH_SAFETY + 1);

  assert(NUM_MISSION_IDS >= types.size());	// postcondition check
}
