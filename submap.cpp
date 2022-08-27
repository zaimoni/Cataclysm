#include "submap.h"
#include "monster.h"
#include "output.h"
#include "Zaimoni.STL/Logging.h"

void submap::set(const tripoint src, const Badge<mapbuffer>& auth) {
    GPS = src;

    // Automatic-repair anything with GPSpos fields, here.  Catches mapgen mismatches between game::lev and the global position of the submap chunk.
    for (decltype(auto) veh : vehicles) veh->GPSpos.first = src;
}

void submap::add(item&& new_item, const point& dest)
{
    if (new_item.active) active_item_count++;
    items_at(dest).push_back(std::move(new_item));
}

std::optional<item> submap::for_drop(ter_id dest, const itype* type, int birthday)
{
    if (type->is_style()) return std::nullopt;
    // ignore corpse special case here when inlining item::made_of
    if ((type->m1 == LIQUID || type->m2 == LIQUID)
        && is<swimmable>(dest))
        return std::nullopt;
    return item(type, birthday).in_its_container();
}

field* submap::add(const point& p, field&& src)
{
    auto& fd = field_at(p);
    if (fd.type == fd_web && src.type == fd_fire) src.density++;
    else if (!fd.is_null()) return nullptr; // Blood & bile are null too
    if (3 < src.density) src.density = 3;
    if (0 >= src.density) return nullptr;
    if (fd.type == fd_null) field_count++;
    return &(fd = std::move(src));
}

void submap::remove_field(const point& p) {
    if (fld[p.x][p.y].type != fd_null) field_count--;
    fld[p.x][p.y] = field();
}

void submap::add_spawn(mon_id type, int count, const point& pt, bool friendly, int faction_id, int mission_id, std::string name)
{
    if (!in_bounds(pt)) {
        debugmsg("Bad add_spawn(%d, %d, %d, %d)", type, count, pt.x, pt.y);
        debuglog("Bad add_spawn(%d, %d, %d, %d)", type, count, pt.x, pt.y);
        return;
    }
    spawns.emplace_back(type, count, pt.x, pt.y, faction_id, mission_id, friendly, name);
}

void submap::add_spawn(const monster& mon)
{
    add_spawn(mon_id(mon.type->id), 1, mon.GPSpos.second, (mon.friendly < 0), mon.faction_id, mon.mission_id, mon.unique_name);
}

std::optional<std::pair<vehicle*, int>> submap::veh_at(const GPS_loc& loc)
{
    for (decltype(auto) veh : vehicles) {
        const auto delta = loc - veh->GPSpos;
        if (const point* const pt = std::get_if<point>(&delta)) { // gross invariant failure: vehicles should have GPSpos tripoint of their submap
            int part = veh->part_at(*pt);
            if (part >= 0) return std::pair(veh.get(), part);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<const vehicle*, int>> submap::veh_at(const GPS_loc& loc) const
{
    if (auto ret = const_cast<submap*>(this)->veh_at(loc)) return std::pair(ret->first, ret->second);
    return std::nullopt;
}

computer* submap::add_computer(const point& pt, std::string&& name, int security)
{
    terrain(pt) = t_console; // TODO: Turn this off?
    comp = computer(std::move(name), security);
    return &comp;
}

vehicle* submap::add_vehicle(vhtype_id type, point pos, int deg)
{
    assert(in_bounds(pos));
    vehicles.emplace_back(new vehicle(type, deg));
    vehicles.back()->GPSpos = GPS_loc(GPS, pos);
    return vehicles.back().get();
}

void submap::add(std::shared_ptr<vehicle> veh, const Badge<map>& auth)
{
    if (veh) {
        veh->GPSpos.first = GPS; // enforce invariant
        vehicles.push_back(veh);
    };
}

void submap::destroy(vehicle& veh)
{
    int i = -1;
    for (decltype(auto) v : vehicles) {
        ++i;
        if (v.get() == &veh) {
            EraseAt(vehicles, i);
            return;
        }
    }

    debuglog("submap::destroy can't find it!");
}

void submap::veh_gain_moves(proxy_vehicles_t& acc, const Badge<map>& auth)
{
    for (decltype(auto) veh : vehicles) {
        if (0 == veh->velocity) continue;
        veh->gain_moves(abs(veh->velocity)); // velocity is ability to make more one-tile steps per turn
        if (0 < veh->moves) acc.push_back(veh);
    }
}

void submap::rotate_vehicles(int turns, const Badge<map>& auth) {
    for (decltype(auto) veh : vehicles) veh->turn(turns * 90);
}

void submap::mapgen_swap(submap& dest, const Badge<map>& auth) {
    std::swap(comp, dest.comp);
    vehicles.swap(dest.vehicles);
    for (decltype(auto) veh : vehicles) veh->GPSpos.first = GPS;
    for (decltype(auto) veh : dest.vehicles) veh->GPSpos.first = dest.GPS;
}

void submap::mapgen_move_cycle(submap* const* cycle, ptrdiff_t ub, const Badge<map>& auth)
{
    assert(2 <= ub);
    assert(cycle);

    computer   t_comp(std::move(cycle[--ub]->comp));
    vehicles_t t_vehs(std::move(cycle[ub]->vehicles));

    while (0 <= --ub) {
        cycle[ub + 1]->comp     = std::move(cycle[ub]->comp);
        cycle[ub + 1]->vehicles = std::move(cycle[ub]->vehicles);
    };

    cycle[0]->comp     = std::move(t_comp);
    cycle[0]->vehicles = std::move(t_vehs);
}

void submap::mapgen_xform(point(*op)(const point&), const Badge<map>& auth)
{
    for (decltype(auto) veh : vehicles) veh->GPSpos.second = op(veh->GPSpos.second);
}

void submap::post_init(const Badge<defense_game>& auth)
{
    spawns.clear();
    memset(trp, 0, sizeof(trp)); // tr_null defined as 0
}

