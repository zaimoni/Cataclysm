#ifndef _VEHICLE_H_
#define _VEHICLE_H_

#include "tileray.h"
#include "veh_type.h"
#include <vector>
#include <string>
#include <iosfwd>

class map;
class player;
class game;

const int k_mvel = 200;

// Structure, describing vehicle part (ie, wheel, seat)
struct vehicle_part
{
	std::vector<item> items;// inventory
	vpart_id id;            // id in list of parts (vpart_list index)
	point mount_d;			// mount point (coordinates flipped like curses, vertical x horizontal y)
    point precalc_d[2];     // mount_dx/mount_dy translated to face.dir [0] and turn_dir [1]
    int hp;                 // current durability, if 0, then broken
    int blood;              // how much blood covers part (in turns). only useful for external
    bool inside;            // if tile provides cover. WARNING: do not read it directly, use vehicle::is_inside() instead
    union
    {
        int amount;         // amount of fuel for tank
        int open;           // door is open
        int passenger;      // seat has passenger
    };

	vehicle_part() = default;
	vehicle_part(vpart_id _id, int _mdx, int _mdy, int _hp, int _blood=0, int _amount=0)
	: id(_id),mount_d(_mdx,_mdy),hp(_hp),blood(_blood),amount(_amount) {};
	vehicle_part(const vehicle_part& src) = default;
	vehicle_part(vehicle_part&& src) = default;
	~vehicle_part() = default;
	vehicle_part& operator=(const vehicle_part& src) = default;
	vehicle_part& operator=(vehicle_part&& src) = default;
};

// Facts you need to know about implementation:
// - Vehicles belong to map. There's std::vector<vehicle>
//   for each submap in grid. When requesting a reference
//   to vehicle, keep in mind it can be invalidated
//   by functions such as map::displace_vehicle.
// - To check if there's any vehicle at given map tile,
//   call map::veh_at, and check vehicle type (veh_null
//   means there's no vehicle there).
// - Vehicle consists of parts (represented by vector). Parts
//   have some constant info: see veh_type.h, vpart_info structure
//   and vpart_list array -- that is accessible through part_info method.
//   The second part is variable info, see vehicle_part structure
//   just above.
// - Parts are mounted at some point relative to vehicle position (or starting part)
//   (0, 0 in mount coords). There can be more than one part at
//   given mount coords. First one is considered external,
//   others are internal (or, as special case, "over" -- like trunk)
//   Check tileray.h file to see a picture of coordinate axes.
// - Vehicle can be rotated to arbitrary degree. This means that
//   mount coords are rotated to match vehicle's face direction before
//   their actual positions are known. For optimization purposes
//   mount coords are precalculated for current vehicle face direction
//   and stored in precalc_*[0]. precalc_*[1] stores mount coords for
//   next move (vehicle can move and turn). Method map::displace vehicle
//   assigns precalc[1] to precalc[0]. At any time (except
//   map::vehmove innermost cycle) you can get actual part coords
//   relative to vehicle's position by reading precalc_*[0].
// - Vehicle keeps track of 3 directions:
//     face (where it's facing currently)
//     move (where it's moving, it's different from face if it's skidding)
//     turn_dir (where it will turn at next move, if it won't stop due to collision)
// - Some methods take "part" or "p" parameter. Some of them
//   assume that's external part number, and all internal parts
//   at this mount point are affected. There is separate
//   vector in which a list of external part is stored,
//   it must correspond to actual list of external parts
//   (assure this if you add/remove parts programmatically).
// - Driver doesn't know what vehicle he drives.
//   There's only player::in_vehicle flag which
//   indicates that he is inside vehicle. To figure
//   out what, you need to ask a map if there's a vehicle
//   at driver/passenger position.
// - To keep info consistent, always use
//   map::board_vehicle and map::unboard_vehicle for
//   boarding/unboarding player.
// - To add new predesigned vehicle, assign new value for vhtype_id enum
//   and declare vehicle and add parts in file veh_typedef.cpp, using macroes,
//   similar to existant. Keep in mind, that positive x coordinate points
//   forwards, negative x is back, positive y is to the right, and
//   negative y to the left:
//       orthogonal dir left (-Y)
//        ^
//  -X ------->  +X (forward)
//        v
//       orthogonal dir right (+Y)
//   When adding parts, function checks possibility to install part at given
//   coords. If it shows debug messages that it can't add parts, when you start
//   the game, you did something wrong.
//   There are a few rules: some parts are external, so one should be the first part
//   at given mount point (tile). They require some part in neighbouring tile (with vpf_mount_point flag) to
//   be mounted to. Other parts are internal or placed over. They can only be installed on top
//   of external part. Some functional parts can be only in single instance per tile, i. e.,
//   no two engines at one mount point.
//   If you can't understand, why installation fails, try to assemble your vehicle in game first.
class vehicle
{
public:
	static std::vector <vehicle*> vtypes;

    vehicle(vhtype_id type_id = veh_null);
	vehicle(const vehicle& src) = default;
	vehicle(vehicle&& src) = default;
	~vehicle() = default;
	vehicle& operator=(const vehicle& src);
	vehicle& operator=(vehicle&& src) = default;

// check if given player controls this vehicle
    bool player_in_control(player& p) const;

// get vpart type info for part number (part at given vector index)
    const vpart_info& part_info(int index) const;

// check if certain part can be mounted at certain position (not accounting frame direction)
    bool can_mount(int dx, int dy, vpart_id id) const;
	bool can_mount(point d, vpart_id id) const { return can_mount(d.x, d.y, id); };

// check if certain external part can be unmounted
    bool can_unmount (int p) const;

// install a new part to vehicle (force to skip possibility check)
    int install_part (int dx, int dy, vpart_id id, int hp = -1, bool force = false);
    
    void remove_part (int p);

// returns the list of indeces of parts at certain position (not accounting frame direction)
    std::vector<int> parts_at_relative (int dx, int dy) const;

// returns the list of indeces of parts inside (or over) given
    std::vector<int> internal_parts(int p) const;

// returns index of part, inner to given, with certain flag (WARNING: without mfb!), or -1
    int part_with_feature(int p, unsigned int f, bool unbroken = true) const;

// returns true if given flag is present for given part index (WARNING: without mfb!)
    bool part_flag(int p, unsigned int f) const;

// Translate seat-relative mount coords into tile coords
    point coord_translate (point reld) const;

// Translate seat-relative mount coords into tile coords using given face direction
    static point coord_translate (int dir, point reld);
    
// Seek a vehicle part which obstructs tile with given coords relative to vehicle position
    int part_at(int dx, int dy) const;

// get symbol for map
    char part_sym (int p) const;

// get color for map
    nc_color part_color (int p) const;

// Vehicle parts description
    void print_part_desc (void *w, int y1, int width, int p, int hl = -1);

// Vehicle fuel indicator
    void print_fuel_indicator (void *w, int y, int x);

// Precalculate mount points for (idir=0) - current direction or (idir=1) - next turn direction
    void precalc_mounts (int idir, int dir);

// get a list of part indeces where is a passenger inside
    std::vector<int> boarded_parts() const;

// get passenger at part p
    player *get_passenger(int p) const;

// get global coords for vehicle
	point global() const;

// Checks how much certain fuel left in tanks. If for_engine == true that means
// ftype == AT_BATT is also takes in account AT_PLUT fuel (electric motors can use both)
    int fuel_left(int ftype, bool for_engine = false) const;
    int fuel_capacity(int ftype) const;

    // refill fuel tank(s) with given type of fuel
    // returns amount of leftover fuel
    int refill (int ftype, int amount);

	bool refill(player& u, int part, bool test=false);

// vehicle's fuel type name
    std::string fuel_name(int ftype);

// fuel consumption of vehicle engines of given type, in one-hundreth of fuel
    int basic_consumption(int ftype) const;

    void consume_fuel ();

// get the total mass of vehicle, including cargo and passengers
    int total_mass() const;

// Get combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int total_power(bool fueled = true) const;

// Get combined power of solar panels
    int solar_power () const;
    
// Get acceleration gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int acceleration(bool fueled = true) const;

// Get maximum velocity gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int max_velocity(bool fueled = true) const;

// Get safe velocity gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int safe_velocity(bool fueled = true) const;

    int noise(bool fueled = true, bool gas_only = false) const;

// Calculate area covered by wheels and, optionally count number of wheels
    int wheels_area(int *cnt = 0) const;

// Combined coefficient of aerodynamic and wheel friction resistance of vehicle, 0-1.0.
// 1.0 means it's ideal form and have no resistance at all. 0 -- it won't move
    float k_dynamics() const;

// Coefficient of mass, 0-1.0.
// 1.0 means mass won't slow vehicle at all, 0 - it won't move
    float k_mass() const;

// strain of engine(s) if it works higher that safe speed (0-1.0)
    float strain() const;

// calculate if it can move using its wheels configuration
    bool valid_wheel_config() const;

// thrust (1) or brake (-1) vehicle
    void thrust (int thd);

// cruise control
    void cruise_thrust (int amount);

// turn vehicle left (negative) or right (positive), degrees
    void turn (int deg);

// handle given part collision with vehicle, monster/NPC/player or terrain obstacle
// return impulse (damage) applied on vehicle for that collision
    int part_collision (int vx, int vy, int part, point dest);

// Process the trap beneath
    void handle_trap (int x, int y, int part);

// add item to part's cargo. if false, then there's no cargo at this part or cargo is full
    bool add_item (int part, item itm);

// remove item from part's cargo
    void remove_item (int part, int itemdex);

    void gain_moves (int mp);

// reduces velocity to 0
    void stop ();

    void find_external_parts ();

    void find_exhaust ();

    void refresh_insides ();

    bool is_inside (int p);

    void unboard_all ();

    // damage types:
    // 0 - piercing
    // 1 - bashing (damage applied if it passes certain treshold)
    // 2 - incendiary
    // damage individual external part. bash means damage
    // must exceed certain threshold to be substracted from hp
    // (a lot light collisions will not destroy parts)
    // returns damage bypassed
    int damage (int p, int dmg, int type = 1, bool aimed = true);

    // damage all parts (like shake from strong collision), range from dmg1 to dmg2
    void damage_all (int dmg1, int dmg2, int type = 1);

    // direct damage to part (armor protection and internals are not counted)
    // returns damage bypassed
    int damage_direct (int p, int dmg, int type = 1);

    void leak_fuel (int p);

    // fire the turret which is part p
    void fire_turret (int p, bool burst = true);

    // internal procedure of turret firing
    bool fire_turret_internal (int p, it_gun &gun, const it_ammo &ammo, int charges);

#if DEAD_FUNC
    // upgrades/refilling/etc. see veh_interact.cpp
    void interact ();
#endif

    // config values
    std::string name;   // vehicle name
	vhtype_id _type;           // vehicle type
    std::vector<vehicle_part> parts;   // Parts which occupy different tiles
    std::vector<int> external_parts;   // List of external parts indeces
	point exhaust_d;

    // temp values
    point sm;   // submap coords. WARNING: must ALWAYS correspond to submap coords in grid, or i'm out [0..10,0..10]
    bool insides_dirty; // if true, then parts' "inside" flags are outdated and need refreshing

    // save values
    point pos;		// position within submap: valid values 0...SEEX/SEEY-1
    tileray face;       // frame direction
    tileray move;       // direction we are moving
    int velocity;       // vehicle current velocity, mph * 100
    int cruise_velocity; // velocity vehicle's cruise control trying to acheive
    bool cruise_on;     // cruise control on/off
    int turn_dir;       // direction, to wich vehicle is turning (player control). will rotate frame on next move
    bool skidding;      // skidding mode
    int last_turn;      // amount of last turning (for calculate skidding due to handbrake)
    int moves;
    int turret_mode;    // turret firing mode: 0 = off, 1 = burst fire	; leave as int in case we want true autofire
private:
	// init parts state for randomly generated vehicle
	void init_state();
};

#endif
