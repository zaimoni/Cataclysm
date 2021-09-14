#include "vehicle.h"
#include "game.h"
#include "line.h"
#include "recent_msg.h"
#include "rng.h"
#include "saveload.h"

#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <array>

void trap_fully_triggered(map& m, const point& pt, const std::vector<item_drop_spec>& drop_these); // trapfunc.cpp

static const char* const JSON_transcode_vparts[] = {
	"seat",
	"frame_h",
	"frame_v",
	"frame_c",
	"frame_y",
	"frame_u",
	"frame_n",
	"frame_b",
	"frame_h2",
	"frame_v2",
	"frame_cover",
	"frame_handle",
	"board_h",
	"board_v",
	"board_y",
	"board_u",
	"board_n",
	"board_b",
	"roof",
	"door",
	"window",
	"blade_h",
	"blade_v",
	"spike_h",
	"spike_v",
	"wheel_large",
	"wheel",
	"engine_gas_small",
	"engine_gas_med",
	"engine_gas_large",
	"engine_motor",
	"engine_motor_large",
	"engine_plasma",
	"fuel_tank_gas",
	"fuel_tank_batt",
	"fuel_tank_plut",
	"fuel_tank_hydrogen",
	"cargo_trunk",
	"cargo_box",
	"controls",
	"muffler",
	"seatbelt",
	"solar_panel",
	"m249",
	"flamethrower",
	"plasmagun",
	"steel_plate",
	"superalloy_plate",
	"spiked_plate",
	"hard_plate"
};

static const char* const JSON_transcode_vtypes[] = {
	"custom",
	"motorcycle",
	"sandbike",
	"car",
	"truck"
};

DEFINE_JSON_ENUM_SUPPORT_TYPICAL(vhtype_id, JSON_transcode_vtypes)
DEFINE_JSON_ENUM_SUPPORT_TYPICAL(vpart_id, JSON_transcode_vparts)

std::vector<const vehicle*> vehicle::vtypes;

vehicle::vehicle(vhtype_id type_id)
: _type(type_id), insides_dirty(true), velocity(0), cruise_velocity(0), cruise_on(true),
  turn_dir(0), skidding(false), last_turn(0), turret_mode(0)
{
    if (veh_custom < _type && vehicle::vtypes.size() > _type)
    {   // get a copy of sample vehicle of this type
        *this = *(vehicle::vtypes[_type]);
        init_state();
    }
    precalc_mounts(0, face.dir());
}

vehicle::vehicle(vhtype_id type_id, int deg) : vehicle(type_id)
{
    face.init(deg);
    turn_dir = deg;
    precalc_mounts(0, deg);
}

DEFINE_ACID_ASSIGN_W_MOVE(vehicle)

std::string vehicle::subject() const
{
    return name;
}

std::string vehicle::direct_object() const
{
    return name;
}

std::string vehicle::indirect_object() const
{
    return name;
}

std::string vehicle::possessive() const
{
    auto ret(name);
    regular_possessive(ret);
    return ret;
}

bool vehicle::player_in_control(const player& p) const
{
    if (!_type || !p.in_vehicle) return false;
    if (const auto veh = p.GPSpos.veh_at()) {
        return veh->first == this && 0 <= part_with_feature(veh->second, vpf_controls, false);
    }
    return false;
}

player* vehicle::driver() const
{
    if (!_type) return nullptr;
    const auto pos = screen_pos();
    if (!pos) return nullptr; // not in reality bubble
    for (int p = 0; p < parts.size(); p++) {
        const auto& p_info = part_info(p);
        if (p_info.has_flag<vpf_controls>()) {
            if (player* pl = game::active()->survivor(*pos + parts[p].precalc_d[0])) {
                if (pl->in_vehicle) return pl;
            }
        }
    }
    return nullptr;
}


void vehicle::init_state()
{
    for (int p = 0; p < parts.size(); p++) {
        const auto& p_info = part_info(p);
        if (p_info.has_flag<vpf_fuel_tank>()) { // 10% to 75% fuel for tank
            const int s = p_info._size;
            parts[p].amount = rng(s / 10, s * 3 / 4);
        }
        if (p_info.has_flag<vpf_openable>()) parts[p].open = 0; // doors are closed
        if (p_info.has_flag<vpf_seat>()) parts[p].passenger = 0; // no passengers
    }
}

const vpart_info& vehicle::part_info(int index) const
{
    vpart_id id = (index < 0 || index >= parts.size()) ? vp_null : parts[index].id;
    if (id < vp_null || id >= num_vparts) id = vp_null;
    return vpart_info::list[id];
}

bool vehicle::can_mount (int dx, int dy, vpart_id id) const
{
    if (id <= 0 || id >= num_vparts) return false;
	const auto& p_info = vpart_info::list[id];
    bool n3ar = parts.empty() || (p_info.flags & (mfb(vpf_internal) | mfb(vpf_over))); // first and internal parts needs no mount point
    if (!n3ar)
        for (int i = 0; i < 4; i++) {
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            std::vector<int> parts_n3ar = parts_at_relative (dx + ndx, dy + ndy);
            if (parts_n3ar.empty()) continue;
            if (part_flag(parts_n3ar[0], vpf_mount_point)) {
                n3ar = true;
                break;
            }
        }
    if (!n3ar) return false; // no point to mount

// TODO: seatbelts must require an obstacle part n3arby

    std::vector<int> parts_here = parts_at_relative (dx, dy);
    if (parts_here.empty()) return p_info.has_flag<vpf_external>(); // can be mounted if first and external

    const int flags1 = part_info(parts_here[0]).flags;
    if (     p_info.has_flag<vpf_armor>()
		 && (flags1 & mfb(vpf_no_reinforce)))
		return false;   // trying to put armor plates on non-reinforcable part

    for (int vf = vpf_func_begin; vf <= vpf_func_end; vf++)
        if (p_info.has_flag(vf) && 0 <= part_with_feature(parts_here[0], vf, false)) {
            return false;   // this part already has inner part with same unique feature
        }

    const bool allow_inner = flags1 & mfb(vpf_mount_inner);
	const bool allow_over  = flags1 & mfb(vpf_mount_over);
	const bool this_inner  = p_info.has_flag<vpf_internal>();
	const bool this_over   = p_info.flags & (mfb(vpf_over) | mfb(vpf_armor));
    if (allow_inner && (this_inner || this_over)) return true; // can mount as internal part or over it
    if (allow_over && this_over) return true; // can mount as part over
    return false;
}

bool vehicle::can_unmount(int p) const
{
    int dx = parts[p].mount_d.x;
    int dy = parts[p].mount_d.y;
    if (!dx && !dy)
    { // central point
        bool is_ext = false;
        for (int ep = 0; ep < external_parts.size(); ep++)
            if (external_parts[ep] == p)
            {
                is_ext = true;
                break;
            }
        if (external_parts.size() > 1 && is_ext) return false; // unmounting 0, 0 part anly allowed as last part
    }

    if (!part_flag (p, vpf_mount_point)) return true;
    for (int i = 0; i < 4; i++)
    {
        int ndx = i < 2? (i == 0? -1 : 1) : 0;
        int ndy = i < 2? 0 : (i == 2? - 1: 1);
        if (!(dx + ndx) && !(dy + ndy)) continue; // 0, 0 point is main mount
        if (parts_at_relative (dx + ndx, dy + ndy).size() > 0)
        {
            int cnt = 0;
            for (int j = 0; j < 4; j++)
            {
                int jdx = j < 2? (j == 0? -1 : 1) : 0;
                int jdy = j < 2? 0 : (j == 2? - 1: 1);
                std::vector<int> pc = parts_at_relative (dx + ndx + jdx, dy + ndy + jdy);
                if (pc.size() > 0 && part_with_feature (pc[0], vpf_mount_point) >= 0)
                    cnt++;
            }
            if (cnt < 2) return false;
        }
    }
    return true;
}

int vehicle::install_part (int dx, int dy, vpart_id id, int hp, bool force)
{
    if (!force && !can_mount (dx, dy, id))
        return -1;  // no money -- no ski!
    // if this is first part, add this part to list of external parts
    if (parts_at_relative (dx, dy).size () < 1)
        external_parts.push_back (parts.size());
    parts.push_back(vehicle_part(id,dx,dy, hp < 0 ? vpart_info::list[id].durability : hp));
    find_exhaust ();
    precalc_mounts (0, face.dir());
    insides_dirty = true;
    return parts.size() - 1;
}

void vehicle::remove_part (int p)
{
    EraseAt(parts, p);
    find_external_parts ();
    find_exhaust ();
    precalc_mounts (0, face.dir());    
    insides_dirty = true;
}


std::vector<int> vehicle::parts_at_relative (int dx, int dy) const
{
    std::vector<int> res;
    for (int i = 0; i < parts.size(); i++)
        if (parts[i].mount_d.x == dx && parts[i].mount_d.y == dy)
            res.push_back (i);
    return res;
}

std::vector<int> vehicle::internal_parts(int p) const
{
    std::vector<int> res;
	const auto& ref_part = parts[p];
    for (int i = p + 1; i < parts.size(); i++)
        if (parts[i].mount_d == ref_part.mount_d)
            res.push_back (i);
    return res;
}

int vehicle::part_with_feature(int p, unsigned int f, bool unbroken) const
{
    if (part_flag(p, f)) return p;
	for (const auto n : internal_parts(p))
        if (part_flag(n, f) && (!unbroken || parts[n].hp > 0))
            return n;
    return -1;
}

bool vehicle::part_flag(int p, unsigned int f) const
{
    if (p < 0 || p >= parts.size()) return false;
    return part_info(p).flags & mfb(f);
}

int vehicle::part_at(int dx, int dy) const
{
	for (const auto p : external_parts) {
		const auto& part = parts[p];
        if (part.precalc_d[0].x == dx &&
			part.precalc_d[0].y == dy)
            return p;
    }
    return -1;
}

char vehicle::part_sym (int p) const
{
    if (p < 0 || p >= parts.size()) return 0;

    std::vector<int> ph = internal_parts (p);
    int po = part_with_feature(p, vpf_over, false);
    int pd = po < 0? p : po;
    const auto& pd_info = part_info(pd);

    if (pd_info.has_flag<vpf_openable>() && parts[pd].open) return '\''; // open door
    return 0 >= parts[pd].hp ? pd_info.sym_broken : pd_info.sym;
}

nc_color vehicle::part_color (int p) const
{
    if (p < 0 || p >= parts.size()) return c_black;
    int parm = part_with_feature(p, vpf_armor, false);
    int po = part_with_feature(p, vpf_over, false);
    int pd = po < 0? p : po;
    if (parts[p].blood > 200) return c_red;
    else if (parts[p].blood > 0) return c_ltred;
    
    if (parts[pd].hp <= 0) return part_info(pd).color_broken;

    // first, check if there's a part over. then, if armor here (projects its color on part)
    if (po >= 0) return part_info(po).color;
    else if (parm >= 0) return part_info(parm).color;

    return part_info(pd).color;
}

static constexpr nc_color part_desc_color(int hp, int max_hp)
{
    if (hp >= max_hp) return c_green;
    else if (0 >= hp) return c_dkgray;

    const int per_cond = hp * 100 / (max_hp < 1 ? 1 : max_hp);
    if      (80 <= per_cond) return c_ltgreen;
    else if (50 <= per_cond) return c_yellow;
    else if (20 <= per_cond) return c_ltred;
    else return c_red;
}

// normal use
static_assert(c_green == part_desc_color(101, 100));
static_assert(c_green == part_desc_color(100, 100));
static_assert(c_ltgreen == part_desc_color(99, 100));
static_assert(c_ltgreen == part_desc_color(80, 100));
static_assert(c_yellow == part_desc_color(79, 100));
static_assert(c_yellow == part_desc_color(50, 100));
static_assert(c_ltred == part_desc_color(49, 100));
static_assert(c_ltred == part_desc_color(20, 100));
static_assert(c_red == part_desc_color(19, 100));
static_assert(c_red == part_desc_color(1, 100));
static_assert(c_dkgray == part_desc_color(0, 100));
static_assert(c_dkgray == part_desc_color(-1, 100));

// pathological cases -- ok to alter these test cases
static_assert(c_green == part_desc_color(2, 0));
static_assert(c_green == part_desc_color(1, 0));
static_assert(c_green == part_desc_color(0, 0));
static_assert(c_dkgray == part_desc_color(-1, 0));

static_assert(c_green == part_desc_color(2, -1));
static_assert(c_green == part_desc_color(1, -1));
static_assert(c_green == part_desc_color(0, -1));
static_assert(c_green == part_desc_color(-1, -1));
static_assert(c_dkgray == part_desc_color(-2, -1));

void vehicle::print_part_desc (void *w, int y1, int p, int hl)
{
    if (p < 0 || p >= parts.size()) return;
    WINDOW* win = (WINDOW*)w;
    const int width = getmaxx(win);
    std::vector<int> pl = internal_parts (p);
    pl.insert (pl.begin(), p);
    int y = y1;
    for (int i = 0; i < pl.size(); i++) {
        const auto& pl_i_info = part_info(pl[i]);
        nc_color col_cond = part_desc_color(parts[pl[i]].hp, pl_i_info.durability);

        const bool armor = pl_i_info.has_flag<vpf_armor>();
        mvwputch(win, y, 1, i == hl ? hilite(c_ltgray) : c_ltgray, armor ? '(' : (i ? '-' : '['));
        mvwaddstrz(win, y, 2, i == hl ? hilite(col_cond) : col_cond, pl_i_info.name);
        mvwputch(win, y, 2 + strlen(pl_i_info.name), i == hl ? hilite(c_ltgray) : c_ltgray, armor ? ')' : (i ? '-' : ']'));

        if (i == 0) mvwaddstrz(win, y, width-5, c_ltgray, is_inside(pl[i])? " In " : "Out ");
        y++;
    }
}

void vehicle::print_fuel_indicator(void *w, int y, int x) const
{
    static constexpr const ammotype fuel_types[] = { AT_GAS, AT_BATT, AT_PLUT, AT_PLASMA };   // 2020-07-12 arguably could be sunk to vehicle::print_fuel_indicator
    static constexpr const nc_color fcs[] = { c_ltred, c_yellow, c_ltgreen, c_ltblue };
    static_assert(sizeof(fuel_types) / sizeof(*fuel_types) == sizeof(fcs) / sizeof(*fcs));

    static constexpr const char fsyms[] = { 'E', '\\', '|', '/', 'F' };
    static_assert(sizeof(fsyms) / sizeof(*fsyms) == sizeof("E...F")-1);

    static constexpr const size_t num_fuel_types = sizeof(fuel_types) / sizeof(*fuel_types);

    WINDOW *win = (WINDOW *) w;
    mvwaddstrz(win, y, x, c_ltgray, "E...F");
    for (int i = 0; i < num_fuel_types; i++) {
        if (int cap = fuel_capacity(fuel_types[i]); 0 < cap) {
            int amnt = fuel_left(fuel_types[i]) * 99 / cap; // can't use 100 as then we might overflow
            int indf = (amnt / 20) % 5;
            mvwputch(win, y, x + indf, fcs[i], fsyms[indf]);
        }
    }
}

point vehicle::coord_translate (int dir, point reld)
{
    tileray tdir (dir);
    tdir.advance (reld.x);
	return point(tdir.dx() + tdir.ortho_dx(reld.y), tdir.dy() + tdir.ortho_dy(reld.y));
}

void vehicle::precalc_mounts (int idir, int dir)
{
    if (idir < 0 || idir > 1) idir = 0;
	for(auto& part : parts) part.precalc_d[idir] = coord_translate(dir, part.mount_d);
}

bool vehicle::any_boarded_parts() const
{
	for (const auto& part : parts) {
		if (!part.has_flag(vpf_seat)) continue;
		assert(part.get_passenger(screenPos()) == part.get_passenger(GPSpos));
		if (part.get_passenger(GPSpos)) return true;
	}
	return false;
}

std::vector<int> vehicle::boarded_parts() const
{
    std::vector<int> res;
    for (int p = 0; p < parts.size(); p++)
        if (part_flag (p, vpf_seat) && parts[p].passenger)
            res.push_back (p);
    return res;
}

player* vehicle_part::get_passenger(point origin) const
{
	if (passenger) {
		origin += precalc_d[0];	// where we are relative to our global position
		auto g = game::active();
		if (g->u.pos == origin && g->u.in_vehicle) return &g->u;
		if (npc* const nPC = g->nPC(origin)) return nPC;	// \todo V0.2.4+ require in_vehicle when it is possible for NPCs to initiate boarding vehicles
	}
	return nullptr;
}

player* vehicle_part::get_passenger(GPS_loc origin) const
{
    if (passenger) {
        origin += precalc_d[0];	// where we are relative to our global position
        auto g = game::active();
        if (g->u.GPSpos == origin && g->u.in_vehicle) return &g->u;
        if (npc* const nPC = g->nPC(origin)) return nPC;	// \todo V0.2.4+ require in_vehicle when it is possible for NPCs to initiate boarding vehicles
    }
    return nullptr;
}

player *vehicle::get_passenger(int p) const
{
    p = part_with_feature (p, vpf_seat, false);
    if (0 > p) return nullptr;
    assert(parts[p].get_passenger(screenPos()) == parts[p].get_passenger(GPSpos));
	return parts[p].get_passenger(GPSpos);
}

std::vector<std::pair<int, player*> > vehicle::passengers() const
{
	std::vector<std::pair<int, player*> > res;
	int p = -1;
	for (const auto& part : parts) {
		++p;
		if (!part.has_flag(vpf_seat)) continue;
        assert(parts[p].get_passenger(screenPos()) == parts[p].get_passenger(GPSpos));
		if (auto pl = part.get_passenger(GPSpos)) res.push_back({ p, pl });
	}
	return res;
}

int vehicle::total_mass() const
{
    int m = 0;
    for (int i = 0; i < parts.size(); i++) {
        const auto& p_info = part_info(i);
        m += item::types[p_info.item]->weight;
		for (const auto& it : parts[i].items) m += it.type->weight;
        if (p_info.has_flag<vpf_seat>() && parts[i].passenger)
            m += 520; // TODO: get real weight
    }
    return m;
}

int vehicle::fuel_left(ammotype ftype, bool for_engine) const
{
    int fl = 0;
	for (int p = 0; p < parts.size(); p++) {
        const auto& p_info = part_info(p);
        if (!p_info.has_flag<vpf_fuel_tank>()) continue;
		if ((ftype == p_info.fuel_type || (for_engine && ftype == AT_BATT && p_info.fuel_type == AT_PLUT)))
			fl += parts[p].amount;
	}
    return fl;
}

int vehicle::fuel_capacity(ammotype ftype) const
{
    int cap = 0;
	for (int p = 0; p < parts.size(); p++) {
        const auto& p_info = part_info(p);
        if (p_info.has_flag<vpf_fuel_tank>() && ftype == p_info.fuel_type) cap += p_info._size;
	}
    return cap;
}

int vehicle::refill (int ftype, int amount)
{
    for (int p = 0; p < parts.size(); p++) {
        const auto& p_info = part_info(p);
        if (!p_info.has_flag<vpf_fuel_tank>()) continue;
        if (p_info.fuel_type == ftype && parts[p].amount < p_info._size) {
            const int need = p_info._size - parts[p].amount;
            if (amount < need) {
                parts[p].amount += amount;
                return 0;
            }
            parts[p].amount += need;
            amount -= need;
        }
    }
    return amount;
}

const char* vehicle::fuel_name(ammotype ftype)
{
    switch (ftype)
    {
    case AT_GAS: return "gasoline";
    case AT_BATT: return "batteries";
    case AT_PLUT: return "plutonium cells";
    case AT_PLASMA: return "hydrogen";
    default: return "INVALID FUEL (BUG)";
    }
}

int vehicle::basic_consumption(int ftype) const
{
    if (ftype == AT_PLUT) ftype = AT_BATT;
    int cnt = 0;
    int fcon = 0;
	for (int p = 0; p < parts.size(); p++) {
        if (0 >= parts[p].hp) continue;
        const auto& p_info = part_info(p);
        if (!p_info.has_flag<vpf_engine>()) continue;
        if (ftype == p_info.fuel_type) {
            fcon += p_info.power;
            cnt++;
        }
	}
    if (fcon < 100 && cnt > 0) fcon = 100;
    return fcon;
}

int vehicle::total_power(bool fueled) const
{
    int pwr = 0;
    int cnt = 0;
	for (int p = 0; p < parts.size(); p++) {
        if (0 >= parts[p].hp) continue;
        const auto& p_info = part_info(p);
        if (!p_info.has_flag<vpf_engine>()) continue;
		if (fuel_left(p_info.fuel_type, true) || !fueled) {
			pwr += p_info.power;
			cnt++;
		}
	}
    if (cnt > 1) pwr = pwr * 4 / (4 + cnt -1);
    return pwr;
}

int vehicle::solar_power() const
{
    int pwr = 0;
    for (int p = 0; p < parts.size(); p++) {
        if (0 >= parts[p].hp) continue;
        const auto& p_info = part_info(p);
        if (p_info.has_flag<vpf_solar_panel>()) pwr += p_info.power;
    }
    return pwr;
}

int vehicle::acceleration(bool fueled) const
{
    return (int) (safe_velocity (fueled) * k_mass() / (1 + strain ()) / 10);
}

int vehicle::max_velocity(bool fueled) const
{
    return total_power(fueled) * 80;
}

int vehicle::safe_velocity(bool fueled) const
{
    int pwrs = 0;
    int cnt = 0;
	for (int p = 0; p < parts.size(); p++) {
        if (0 >= parts[p].hp) continue;
        const auto& p_info = part_info(p);
        if (!p_info.has_flag<vpf_engine>()) continue;
		if (fuel_left(p_info.fuel_type, true) || !fueled) {
			int m2c = 100;
			switch (p_info.fuel_type) {
			case AT_GAS:    m2c = 60; break;
			case AT_PLASMA: m2c = 75; break;
			case AT_BATT:   m2c = 90; break;
			}
			pwrs += p_info.power * m2c / 100;
			cnt++;
		}
	}
    if (cnt > 0) pwrs = pwrs * 4 / (4 + cnt -1);
    return (int) (pwrs * k_dynamics() * k_mass()) * 80;
}

int vehicle::noise(bool fueled, bool gas_only) const
{
    int pwrs = 0;
    int cnt = 0;
    int muffle = 100;
    for (int p = 0; p < parts.size(); p++) {
        if (0 >= parts[p].hp) continue;
        const auto& p_info = part_info(p);
        if (p_info.has_flag<vpf_muffler>() && p_info.bonus < muffle) muffle = p_info.bonus;
    }

    for (int p = 0; p < parts.size(); p++) {
        if (0 >= parts[p].hp) continue;
        const auto& p_info = part_info(p);
        if (!p_info.has_flag<vpf_engine>()) continue;
        if (fuel_left(p_info.fuel_type, true) || !fueled) {
            int nc = 10;
            switch (p_info.fuel_type) {
            case AT_GAS:    nc = 25; break;
            case AT_PLASMA: nc = 10; break;
            case AT_BATT:   nc = 3; break;
            }
            if (!gas_only || p_info.fuel_type == AT_GAS) {
                int pwr = p_info.power * nc / 100;
                if (muffle < 100 && (p_info.fuel_type == AT_GAS ||
                                     p_info.fuel_type == AT_PLASMA))
                    pwr = pwr * muffle / 100;
                pwrs += pwr;
                cnt++;
            }
        }
    }
    return pwrs;
}

int vehicle::wheels_area(int *cnt) const
{
    int count = 0;
    int size = 0;
    for (int p : external_parts) {
        if (0 >= parts[p].hp) continue;
        if (const auto& p_info = part_info(p); p_info.has_flag<vpf_wheel>()) {
            size += p_info._size;
            count++;
        }
    }
    if (cnt) *cnt = count;
    return size;
}

float vehicle::k_dynamics() const
{
    static constexpr const int max_obst = radius + 1;

    int obst[max_obst] = {};

    for (const int p : external_parts) {
        int frame_size = part_flag(p, vpf_obstacle) ? 30 : 10;
        int pos = parts[p].mount_d.y + max_obst / 2;
        if (pos < 0) pos = 0;
        else if (pos >= max_obst) pos = max_obst - 1;
        if (obst[pos] < frame_size) obst[pos] = frame_size;
    }

    int frame_obst = 0;
    for (int o = 0; o < max_obst; o++) frame_obst += obst[o];

    static constexpr const float ae0 = 200.0;
    static constexpr const float fr0 = 1000.0;

    float ka = ae0 / (ae0 + frame_obst); // calculate aerodynamic coefficient
    float kf = fr0 / (fr0 + wheels_area()); // calculate safe speed reduction due to wheel friction

    return ka * kf;
}

float vehicle::k_mass() const
{
    int wa = wheels_area();
    float ma0 = 50.0;

    // calculate safe speed reduction due to mass
    float km = wa > 0? ma0 / (ma0 + total_mass() / 8 / (float) wa) : 0;

    return km;
}

float vehicle::strain() const
{
    const int sv = safe_velocity();
    if (velocity <= sv) return 0.0f;
    const int mv = max_velocity();
    const float denominator = (mv <= sv) ? 1 : mv - sv;
    return (velocity - sv) / denominator;
}

bool vehicle::valid_wheel_config() const
{
    int x1, y1, x2, y2;
    int count = 0;
	for (const auto p : external_parts) {
		if (!part_flag(p, vpf_wheel)) continue;
		const auto& part = parts[p];
        if (part.hp <= 0) continue;
        if (!count) {
            x1 = x2 = part.mount_d.x;
            y1 = y2 = part.mount_d.y;
        }
        if (part.mount_d.x < x1) x1 = part.mount_d.x;
        if (part.mount_d.x > x2) x2 = part.mount_d.x;
        if (part.mount_d.y < y1) y1 = part.mount_d.y;
        if (part.mount_d.y > y2) y2 = part.mount_d.y;
        count++;
    }
    if (count < 2) return false;
    float xo = 0, yo = 0;
    float wo = 0, w2;
    for (int p = 0; p < parts.size(); p++)
    { // lets find vehicle's center of masses
        w2 = item::types[part_info(p).item]->weight;
        if (w2 < 1) continue;
        xo = xo * wo / (wo + w2) + parts[p].mount_d.x * w2 / (wo + w2);
        yo = yo * wo / (wo + w2) + parts[p].mount_d.y * w2 / (wo + w2);
        wo += w2;
    }
    if ((int)xo < x1 || (int)xo > x2 || (int)yo < y1 || (int)yo > y2)
        return false; // center of masses not inside support of wheels (roughly)
    return true;
}

void vehicle::consume_fuel ()
{
    static constexpr const std::array<ammotype, 3> ftypes = { AT_GAS, AT_BATT, AT_PLASMA };
    for (ammotype f_type : ftypes) {
        float st = strain() * 10;
        int amnt = (int) (basic_consumption(f_type) * (1.0 + st * st) / 100);
        if (!amnt) continue; // no engines of that type
        const bool elec = f_type == AT_BATT;
        bool found = false;
        for (int j = 0; j < (elec? 2 : 1); j++) {
            for (int p = 0; p < parts.size(); p++) {
                // if this is a fuel tank
                // and its type is same we're looking for now
                // and for electric engines:
                //  - if j is 0, then we're looking for plutonium (it's first)
                //  - otherwise we're looking for batteries (second)
                const auto& p_info = part_info(p);
                if (p_info.has_flag<vpf_fuel_tank>() &&
                    (p_info.fuel_type == (elec ? (j ? AT_BATT : AT_PLUT) : f_type))) {
                    parts[p].amount -= amnt;
                    if (parts[p].amount < 0) parts[p].amount = 0;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
}

void vehicle::thrust (int thd)
{
    if (velocity == 0) {
        turn_dir = face.dir();
        last_turn = 0;
        move = face;
        moves = 0;
        last_turn = 0;
        skidding = false;
    }

    if (!thd) return;

	auto g = game::active();

    const bool pl_ctrl = player_in_control(g->u);

    if (!valid_wheel_config() && velocity == 0) {
        if (pl_ctrl) messages.add("The %s don't have enough wheels to move!", name.c_str());
        return;
    }

    int sgn = velocity < 0? -1 : 1;
    bool thrusting = sgn == thd;

    if (thrusting) {
        if (total_power () < 1) {
            if (pl_ctrl) {
                if (total_power (false) < 1)
					messages.add("The %s don't have engine!", name.c_str());
                else
					messages.add("The %s's engine emits sneezing sound.", name.c_str());
            }
            cruise_velocity = 0;
            return;
        }

        consume_fuel ();

        int strn = (int) (cataclysm::square(strain()) * 100);

        for (int p = 0; p < parts.size(); p++) {
            const auto& p_info = part_info(p);
            if (   p_info.has_flag<vpf_engine>() && (fuel_left(p_info.fuel_type, true))
                && parts[p].hp > 0 && rng(1, 100) < strn) {
                int dmg = rng(strn * 2, strn * 4);
                damage_direct(p, dmg, damage_type::pierce);
            }
        }

        // add sound and smoke
		const point origin(screenPos());
        if (int smk = noise(true, true); 0 < smk) {
            point rd(origin + coord_translate(exhaust_d));
            g->m.add_field(g, rd, fd_smoke, (smk / 50) + 1);
        }
        g->sound(origin, noise(), "");
    }

    if (skidding) return;

    int accel = acceleration();
    int max_vel = max_velocity();
    int brake = 30 * k_mass();
    int brk = abs(velocity) * brake / mph_1;
    if (brk < accel) brk = accel;
    if (brk < 10 * mph_1) brk = 10 * mph_1;
    int vel_inc = (thrusting? accel : brk) * thd;
    if ((velocity > 0 && velocity + vel_inc < 0) ||
        (velocity < 0 && velocity + vel_inc > 0))
        stop ();
    else {
        velocity += vel_inc;
        if (velocity > max_vel)
            velocity = max_vel;
        else if (velocity < -max_vel / 4)
            velocity = -max_vel / 4;
    }
}

void vehicle::cruise_thrust (int amount)
{
    if (!amount) return;
    int max_vel = (safe_velocity() * 11 / (100 * mph_1) + 1) * (10 * mph_1);
    cruise_velocity += amount;
    cruise_velocity = cruise_velocity / abs(amount) * abs(amount);
    if (cruise_velocity > max_vel)
        cruise_velocity = max_vel;
    else
    if (-cruise_velocity > max_vel / 4)
        cruise_velocity = -max_vel / 4;
}

void vehicle::turn (int deg)
{
    if (deg == 0)
        return;
    if (velocity < 0)
        deg = -deg;
    last_turn = deg;
    turn_dir += deg;
    if (turn_dir < 0)
        turn_dir += 360;
    if (turn_dir >= 360)
        turn_dir -= 360;
}

tileray vehicle::physical_facing() const
{
    if (skidding) return move; // if skidding, it's the move vector
    if (turn_dir != face.dir()) return tileray(turn_dir); // driver turned vehicle, get turn_dir
    return face; // not turning, keep face.dir
}

void vehicle::physical_facing(const tileray& src)
{
    if (skidding) face.init(turn_dir);
    else face = src;
    move = src;
}

void vehicle::drive(int x, int y, const player& u)
{
    const constexpr int thr_amount = 10 * mph_1;
    if (cruise_on) cruise_thrust(-y * thr_amount);
    else thrust(-y);

    turn(15 * x);
    if (skidding && valid_wheel_config()) {
        if (rng(0, 40) < u.dex_cur + u.sklevel[sk_driving] * 2) {
            const bool pc = !u.is_npc();
            const bool vis_npc = u.is_npc() && game::active()->u_see(u.pos);
            if (pc) messages.add("You regain control of the %s.", name.c_str());
            else if (vis_npc) messages.add("%s regains control of the %s.", u.name.c_str(), name.c_str());
            skidding = false;
            move.init(turn_dir);
        }
    }
}

void vehicle::stop ()
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0;
    moves = 0;
}

/// <param name="u">The operator of the controls.</param>
void vehicle::handbrake(player& u)
{
    std::string you;
    std::string pull;
    decltype(game::active()->u_see(u.pos)) vis_loc = std::nullopt;
    const auto vis_npc = game::active()->u.see(u);
    if (vis_npc) {
        you = grammar::capitalize(u.desc(grammar::noun::role::subject));
        messages.add("%s %s a handbrake.", you.c_str(), u.regular_verb_agreement("pull").c_str());
    } else if (vis_loc = game::active()->u_see(u.pos)) {
        messages.add("Someone pulls a handbrake.");
    }
    cruise_velocity = 0;
    if (last_turn != 0 && rng(15, 60) * mph_1 < abs(velocity)) {
        skidding = true;
        if (vis_npc) {
            messages.add("%s loses control of %s.", you.c_str(), u.regular_verb_agreement("lose").c_str(), name.c_str());
        } else if (vis_loc = game::active()->u_see(u.pos)) {
            messages.add("Someone loses control of %s.", name.c_str());
        }
        turn(last_turn > 0 ? 60 : -60);
    } else if (velocity < 0) stop();
    else {
        velocity = velocity / 2 - 10 * mph_1;
        if (velocity < 0) stop();
    }
    u.moves = 0;
}

int vehicle::part_collision (int vx, int vy, int part, point dest)
{
	static constexpr const int mass_from_msize[mtype::MS_MAX] = { 15, 40, 80, 200, 800 };

	auto g = game::active();

    const bool pl_ctrl = player_in_control(g->u);
    // automatic collision w/NPCs until they can board \todo fix
	player* ph = g->survivor(dest);
    monster* const z = g->mon(dest);
    if (ph->in_vehicle) ph = nullptr;
    const auto v = g->m._veh_at(dest);
    vehicle* const oveh = v ? v->first : nullptr; // backward compatibility
    const bool veh_collision = oveh && oveh->GPSpos != GPSpos;
    bool body_collision = ph || z;

    // 0 - nothing, 1 - monster/player/npc, 2 - vehicle,
    // 3 - thin_obstacle, 4 - bashable, 5 - destructible, 6 - other
    int collision_type = 0;
    std::string obs_name(name_of(g->m.ter(dest)));

    int parm = part_with_feature (part, vpf_armor);
    if (parm < 0) parm = part;
    int dmg_mod = part_info(parm).dmg_mod;
    // let's calculate type of collision & mass of object we hit
    int mass = total_mass() / 8;
    int mass2;
    if (veh_collision)
    { // first, check if we collide with another vehicle (there shouldn't be impassable terrain below)
        collision_type = 2; // vehicle
        mass2 = oveh->total_mass() / 8;
        body_collision = false;
        obs_name = oveh->name.c_str();
    } else if (body_collision)
    { // then, check any monster/NPC/player on the way
        collision_type = 1; // body
		mass2 = mass_from_msize[z ? z->type->size : MS_MEDIUM];
    } else // if all above fails, go for terrain which might obstruct moving
    {
        const auto terrain = g->m.ter(dest);
        if (is<thin_obstacle>(terrain)) {
            collision_type = 3; // some fence
            mass2 = 20;
        } else if (is<bashable>(terrain)) {
            collision_type = 4; // bashable (door, window)
            mass2 = 50;    // special case: instead of calculating absorb based on mass of obstacle later, we let
                           // map::bash function deside, how much absorb is
        } else if (0 == move_cost_of(terrain)) {
            if (is_destructible(terrain)) {
                collision_type = 5; // destructible (wall)
                mass2 = 200;
            } else if (!is<swimmable>(terrain)) {
                collision_type = 6; // not destructible
                mass2 = 1000;
            }
        }
    }
    if (!collision_type) return 0;  // hit nothing

    int degree = rng (70, 100);
    int imp = abs(velocity) * mass / k_mvel ;
    int imp2 = imp * mass2 / (mass + mass2) * degree / 100;
    bool smashed = true;
    std::string snd;
    if (collision_type == 4 || collision_type == 2) // something bashable -- use map::bash to determine outcome
    {
        int absorb = -1;
        g->m.bash(dest, imp * dmg_mod / 100, snd, &absorb);
        if (absorb != -1) imp2 = absorb;
        smashed = imp * dmg_mod / 100 > absorb;
    } else if (collision_type >= 3) // some other terrain
    {
        smashed = imp * rng (80, 120) / 100 > mass2;
        if (smashed)
            switch (collision_type) // destroy obstacle
            {
            case 3:
                g->m.ter(dest) = t_dirt;
                break;
            case 5:
                g->m.ter(dest) = t_rubble;
                snd = "crash!";
                break;
            case 6:
                smashed = false;
                break;
            default:;
            }
        g->sound(dest, smashed? 80 : 50, "");
    }

    const auto& p_info = part_info(part);

    if (!body_collision) {
        if (pl_ctrl) {
            if (snd.length() > 0)
				messages.add("Your %s's %s rams into %s with a %s", name.c_str(), p_info.name, obs_name.c_str(), snd.c_str());
            else
				messages.add("Your %s's %s rams into %s.", name.c_str(), p_info.name, obs_name.c_str());
        } else if (snd.length() > 0)
			messages.add("You hear a %s", snd.c_str());
    }
    if (p_info.has_flag<vpf_sharp>() && smashed) imp2 /= 2;
    int imp1 = imp - imp2;
    int vel1 = imp1 * k_mvel / mass;
    int vel2 = imp2 * k_mvel / mass2;

    if (collision_type == 1) {
        int dam = imp1 * dmg_mod / 100;
        if (z) {
            int z_armor = p_info.has_flag<vpf_sharp>() ? z->type->armor_cut : z->type->armor_bash;
            if (z_armor < 0) z_armor = 0;
            if (z) dam -= z_armor;
        }
        if (dam < 0) dam = 0;

        if (p_info.has_flag<vpf_sharp>())
            parts[part].blood += (20 + dam) * 5;
        else if (dam > rng (10, 30))
            parts[part].blood += (10 + dam / 2) * 5;

        int turns_stunned = rng (0, dam) > 10? rng (1, 2) + (dam > 40? rng (1, 2) : 0) : 0;
        if (p_info.has_flag<vpf_sharp>()) turns_stunned = 0;
        if (turns_stunned > 6) turns_stunned = 6;
        if (turns_stunned > 0 && z) z->add_effect(ME_STUNNED, turns_stunned);

        std::string dname(z ? z->name().c_str() : ph->name);
        if (pl_ctrl)
			messages.add("Your %s's %s rams into %s, inflicting %d damage%s!",
                    name.c_str(), p_info.name, dname.c_str(), dam,
                    turns_stunned > 0 && z? " and stunning it" : "");

        int angle = (100 - degree) * 2 * (one_in(2)? 1 : -1);
        if (z) {
            z->hurt(dam);
            if (vel2 > rng (5, 30))
                g->fling_player_or_monster(nullptr, z, move.dir() + angle, vel2 / 100);
            if (z->hp < 1) g->kill_mon(*z, pl_ctrl ? &g->u : nullptr);
        } else {
            ph->hitall (g, dam, 40);
            if (vel2 > rng (5, 30))
                g->fling_player_or_monster(ph, nullptr, move.dir() + angle, vel2 / 100);
        }

        if (p_info.has_flag<vpf_sharp>()) {
            if (const auto blood = bleeds(z)) {
                auto& f = g->m.field_at(dest);
                if (f.type == blood) {
                    if (f.density < 3) f.density++;
                }
                else g->m.add_field(g, dest, blood, 1);
            }
        } else
            g->sound(dest, 20, "");
    }

    if (!smashed || collision_type == 2) { // vehicles shouldn't intersect
        cruise_on = false;
        stop();
        imp2 = imp;
    } else {
        if (vel1 < 5 * mph_1) stop();
        else velocity = (velocity < 0) ? -vel1 : vel1;

        int turn_roll = rng (0, 100);
        int turn_amount = rng (1, 3) * sqrt (imp2);
        turn_amount /= 15;
        if (turn_amount < 1) turn_amount = 1;
        turn_amount *= 15;
        if (turn_amount > 120) turn_amount = 120;
        bool turn_veh = turn_roll < (abs(velocity) - vel1) / mph_1;
        if (turn_veh) {
            skidding = true;
            turn (one_in (2)? turn_amount : -turn_amount);
        }

    }
    damage(parm, imp2);
    return imp2;
}

void vehicle::handle_trap(const point& pt, int part)
{
    int pwh = part_with_feature (part, vpf_wheel);
    if (pwh < 0) return;
	auto g = game::active();
    trap_id& tr = g->m.tr_at(pt);
    if (tr_null == tr) return;
    const std::string& tr_name = trap::traps[tr]->name; // message must refer to before-update trap
    int noise = 0;
    int chance = 100;
    int expl = 0;
    int shrap = 0;
    trap_id triggered = tr_null;
    bool wreckit = false;
    const char* msg = "The %s's %s runs over %s.";
    std::string snd;
    switch (tr)
    {
        case tr_bubblewrap:
            noise = 18;
            snd = "Pop!";
            break;
        case tr_beartrap:
        case tr_beartrap_buried:
            noise = 8;
            snd = "SNAP!";
            wreckit = true;
            // the two beartraps have the same configuration
            trap_fully_triggered(g->m, pt, trap::traps[tr_beartrap]->trigger_components);
            break;
        case tr_nailboard:
            wreckit = true;
            break;
        case tr_blade:
            noise = 1;
            snd = "Swinnng!";
            wreckit = true;
            break;
        case tr_crossbow:
            chance = 30;
            noise = 1;
            snd = "Clank!";
            wreckit = true;
            trap_fully_triggered(g->m, pt, trap::traps[tr_crossbow]->trigger_components);
            if (!one_in(10)) g->m.add_item(pt, item::types[itm_bolt_steel], 0);
            break;
        case tr_shotgun_2:
        case tr_shotgun_1:
            noise = 60;
            snd = "Bang!";
            chance = 70;
            wreckit = true;
            if (tr_shotgun_2 == tr) tr = tr_shotgun_1;
            else trap_fully_triggered(g->m, pt, trap::traps[tr_shotgun_2]->trigger_components); // the two shotguns have the same configuration
            break;
        case tr_landmine:
            expl = 10;
            shrap = 8;
            break;
        case tr_boobytrap:
            expl = 18;
            shrap = 12;
            break;
        case tr_dissector:
            noise = 10;
            snd = "BRZZZAP!";
            wreckit = true;
            break;
        case tr_spike_pit:
            if (one_in(4)) triggered = tr_spike_pit;
            [[fallthrough]];
        case tr_sinkhole:
        case tr_pit:
        case tr_ledge:
            wreckit = true;
            break;
        case tr_goo:
        case tr_portal:
        case tr_telepad:
        case tr_temple_flood:
        case tr_temple_toggle:
            msg = nullptr;
    }
    if (msg && g->u_see(pt))
		messages.add(msg, name.c_str(), part_info(part).name, tr_name.c_str());
    if (noise > 0) g->sound(pt, noise, snd);
    if (wreckit && chance >= rng (1, 100)) damage(part, 500);
    if (triggered) {
        if (g->u_see(pt)) messages.add("The spears break!"); // hard-code the spiked pit trap, for now
        trap_fully_triggered(g->m, pt, trap::traps[triggered]->trigger_components);
        // hard-code overriding trap_at effects, above
        g->m.ter(pt) = t_pit;
        g->m.tr_at(pt) = tr_pit;
    }
    if (expl > 0) g->explosion(pt, expl, shrap, false);
}

bool vehicle::add_item (int part, item itm)
{
    const auto& p_info = part_info(part);
    if (!p_info.has_flag<vpf_cargo>() || parts[part].items.size() >= 26) return false;
	if (p_info.has_flag<vpf_turret>()) {
		const it_ammo* const ammo = dynamic_cast<const it_ammo*>(itm.type);
		if (!ammo || (ammo->type != p_info.fuel_type ||
			ammo->type == AT_GAS ||
			ammo->type == AT_PLASMA))
			return false;
	}
    parts[part].items.push_back (itm);
    return true;
}

void vehicle::remove_item (int part, int itemdex)
{
    if (itemdex < 0 || itemdex >= parts[part].items.size()) return;
    EraseAt(parts[part].items, itemdex);
}

void vehicle::gain_moves (int mp)
{
    moves += mp;
	auto g = game::active();
    // cruise control TODO: enable for NPC?
    if (player_in_control(g->u)) {
        if (cruise_on)
        if (abs(cruise_velocity - velocity) >= acceleration()/2 ||
            (cruise_velocity != 0 && velocity == 0) ||
            (cruise_velocity == 0 && velocity != 0))
            thrust (cruise_velocity > velocity? 1 : -1);
    }

    if (g->is_in_sunlight(GPSpos)) {
        if (const int spw = solar_power()) {
            int fl = spw / 100;
            if (rng (0, 100) <= (spw % 100)) fl++;
            if (fl) refill (AT_BATT, fl);
        }
    }

    // check for smoking parts
    const point anchor(screenPos());
    for (const int p : external_parts) {
        if (parts[p].blood > 0) parts[p].blood--;
        int p_eng = part_with_feature(p, vpf_engine, false);
        if (p_eng < 0 || parts[p_eng].hp > 0 || parts[p_eng].amount < 1) continue;
        parts[p_eng].amount--;
        const point origin(anchor + parts[p_eng].precalc_d[0]);
        for (int ix = -1; ix <= 1; ix++)
            for (int iy = -1; iy <= 1; iy++)
                if (!rng(0, 2)) g->m.add_field(g, origin.x + ix, origin.y + iy, fd_smoke, rng(2, 4));
    }

    if (turret_mode) // handle turrets
        for (int p = 0; p < parts.size(); p++)
            fire_turret (p);
}

void vehicle::find_external_parts ()
{
    external_parts.clear();
    for (int p = 0; p < parts.size(); p++)
    {
        bool ex = false;
        for (int i = 0; i < external_parts.size(); i++)
            if (parts[external_parts[i]].mount_d == parts[p].mount_d)
            {
                ex = true;
                break;
            }
        if (!ex) external_parts.push_back (p);
    }
}

void vehicle::find_exhaust ()
{
    int en = -1;
    for (int p = 0; p < parts.size(); p++) {
        const auto& p_info = part_info(p);
        if (p_info.has_flag<vpf_engine>() && p_info.fuel_type == AT_GAS) {
            en = p;
            break;
        }
    }
    if (en < 0) {
		exhaust_d = point(0, 0);
        return;
    }
    exhaust_d = parts[en].mount_d;
    for (int p = 0; p < parts.size(); p++)
        if (parts[p].mount_d.y == exhaust_d.y && parts[p].mount_d.x < exhaust_d.x)
            exhaust_d.x = parts[p].mount_d.x;
    exhaust_d.x--;
}

void vehicle::refresh_insides() const
{
    insides_dirty = false;
    for(const int p : external_parts) {
        if (part_with_feature(p, vpf_roof) < 0 || parts[p].hp <= 0) { // if there's no roof (or it's broken) -- it's outside!
            parts[p].inside = false;
            continue;
        }

        parts[p].inside = true; // inside if not otherwise
        for (int i = 0; i < 4; i++) { // let's check four neighbour parts
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for (const int pn : parts_at_relative(parts[p].mount_d.x + ndx, parts[p].mount_d.y + ndy)) {
                if (parts[pn].hp <= 0) continue;   // it's broken = can't cover
                const auto& pn_info = part_info(pn);
                if (pn_info.has_flag<vpf_roof>()) { // another roof -- cover
                    cover = true;
                    break;
                } else if (pn_info.has_flag<vpf_obstacle>()) { // found an obstacle, like board or windshield or door
                    if (pn_info.has_flag<vpf_openable>() && parts[pn].open) continue; // door and it's open -- can't cover
                    cover = true;
                    break;
                }
            }
            if (!cover) {
                parts[p].inside = false;
                break;
            }
        }
    }
}

bool vehicle::is_inside(int p) const
{
    if (p < 0 || p >= parts.size()) return false;
    if (insides_dirty) refresh_insides ();
    return parts[p].inside;
}

void vehicle::unboard(int part)
{
    int seat_part = part_with_feature(part, vpf_seat, false); // \todo what about minivans?
    if (0 > seat_part) {
        debuglog("vehicle::unboard: unboarding %s (not seat)", part_info(part).name);
        return;
    }
    // relies on passenger position continually being in sync
    // does *NOT* try to reposition outside of vehicle
    // \todo? change target for vehicle-relative coordinates for player when boarded
    if (player* psg = get_passenger(seat_part)) {
        psg->in_vehicle = false;
        psg->driving_recoil = 0;
        parts[seat_part].passenger = 0;
        skidding = true;
        return;
    }
    debuglog("vehicle::unboard: passenger not found");
}

void vehicle::unboard_all ()
{
    for (int part : boarded_parts()) unboard(part);
}

int vehicle::damage(int p, int dmg, damage_type type, bool aimed)
{
    if (dmg < 1) return dmg;

    std::vector<int> pl = internal_parts (p);
    pl.insert (pl.begin(), p);
    if (!aimed) {
        bool found_obs = false;
        for (int i = 0; i < pl.size(); i++) {
            const auto& pl_i_info = part_info(pl[i]);
            if (    pl_i_info.has_flag<vpf_obstacle>()
                && (!pl_i_info.has_flag<vpf_openable>() || !parts[pl[i]].open)) {
                found_obs = true;
                break;
            }
        }
        if (!found_obs) return dmg; // not aimed at this tile and no obstacle here -- fly through
    }
    int parm = part_with_feature (p, vpf_armor);
    int pdm = pl[rng (0, pl.size()-1)];
    int dres;
    if (parm < 0) dres = damage_direct (pdm, dmg, type); // not covered by armor -- damage part
    else { // covered by armor -- damage armor first
        dres = damage_direct (parm, dmg, type);
        // half damage for internal part(over parts not covered)
        damage_direct (pdm, part_flag(pdm, vpf_over)? dmg : dmg / 2, type);
    }
    return dres;
}

void vehicle::damage_all(int dmg1, int dmg2, damage_type type)
{
    if (dmg2 < dmg1)
    {
        int t = dmg2;
        dmg2 = dmg1;
        dmg1 = t;
    }
    if (dmg1 < 1)
        return;
    for (int p = 0; p < parts.size(); p++)
        if (!one_in(4))
            damage_direct (p, rng (dmg1, dmg2), type);
}

int vehicle::damage_direct(int p, int dmg, damage_type type)
{
    if (parts[p].hp <= 0) return dmg;
    const auto& p_info = part_info(p);
    int dres = dmg;
    if (damage_type::bash != type || clamped_ub<20>(p_info.durability / 10) <= dmg) {
        dres -= parts[p].hp;
        int last_hp = parts[p].hp;
        parts[p].hp -= dmg;
        if (parts[p].hp < 0) parts[p].hp = 0;
        if (!parts[p].hp && last_hp > 0) insides_dirty = true;
		auto g = game::active();
        if (p_info.has_flag<vpf_fuel_tank>()) {
            const auto ft = p_info.fuel_type;
            if (ft == AT_GAS || ft == AT_PLASMA) {
                int pow = parts[p].amount / 40;
                if (parts[p].hp <= 0) leak_fuel (p);
                if (damage_type::incendiary == type ||
                    (one_in (ft == AT_GAS? 2 : 4) && pow > 5 && rng (75, 150) < dmg)) {
                    g->explosion(screenPos() + parts[p].precalc_d[0], pow, 0, ft == AT_GAS);
                    parts[p].hp = 0;
                }
            }
        } else if (parts[p].hp <= 0 && p_info.has_flag<vpf_unmount_on_damage>()) {
			const point dest(screenPos() + parts[p].precalc_d[0]);
            g->m.add_item(dest, item::types[p_info.item], messages.turn);
            remove_part(p);
        }
    }
    if (dres < 0) dres = 0;
    return dres;
}

void vehicle::leak_fuel (int p)
{
    const auto& p_info = part_info(p);
    if (!p_info.has_flag<vpf_fuel_tank>()) return;
    if (AT_GAS == p_info.fuel_type) {
		const point origin(screenPos());
		auto g = game::active();
        for (int i = origin.x - 2; i <= origin.x + 2; i++)
            for (int j = origin.y - 2; j <= origin.y + 2; j++)
                if (g->m.move_cost(i, j) > 0 && one_in(2)) {
                    if (parts[p].amount < 100) {
                        parts[p].amount = 0;
                        return;
                    }
                    g->m.add_item(i, j, item::types[itm_gasoline], 0);
                    parts[p].amount -= 100;
                }
    }
    parts[p].amount = 0;
}

bool vehicle::refill(player& u, const int part, const bool test)
{
    const auto& p_info = part_info(part);
    if (!p_info.has_flag<vpf_fuel_tank>()) return false;
	const auto ftype = p_info.fuel_type;

    int i_itm = -1;
    item* p_itm = nullptr;
    int min_charges = -1;
    bool i_cont = false;

    itype_id itid = default_ammo(ftype);
    if (u.weapon.is_container() && !u.weapon.contents.empty() && u.weapon.contents[0].type->id == itid) {
        i_itm = -2;
        p_itm = &u.weapon.contents[0];
        min_charges = u.weapon.contents[0].charges;
        i_cont = true;
    } else if (u.weapon.type->id == itid) {
        i_itm = -2;
        p_itm = &u.weapon;
        min_charges = u.weapon.charges;
    } else
     for (size_t i = 0; i < u.inv.size(); i++) {
        item *itm = &u.inv[i];
        bool cont = false;
        if (itm->is_container() && !itm->contents.empty()) {
            cont = true;
            itm = &(itm->contents[0]);
        }
        if (itm->type->id != itid) continue;
        if (i_itm < 0 || min_charges > itm->charges) {
            i_itm = i;
            p_itm = itm;
            i_cont = cont;
            min_charges = itm->charges;
        }
     }

    if (i_itm == -1) return false;
    else if (test) return true;

    int fuel_per_charge = 1;
    switch (ftype)
    {
    case AT_PLUT:
        fuel_per_charge = 1000;
        break;
    case AT_PLASMA:
        fuel_per_charge = 100;
        break;
    default:;
    }

	const int max_fuel = p_info._size;
	auto& v_part = parts[part];
    int dch = (max_fuel - v_part.amount) / fuel_per_charge;
    if (dch < 1) dch = 1;
    const bool rem_itm = min_charges <= dch;
    const int used_charges = rem_itm? min_charges : dch;
    if (max_fuel > (v_part.amount += used_charges * fuel_per_charge)) v_part.amount = max_fuel;

	messages.add("You %s %s's %s%s.", ftype == AT_BATT? "recharge" : "refill", name.c_str(),	// XXX \todo augment to handle NPCs
             ftype == AT_BATT? "battery" : (ftype == AT_PLUT? "reactor" : "fuel tank"),
             v_part.amount == max_fuel? " to its maximum" : "");

    p_itm->charges -= used_charges;
    if (rem_itm) {
        if (i_itm == -2) {
            if (i_cont)
                EraseAt(u.weapon.contents, 0);
            else
                u.remove_weapon ();
        } else {
            if (i_cont)
                EraseAt(u.inv[i_itm].contents, 0);
            else
                u.inv.remove_item (i_itm);
        }
    }
	return true;
}

void vehicle::fire_turret (int p, bool burst)
{
    const auto& p_info = part_info(p);
    if (!p_info.has_flag<vpf_turret>()) return;
    // at this point we know p is "in bounds"
    it_gun *gun = dynamic_cast<it_gun*> (item::types[p_info.item]);
    if (!gun) return;
    int charges = burst? gun->burst : 1;
    if (!charges) charges = 1;
    vehicle_part& turret = parts[p];
    const auto amt = p_info.fuel_type;
    if (amt == AT_GAS || amt == AT_PLASMA) {
        if (1 > fuel_left(amt)) return;
        const it_ammo* const ammo = dynamic_cast<it_ammo*>(item::types[amt == AT_GAS ? itm_gasoline : itm_plasma]);
        if (!ammo) return;
        if (amt == AT_GAS) charges = 20; // hacky
        if (fire_turret_internal(turret, *gun, *ammo, charges))
        { // consume fuel
            if (amt == AT_PLASMA) charges *= 10; // hacky, too
            for (int p1 = 0; p1 < parts.size(); p1++) {
                const auto& p1_info = part_info(p1);
                if (p1_info.has_flag<vpf_fuel_tank>() && p1_info.fuel_type == amt && 0 < parts[p1].amount) {
                    parts[p1].amount -= charges;
                    if (parts[p1].amount < 0) parts[p1].amount = 0;
                }
            }
        }
    } else if (!turret.items.empty()) {
        decltype(auto) ammo_src = turret.items.front();
        if (1 > ammo_src.charges) return;
        const it_ammo* const ammo = dynamic_cast<const it_ammo*>(ammo_src.type);
        if (!ammo || ammo->type != amt) return;
        clamp_ub(charges, ammo_src.charges);
        if (fire_turret_internal(turret, *gun, *ammo, charges)) { // consume ammo
            if (charges >= ammo_src.charges)
                EraseAt(turret.items, 0); // ammo_src invalidated
            else
                ammo_src.charges -= charges;
        }
    }
}

bool vehicle::fire_turret_internal(const vehicle_part& p, it_gun &gun, const it_ammo &ammo, int charges)
{
    const auto origin_GPS(GPSpos + p.precalc_d[0]);
    player* driving = driver();
    // code copied form mattack::smg, mattack::flamethrower
    int fire_t;
    point target_pos;
    monster* target = nullptr;
    const int range = ammo.type == AT_GAS? 5 : 12;
    int closest = range + 1;
	auto g = game::active();
    const point origin(screenPos() + p.precalc_d[0]);
    // use driver (eventually, ai who usually is friendly to driver) as definer of is_enemy status
	for(auto& _mon : g->z) {
        if (!_mon.is_enemy(driving)) continue;
        if (const int dist = rl_dist(origin_GPS, _mon.GPSpos); dist < closest) {
            if (const auto pos = _mon.screen_pos()) {
                if (const auto t = g->m.sees(origin, *pos, range)) {
                    target = &_mon;
                    closest = dist;
                    fire_t = *t;
                    target_pos = *pos;
                }
            }
        }
    }
    // \todo allow targeting hostile NPCs https://github.com/zaimoni/Cataclysm/issues/108
    if (!target) return false;

    std::vector<point> traj = line_to(origin, target_pos, fire_t);
    for (int i = 0; i < traj.size(); i++)
        if (traj[i] == g->u.pos) return false; // won't shoot at player
    if (g->u_see(origin)) messages.add("The %s fires its %s!", name.c_str(), p.info().name);
    player tmp(player::get_proxy(std::string("The ") + p.info().name, origin, gun, abs(velocity) / (4 * mph_1), charges));
    g->fire(tmp, target_pos, traj, true);
    if (ammo.type == AT_GAS) {
		for(const auto& pt : traj) g->m.add_field(g, pt, fd_fire, 1);
    }
	return true;
}
