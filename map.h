#ifndef _MAP_H_
#define _MAP_H_

#include "mapitems.h"
#include "overmap.h"
#include "submap.h"

#include <functional>
#include <string>

#define MAPSIZE 11

class player;
class monster;
class vehicle;
class overmap;

// We do not want to use the Curiously Recurring Template Pattern to deal with the submap grid
class map
{
 public:
 static std::vector <itype_id> items[num_itloc]; // Items at various map types
 static map_extra _force_map_extra; // debugging assistant
 static point _force_map_extra_pos; // debugging assistant
 typedef std::pair<short,point> localPos; // nonant, x, y

// Constructors & Initialization
 map(int map_size = MAPSIZE) : my_MAPSIZE(map_size), grid(map_size*map_size, nullptr) {}
 map(const map& src) = default;
 map(map&& src) = default;
 virtual ~map() = default;	// works because mapbuffer is the owner of the submaps, not us (copy works by same rationale)
 map& operator=(const map& src) = default;
 map& operator=(map&& src) = default;

// Visual Output
 void draw(game *g, WINDOW* w, point center);
 void debug();
 void drawsq(WINDOW* w, const player& u, int x, int y, bool invert, bool show_items,
             int cx = -1, int cy = -1) const;

// File I/O
 void save(const tripoint& om_pos, unsigned int turn, const point& world);
 void save(const OM_loc& GPS, unsigned int turn) { save(GPS.first, turn, GPS.second); };
 void load(game *g, const point& world);
 void load(const tripoint& GPS);
 void load(const OM_loc& GPS);
 void shift(game *g, const point& world, const point& delta);
 void spawn_monsters(game *g);
 void clear_spawns();
 void clear_traps();

// Movement and LOS
 bool to(int x, int y, localPos& dest) const;
 bool to(const point& pt, localPos& dest) const { return to(pt.x, pt.y, dest); };

 bool find_stairs(const point& pt, int movez, point& pos) const;
 bool find_terrain(const point& pt, ter_id dest, point& pos) const;

 int move_cost(int x, int y) const; // Cost to move through; 0 = impassible
 int move_cost(const point& pt) const { return move_cost(pt.x, pt.y); };
 int move_cost(const localPos& pos) const;
 int move_cost_ter_only(int x, int y) const; // same as above, but don't take vehicles into account
 bool trans(int x, int y) const; // Transparent?
 bool trans(const localPos& pos) const;
 bool _BresenhamLine(int Fx, int Fy, int Tx, int Ty, int range, int& tc, std::function<bool(localPos)> test) const;
 // (Fx, Fy) sees (Tx, Ty), within a range of (range)?
 // tc indicates the Bresenham line used to connect the two points, and may
 //  subsequently be used to form a path between them
 bool sees(int Fx, int Fy, int Tx, int Ty, int range) const;
 bool sees(const point& F, int Tx, int Ty, int range) const { return sees(F.x, F.y, Tx, Ty, range); };
 bool sees(const point& F, const point& T, int range) const { return sees(F.x, F.y, T.x, T.y, range); };
 bool sees(int Fx, int Fy, const point& T, int range) const { return sees(Fx, Fy, T.x, T.y, range); };
 bool sees(int Fx, int Fy, int Tx, int Ty, int range, int &tc) const;
 bool sees(const point& F, int Tx, int Ty, int range, int &tc) const { return sees(F.x, F.y, Tx, Ty, range, tc); };
 bool sees(const point& F, const point& T, int range, int &tc) const { return sees(F.x, F.y, T.x, T.y, range, tc); };
 bool sees(int Fx, int Fy, const point& T, int range, int &tc) const { return sees(Fx, Fy, T.x, T.y, range, tc); };
 // clear_path is the same idea, but uses cost_min <= move_cost <= cost_max
 bool clear_path(int Fx, int Fy, int Tx, int Ty, int range, int cost_min,
                 int cost_max, int &tc) const;
// route() generates an A* best path; if bash is true, we can bash through doors
 std::vector<point> route(int Fx, int Fy, int Tx, int Ty, bool bash = true) const;
 std::vector<point> route(const point& F, int Tx, int Ty, bool bash = true) const { return route(F.x, F.y, Tx, Ty, bash); };
 std::vector<point> route(const point& F, const point& T, bool bash = true) const { return route(F.x, F.y, T.x, T.y, bash); };
 std::vector<point> route(int Fx, int Fy, const point& T, bool bash = true) const { return route(Fx, Fy, T.x, T.y, bash); };

// vehicles
// checks, if tile is occupied by vehicle and by which part
 vehicle* veh_at(int x, int y, int &part_num) const;
 vehicle* veh_at(const point& pt, int &part_num) const { return veh_at(pt.x, pt.y, part_num); };
 vehicle* veh_at(const localPos& src, int& part_num) const;
 vehicle* veh_at(int x, int y) const;
 vehicle* veh_at(const point& pt) const { return veh_at(pt.x, pt.y); };
 // put player on vehicle at x,y
 void board_vehicle(game *g, int x, int y, player *p);
 void unboard_vehicle(const point& pt);//remove player from vehicle at x,y

 void destroy_vehicle (vehicle *veh);
// Change vehicle coords and move vehicle's driver along.
// Returns true, if there was a submap change.
// If test is true, function only checks for submap change, no displacement
// WARNING: not checking collisions!
 bool displace_vehicle(game *g, int &x, int &y, const point& delta, bool test=false);
 void vehmove(game* g);          // Vehicle movement
// move water under wheels. true if moved
 bool displace_water (int x, int y);

// Terrain
 ter_id& ter(int x, int y); // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 ter_id& ter(const point& pt) { return ter(pt.x, pt.y); };
 ter_id& ter(const localPos& src) { return grid[src.first]->ter[src.second.x][src.second.y]; };
 ter_id ter(const localPos& src) const { return grid[src.first]->ter[src.second.x][src.second.y]; };
 ter_id ter(int x, int y) const { return const_cast<map*>(this)->ter(x, y); };	// \todo specialize this properly
 ter_id ter(const point& pt) const { return const_cast<map*>(this)->ter(pt.x, pt.y); };

 template<ter_id src, ter_id dest> void rewrite(int x, int y) {
	 static_assert(src!=dest);
	 auto& t = ter(x,y);
	 if (src == t) t = dest;
 }

 template<ter_id src, ter_id dest> void rewrite_inv(int x, int y) {
	 static_assert(src != dest);
	 auto& t = ter(x, y);
	 if (src != t) t = dest;
 }

 template<ter_id src> void rewrite(int x, int y, ter_id dest) {
	 auto& t = ter(x, y);
	 if (src == t) t = dest;
 }

 template<ter_id src, ter_id src2, ter_id src3, ter_id dest> void rewrite(int x, int y) {
	 static_assert(src != dest);
	 static_assert(src2 != dest);
	 static_assert(src3 != dest);
	 static_assert(src != src2);
	 static_assert(src != src3);
	 static_assert(src2 != src3);
	 auto& t = ter(x, y);
	 if (src == t || src2 == t || src3 == t) t = dest;
 }

 template<ter_id src, ter_id dest> bool rewrite_test(int x, int y) {
	 static_assert(src != dest);
	 auto& t = ter(x, y);
	 bool ret = (src == t);
	 if (ret) t = dest;
	 return ret;
 }

 template<ter_id src, ter_id dest> bool rewrite_test(const point& pt) {
	 static_assert(src != dest);
	 auto& t = ter(pt);
	 bool ret = (src == t);
	 if (ret) t = dest;
	 return ret;
 }

 template<ter_id src, ter_id src2, ter_id dest> bool rewrite_test(int x, int y) {
	 static_assert(src != dest);
	 static_assert(src2 != dest);
	 static_assert(src != src2);
	 auto& t = ter(x,y);
	 bool ret = (src == t || src2 == t);
	 if (ret) t = dest;
	 return ret;
 }

 const std::string& tername(int x, int y) const; // Name of terrain at (x, y)
 const std::string& tername(const point& pt) const { return tername(pt.x, pt.y); };

 std::string features(const point& pt) const; // Words relevant to terrain (sharp, etc)
 bool has_flag(t_flag flag, int x, int y) const;  // checks terrain and vehicles
 bool has_flag(t_flag flag, const point& pt) const { return has_flag(flag, pt.x, pt.y); };
 bool has_flag(t_flag flag, const localPos& pos) const;
 bool has_flag_ter_only(t_flag flag, int x, int y) const; // only checks terrain
 bool is_destructable(int x, int y) const;        // checks terrain and vehicles
 bool is_destructable_ter_only(int x, int y) const;       // only checks terrain
 bool is_outside(int x, int y) const;
 bool is_outside(const point& pt) const { return is_outside(pt.x, pt.y); };
 bool flammable_items_at(int x, int y) const;
 point random_outdoor_tile();

 std::vector<point> grep(const point& tl, const point& br, std::function<bool(point)> test);

 template<ter_id src, ter_id dest> void translate() { // Change all instances of $src->$dest
	 static_assert(src != dest);
	 _translate(src,dest);
 }
 
 bool close_door(const point& pt);
 bool open_door(int x, int y, bool inside);
 // bash: if res pointer is supplied, res will contain absorbed impact or -1
 bool bash(int x, int y, int str, std::string& sound, int* res = nullptr);
 bool bash(const point& pt, int str, std::string& sound, int* res = nullptr) { return bash(pt.x,pt.y,str,sound,res); }
 bool bash(int x, int y, int str, int* res = nullptr);
 bool bash(const point& pt, int str, int* res = nullptr) { return bash(pt.x, pt.y, str, res ); }
 void destroy(game *g, int x, int y, bool makesound);
 void shoot(game *g, int x, int y, int &dam, bool hit_items, unsigned flags);
 bool hit_with_acid(game *g, int x, int y);
 void marlossify(int x, int y);

// Radiation
 int& radiation(int x, int y);	// Amount of radiation at (x, y);

// Items
 std::vector<item>& i_at(int x, int y);
 std::vector<item>& i_at(const point& pt) { return i_at(pt.x, pt.y); };
 std::vector<item>& i_at(const localPos& pos) { return grid[pos.first]->itm[pos.second.x][pos.second.y]; };
 const std::vector<item>& i_at(const localPos& pos) const { return grid[pos.first]->itm[pos.second.x][pos.second.y]; };
 const std::vector<item>& i_at(int x, int y) const { return const_cast<map*>(this)->i_at(x, y); };
 const std::vector<item>& i_at(const point& pt) const { return const_cast<map*>(this)->i_at(pt.x, pt.y); };
 item water_from(int x, int y) const;
 void i_clear(int x, int y);
 void i_clear(const point& pt) { return i_clear(pt.x, pt.y); };
 void i_rem(int x, int y, int index);
 void i_rem(const point& pt, int index) { i_rem(pt.x, pt.y, index); };
 point find_item(item *it) const;
 void add_item(int x, int y, const itype* type, int birthday);
 void add_item(const point& pt, const itype* type, int birthday) { add_item(pt.x, pt.y, type, birthday); };
 void add_item(int x, int y, const item& new_item);
 void add_item(const point& pt, const item& new_item) { return add_item(pt.x, pt.y, new_item); };
 void process_active_items(game *g);
// void process_vehicles(game *g);	// undefined function

 void use_amount(point origin, int range, itype_id type, int quantity,
                 bool use_container = false);
 void use_charges(point origin, int range, itype_id type, int quantity);

// Traps
 trap_id& tr_at(int x, int y);
 trap_id& tr_at(const point& pt) { return tr_at(pt.x, pt.y); };
 trap_id& tr_at(const localPos& src) { return grid[src.first]->trp[src.second.x][src.second.y]; };
 trap_id tr_at(const localPos& src) const { return grid[src.first]->trp[src.second.x][src.second.y]; };
 trap_id tr_at(int x, int y) const { return const_cast<map*>(this)->tr_at(x, y); };	// \todo specialize this properly
 trap_id tr_at(const point& pt) const { return const_cast<map*>(this)->tr_at(pt.x, pt.y); };
 void add_trap(int x, int y, trap_id t);
 void disarm_trap( game *g, const point& pt);

// Fields
 field& field_at(int x, int y);
 field& field_at(const point& pt) { return field_at(pt.x, pt.y); };
 field& field_at(const localPos& src) { return grid[src.first]->fld[src.second.x][src.second.y]; };
 const field& field_at(const localPos& src) const { return grid[src.first]->fld[src.second.x][src.second.y]; };
 const field& field_at(int x, int y) const { return const_cast<map*>(this)->field_at(x, y); };
 const field& field_at(const point& pt) const { return const_cast<map*>(this)->field_at(pt.x, pt.y); };
 bool add_field(game *g, int x, int y, field_id t, unsigned char density, unsigned int age=0);
 bool add_field(game *g, const point& pt, field_id t, unsigned char density, unsigned int age = 0) { return add_field(g, pt.x, pt.y, t, density, age); };
 void remove_field(int x, int y);
 void remove_field(const point& pt) { remove_field(pt.x, pt.y); };
 bool process_fields(game *g);				// See field.cpp
 void step_in_field(const point& pt, game *g);		// See field.cpp	// V 0.2.1 break hard-coding to player g->u
 void mon_in_field(const point& pt, game *g, monster *z);	// See field.cpp	\todo all callers use the monster's position: decide whether to hardcode this

// Computers
 computer* computer_at(const point& pt);

// mapgen.cpp functions
 void apply_temple_switch(ter_id trigger, int y0, int x, int y);

 void generate(game *g, overmap *om, int x, int y);
 void post_process(game *g, unsigned zones);
 void place_items(items_location loc, int chance, int x1, int y1,
                  int x2, int y2, bool ongrass, int turn);
// put_items_from puts exactly num items, based on chances
 void put_items_from(items_location loc, int num, int x, int y, int turn = 0);
 void add_spawn(mon_id type, int count, int x, int y, bool friendly = false,
                int faction_id = -1, int mission_id = -1,
                std::string name = "NONE");
 void add_spawn(monster *mon);
 void create_anomaly(int cx, int cy, artifact_natural_property prop);
 vehicle* add_vehicle(vhtype_id type, int x, int y, int dir);
 computer* add_computer(int x, int y, std::string name, int security);

 static constexpr bool in_bounds(int x, int y) { return 0 <= x && x < SEE* MAPSIZE && 0 <= y && y < SEE* MAPSIZE; }
 static constexpr bool in_bounds(const point& p) { return in_bounds(p.x, p.y); }
 static void init();
protected:
 void saven(const tripoint& om_pos, unsigned int turn, const point& world, int gridx, int gridy);
 bool loadn(game *g, const point& world, int gridx, int gridy);
 bool loadn(const tripoint& GPS, int gridx, int gridy);
 void copy_grid(int to, int from);
 void draw_map(oter_id terrain_type, oter_id t_north, oter_id t_east,
               oter_id t_south, oter_id t_west, oter_id t_above, int turn,
               game *g);
 void add_extra(map_extra type, game *g);
 void rotate(int turns);// Rotates the current map 90*turns degress clockwise
			// Useful for houses, shops, etc

 bool inbounds(int x, int y) const;
 bool is_tiny() const { return 2== my_MAPSIZE; };

 int my_MAPSIZE;
 std::vector<submap*> grid;
private:
 void process_active_items_in_submap(game *g, int nonant);
 bool process_fields_in_submap(game *g, int gridn);	// See fields.cpp
 void _translate(ter_id from, ter_id to);	// error-checked backend for map::translate
};

class tinymap : public map	// XXX direct subclassing defeats the point of this class \todo fix this
{
public:
 tinymap() : map(2) {};
 ~tinymap() = default;
};

#endif
