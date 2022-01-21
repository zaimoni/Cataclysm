#ifndef _MAP_H_
#define _MAP_H_

#include "Zaimoni.STL/GDI/box.hpp" // early, to trigger conditional includes
#include "mapitems.h"
#include "overmap.h"
#include "submap.h"
#include "options.h"
#include <functional>
#include <string>
#include <optional>

#define MAPSIZE 11

class player;
class monster;
class vehicle;
class overmap;

// We do not want to use the Curiously Recurring Template Pattern to deal with the submap grid
class map
{
 public:
 static constexpr const zaimoni::gdi::box<point> reality_bubble_extent = zaimoni::gdi::box<point>(point(0), point(SEE* MAPSIZE));
 // \todo? Socrates' Daimon links map.cpp: move definition to map.cpp
 static const zaimoni::gdi::box<point>& view_center_extent() {
	 static const zaimoni::gdi::box<point> ooao(point(-VIEW_CENTER), point(VIEW_CENTER));
	 return ooao;
 }
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
 void save(const OM_loc<1>& GPS, unsigned int turn) { save(GPS.first, turn, GPS.second); };
 void save(const OM_loc<2>& GPS, unsigned int turn) { save(GPS.first, turn, 2 * GPS.second); };
 void load(game *g, const point& world);
 void load(const tripoint& GPS);
 void load(const OM_loc<1>& GPS);
 void load(const OM_loc<2>& GPS) { load(OM_loc<1>(GPS.first, 2 * GPS.second)); }
 void shift(game *g, const point& world, const point& delta);
 void spawn_monsters(game *g);
 void clear_spawns();
 void clear_traps();

// Movement and LOS
 std::optional<reality_bubble_loc> to(const point& pt) const;
 std::optional<reality_bubble_loc> to(int x, int y) const { return to(point(x,y)); };
 static std::optional<reality_bubble_loc> to(const GPS_loc& pt);
 submap* chunk(const GPS_loc& pt);
 const submap* chunk(const GPS_loc& pt) const { return const_cast<map*>(this)->chunk(pt); }

 bool find_stairs(const point& pt, int movez, point& pos) const;
 bool find_terrain(const point& pt, ter_id dest, point& pos) const;

 int move_cost(int x, int y) const; // Cost to move through; 0 = impassible
 int move_cost(const point& pt) const { return move_cost(pt.x, pt.y); };
 int move_cost(const reality_bubble_loc& pos) const;
 int move_cost_ter_only(int x, int y) const; // same as above, but don't take vehicles into account
 bool trans(const point& pt) const; // Transparent?
 bool trans(const reality_bubble_loc& pos) const;
 std::optional<int> _BresenhamLine(int Fx, int Fy, int Tx, int Ty, int range, std::function<bool(reality_bubble_loc)> test) const;
 // (Fx, Fy) sees (Tx, Ty), within a range of (range)?
 // tc indicates the Bresenham line used to connect the two points, and may
 //  subsequently be used to form a path between them
 std::optional<int> sees(int Fx, int Fy, int Tx, int Ty, int range) const;
 std::optional<int> sees(const point& F, int Tx, int Ty, int range) const { return sees(F.x, F.y, Tx, Ty, range); };
 std::optional<int> sees(const point& F, const point& T, int range) const { return sees(F.x, F.y, T.x, T.y, range); };
 std::optional<int> sees(int Fx, int Fy, const point& T, int range) const { return sees(Fx, Fy, T.x, T.y, range); };
 // clear_path is the same idea, but uses cost_min <= move_cost <= cost_max
 std::optional<int> clear_path(int Fx, int Fy, int Tx, int Ty, int range, int cost_min, int cost_max) const;
// route() generates an A* best path; if bash is true, we can bash through doors
 std::vector<point> route(int Fx, int Fy, int Tx, int Ty, bool bash = true) const;
 std::vector<point> route(const point& F, int Tx, int Ty, bool bash = true) const { return route(F.x, F.y, Tx, Ty, bash); };
 std::vector<point> route(const point& F, const point& T, bool bash = true) const { return route(F.x, F.y, T.x, T.y, bash); };
 std::vector<point> route(int Fx, int Fy, const point& T, bool bash = true) const { return route(Fx, Fy, T.x, T.y, bash); };

// vehicles
// checks, if tile is occupied by vehicle and by which part.  These are deep const correct
 std::optional<std::pair<const vehicle*, int>> _veh_at(const point& src) const;
 std::optional<std::pair<vehicle*, int>> _veh_at(const point& src);
 std::optional<std::pair<const vehicle*, int>> _veh_at(int x, int y) const { return _veh_at(point(x, y)); }
 std::optional<std::pair<vehicle*, int>> _veh_at(int x, int y) { return _veh_at(point(x, y)); }
 std::optional<std::pair<const vehicle*, int>> veh_at(const reality_bubble_loc& src) const;
 std::optional<std::pair<vehicle*, int>> veh_at(const reality_bubble_loc& src);

 vehicle* veh_near(const point& pt);
 std::optional<std::vector<std::pair<point, vehicle*> > > all_veh_near(const point& pt);

 // put player on vehicle at x,y
 bool try_board_vehicle(game* g, int x, int y, player& p);

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
 ter_id& ter(const reality_bubble_loc& src) { return grid[src.first]->ter[src.second.x][src.second.y]; };
 ter_id ter(const reality_bubble_loc& src) const { return grid[src.first]->ter[src.second.x][src.second.y]; };
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

 std::string features(const point& pt) const; // Words relevant to terrain (sharp, etc)
 bool has_flag(t_flag flag, int x, int y) const;  // checks terrain and vehicles
 bool has_flag(t_flag flag, const point& pt) const { return has_flag(flag, pt.x, pt.y); };
 bool has_flag(t_flag flag, const reality_bubble_loc& pos) const;
 bool is_destructable(int x, int y) const;        // checks terrain and vehicles
 bool is_destructable(const point& pt) const { return is_destructable(pt.x, pt.y); }
 bool is_outside(int x, int y) const;
 bool is_outside(const point& pt) const { return is_outside(pt.x, pt.y); };
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
 void shoot(game *g, const point& pt, int &dam, bool hit_items, unsigned flags);
 bool hit_with_acid(const point& pt);
 void marlossify(int x, int y);

// Radiation
 template<class...Args>
 int& radiation(Args...params)
 {
	 if (const auto pos = to(params...)) return grid[pos->first]->rad[pos->second.x][pos->second.y];
	 return cataclysm::discard<int>::x = 0;
 }

// Items
 template<class...Args>
 std::vector<item>& i_at(Args...params)
 {
	 if (const auto pos = to(params...)) return grid[pos->first]->itm[pos->second.x][pos->second.y];
	 cataclysm::discard<std::vector<item> >::x.clear();
	 return cataclysm::discard<std::vector<item> >::x;
 }

 std::vector<item>& i_at(const reality_bubble_loc& pos) { return grid[pos.first]->items_at(pos.second); }

 template<class...Args>
 const std::vector<item>& i_at(Args...params) const { return const_cast<map*>(this)->i_at(params...); }

 std::optional<item> water_from(const point& pt) const;
 void i_clear(int x, int y);
 void i_clear(const point& pt) { return i_clear(pt.x, pt.y); };
 void i_rem(int x, int y, int index);
 void i_rem(const point& pt, int index) { i_rem(pt.x, pt.y, index); };
 std::optional<std::pair<point, int>> find_item(item* it) const;
 // consider GPS_loc::add instead of these
 void add_item(int x, int y, const itype* type, int birthday);
 void add_item(const point& pt, const itype* type, int birthday) { add_item(pt.x, pt.y, type, birthday); };
 void add_item(int x, int y, const item& new_item);
 void add_item(const point& pt, const item& new_item) { return add_item(pt.x, pt.y, new_item); };
 void add_item(int x, int y, item&& new_item);
 void add_item(const point& pt, item&& new_item) { return add_item(pt.x, pt.y, std::move(new_item)); }
 bool hard_landing(const point& pt, item&& thrown, player* p = nullptr); // for thrown objects
 void process_active_items();
// void process_vehicles(game *g);	// undefined function

 void use_amount(point origin, int range, itype_id type, int quantity,
                 bool use_container = false);
 void use_charges(point origin, int range, itype_id type, int quantity);

// Traps
 trap_id& tr_at(int x, int y);
 trap_id& tr_at(const point& pt) { return tr_at(pt.x, pt.y); };
 trap_id& tr_at(const reality_bubble_loc& src) { return grid[src.first]->trp[src.second.x][src.second.y]; };
 trap_id tr_at(const reality_bubble_loc& src) const { return grid[src.first]->trp[src.second.x][src.second.y]; };
 trap_id tr_at(int x, int y) const { return const_cast<map*>(this)->tr_at(x, y); };	// \todo specialize this properly
 trap_id tr_at(const point& pt) const { return const_cast<map*>(this)->tr_at(pt.x, pt.y); };
 void add_trap(int x, int y, trap_id t);
 void add_trap(const point& pt, trap_id t) { add_trap(pt.x, pt.y, t); }

// Fields
 template<class...Args>
 field& field_at(Args...params) {
	 if (const auto pos = to(params...)) return grid[pos->first]->field_at(pos->second);
	 return (cataclysm::discard<field>::x = field());
 }

 template<class...Args>
 const field& field_at(Args...params) const {
	 if (const auto pos = to(params...)) return grid[pos->first]->field_at(pos->second);
	 return (cataclysm::discard<field>::x = field());
 }

 bool add_field(game *g, int x, int y, field_id t, unsigned char density, unsigned int age=0);
 bool add_field(game *g, const point& pt, field_id t, unsigned char density, unsigned int age = 0) { return add_field(g, pt.x, pt.y, t, density, age); };

 template<class...Args>
 void remove_field(Args...params)
 {
	 if (const auto pos = to(params...)) grid[pos->first]->remove_field(pos->second);
 }

 bool process_fields(game *g);				// See field.cpp
 void step_in_field(game* g, player& u);		// See field.cpp	// V 0.2.5+ break hard-coding to player g->u
 void mon_in_field(game* g, monster& z);	// See field.cpp

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
                int mission_id = -1, std::string name = std::string(),
                int faction_id = -1);
 void create_anomaly(int cx, int cy, artifact_natural_property prop);
 vehicle* add_vehicle(vhtype_id type, point pos, int deg);
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
