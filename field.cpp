#include "rng.h"
#include "game.h"
#include "submap.h"
#include "line.h"
#include "stl_limits.h"
#include "recent_msg.h"
#include "inline_stack.hpp"

static const char* JSON_transcode[] = {
	"blood",
	"bile",
	"web",
	"slime",
	"acid",
	"sap",
	"fire",
	"smoke",
	"toxic_gas",
	"tear_gas",
	"nuke_gas",
	"gas_vent",
	"fire_vent",
	"flame_burst",
	"electricity",
	"fatigue",
	"push_items",
	"shock_vent",
	"acid_vent"
};

DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(field_id, JSON_transcode)

bool map::process_fields()
{
 bool found_field = false;
 for (int x = 0; x < my_MAPSIZE; x++) {
  for (int y = 0; y < my_MAPSIZE; y++) {
      found_field |= grid[x + y * my_MAPSIZE]->process_fields();
  }
 }
 return found_field;
}

static void clear_nearby_scent(GPS_loc loc)
{
    const auto g = game::active();
    auto pos = g->toScreen(loc);
    if (pos) g->scent(*pos) = 0;
    for (decltype(auto) dir : Direction::vector) {
        auto pos = g->toScreen(loc+dir);
        if (pos) g->scent(*pos) = 0;
    }
}

bool submap::process_fields()
{
    if (0 >= field_count) return false;
    bool found_field = false;
    const auto g = game::active();
    for (int locx = 0; locx < SEEX; locx++) {
        for (int locy = 0; locy < SEEY; locy++) {
            field& cur = fld[locx][locy];
            GPS_loc loc(GPS, point(locx, locy));
            const auto pt = g->toScreen(loc);

            const field_id curtype = cur.type;
            if (curtype != fd_null) found_field = true;
            // \todo ideally would use accessors and preclude this by construction
            if (3 < cur.density || 1 > cur.density) {
                if (const char* const json = JSON_key(curtype)) {
                    debuglog("Whoooooa density of %d for %s", cur.density, json);
                } else {
                    debuglog("Whoooooa density of %d", cur.density);
                }
                // repair it
                if (3 < cur.density) cur.density = 3;
                else cur.density = 1;
            }

            if (0 == cur.age) continue; // Don't process "newborn" fields

            switch (curtype) {
            case fd_null: break;	// Do nothing, obviously.  OBVIOUSLY.

            case fd_blood:
            case fd_bile:
                if (is<swimmable>(loc.ter())) cur.age += 250;	// Dissipate faster in water
                break;

            case fd_acid:
                if (is<swimmable>(loc.ter())) cur.age += 20;	// Dissipate faster in water
                {
                    auto& stack = loc.items_at();
                    int i = stack.size();
                    while (0 <= --i) {
                        item& melting = stack[i];
                        if (melting.made_of(LIQUID) || melting.made_of(VEGGY) ||
                            melting.made_of(FLESH) || melting.made_of(POWDER) ||
                            melting.made_of(COTTON) || melting.made_of(WOOL) ||
                            melting.made_of(PAPER) || melting.made_of(PLASTIC) ||
                            (melting.made_of(GLASS) && !one_in(3)) || one_in(4)) {
                            // Acid destructible objects here
                            melting.damage++;
                            if (5 <= melting.damage || (melting.made_of(PAPER) && 3 <= melting.damage)) {
                                cur.age += melting.volume();
                                decltype(melting.contents) stage;
                                stage.swap(melting.contents);
                                EraseAt(stack, i);
                                if (auto ub = stage.size()) {
                                    stack.reserve(stack.size() + ub);
                                    for (decltype(auto) it : stage) stack.push_back(std::move(it));
                                }
                            }
                        }
                    }
                }
                break;

            case fd_sap: break; // It doesn't do anything.

            case fd_fire: {
                // Consume items as fuel to help us grow/last longer.
                int smoke = 0, consumed = 0;
                auto& stack = loc.items_at();
                int i = stack.size();
                while (0 <= --i && consumed < cur.density * 2) {
                    bool destroyed = false;
                    item& it = stack[i];
                    const int vol = it.volume();

                    // firearms ammo cooks easily in B movies
                    if (ammotype am = it.provides_ammo_type(); am && am != AT_BATT && am != AT_NAIL && am != AT_BB && am != AT_BOLT && am != AT_ARROW) {
                        // as charger rounds are not typically found in naked ground inventories, their new (2020-04-10) inability to burn should be a non-issue
                        cur.age /= 2;
                        cur.age -= 600;
                        destroyed = true;
                        smoke += 6;
                        consumed++;
                    } else if (it.made_of(PAPER)) {
                        destroyed = it.burn(cur.density * 3);
                        consumed++;
                        if (1 == cur.density) cur.age -= vol * 10;
                        if (4 <= vol) smoke++;
                    } else if ((it.made_of(WOOD) || it.made_of(VEGGY))) {
                        if (vol <= cur.density * 10 || 3 == cur.density) {
                            cur.age -= 4;
                            destroyed = it.burn(cur.density);
                            smoke++;
                            consumed++;
                        } else if (it.burnt < cur.density) {
                            destroyed = it.burn(1);
                            smoke++;
                        }
                    } else if ((it.made_of(COTTON) || it.made_of(WOOL))) {
                        if (vol <= cur.density * 5 || 3 == cur.density) {
                            cur.age--;
                            destroyed = it.burn(cur.density);
                            smoke++;
                            consumed++;
                        } else if (it.burnt < cur.density) {
                            destroyed = it.burn(1);
                            smoke++;
                        }
                    } else if (it.made_of(FLESH)) {
                        if (vol <= cur.density * 5 || (3 == cur.density && one_in(vol / 20))) {
                            cur.age--;
                            destroyed = it.burn(cur.density);
                            smoke += 3;
                            consumed++;
                        } else if (it.burnt < cur.density * 5 || 2 <= cur.density) {
                            destroyed = it.burn(1);
                            smoke++;
                        }
                    } else if (it.made_of(LIQUID)) {
                        if (is_hard_liquor(it.type->id)) { // TODO: Make this be not a hack.
                            cur.age -= 300;
                            smoke += 6;
                        } else {
                            cur.age += rng(80 * vol, 300 * vol);
                            smoke++;
                        }
                        destroyed = true;
                        consumed++;
                    } else if (it.made_of(POWDER)) {
                        cur.age -= vol;
                        destroyed = true;
                        smoke += 2;
                    } else if (it.made_of(PLASTIC)) {
                        smoke += 3;
                        if (it.burnt <= cur.density * 2 || (3 == cur.density && one_in(vol))) {
                            destroyed = it.burn(cur.density);
                            if (one_in(vol + it.burnt)) cur.age--;
                        }
                    }

                    if (destroyed) {
                        decltype(it.contents) stage;
                        stage.swap(it.contents);
                        EraseAt(stack, i);
                        if (auto ub = stage.size()) {
                            stack.reserve(stack.size() + ub);
                            for (decltype(auto) obj : stage) stack.push_back(std::move(obj));
                        }
                    }
                }

                if (const auto veh = loc.veh_at()) veh->first->damage(veh->second, cur.density * 10, vehicle::damage_type::incendiary);
                // Consume the terrain we're on
                auto& terrain = loc.ter();
                if (is<explodes>(terrain)) {
                    terrain = ter_id(int(terrain) + 1);
                    cur.age = 0;
                    cur.density = 3;
                    loc.explosion(40, 0, true);
                } else if (is<flammable>(terrain) && one_in(32 - cur.density * 10)) {
                    cur.age -= cur.density * cur.density * 40;
                    smoke += 15;
                    if (3 == cur.density) terrain = t_rubble;
                } else if (is<l_flammable>(terrain) && one_in(62 - cur.density * 10)) {
                    cur.age -= cur.density * cur.density * 30;
                    smoke += 10;
                    if (cur.density == 3) terrain = t_rubble;
                } else if (is<swimmable>(terrain))
                    cur.age += 800;	// Flames die quickly on water

               // If we consumed a lot, the flames grow higher
                while (3 > cur.density && 0 > cur.age) {
                    cur.age += 300;
                    cur.density++;
                }
                // If the flames are in a pit, it can't spread to non-pit
                const bool in_pit = (t_pit == terrain);
                // If the flames are REALLY big, they contribute to adjacent flames
                if (3 == cur.density && 0 > cur.age) {
                    inline_stack<field*, std::end(Direction::vector) - std::begin(Direction::vector)> stage;
                    for (decltype(auto) dir : Direction::vector) {
                        auto dest = loc + dir;
                        auto& fd = dest.field_at();
                        if (fd_fire == fd.type && 3 > fd.density && (!in_pit || t_pit == dest.ter())) stage.push(&fd);
                    }
                    if (auto ub = stage.size()) {
                        auto& fd = *stage[rng(0, ub - 1)];
                        fd.density++;
                        fd.age = 0;
                        cur.age = 0;
                    }
                }

                // Consume adjacent fuel / terrain / webs to spread.
                inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector) + 1> exploding;
                inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector) + 1> spreading;
                inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector) + 1> smoking;

                static auto decide_behavior = [&](point pt) {
                    auto dest = loc + pt;
                    if (auto pos = game::active()->toScreen(dest)) { // \todo disconnect this test, no longer a data integrity issue
                        int spread_chance = 20 * (cur.density - 1) + 10 * smoke;
                        auto& f = loc.field_at();
                        if (f.type == fd_web) spread_chance = 50 + spread_chance / 2;
                        auto& t = loc.ter();
                        if (is<explodes>(t) && one_in(8 - cur.density)) {
                            exploding.push(dest);
                        } else if ((0 != pos->x || 0 != pos->y) && rng(1, 100) < spread_chance &&
                            (!in_pit || t_pit == t) &&
                            ((3 == cur.density && (is<flammable>(t) || one_in(20))) ||
                                (3 == cur.density && (is<l_flammable>(t) && one_in(10))) ||
                                contains_ignitable(loc.items_at()) ||
                                f.type == fd_web)) {
                            spreading.push(dest);
                        } else {
                            smoking.push(dest);
                        }
                    }
                };

                forall_do_inclusive(within_rldist<1>, decide_behavior);

                int ub = smoking.size();
                while (0 <= --ub) {
                    bool nosmoke = true;

                    static auto find_smoke = [&](point pt) {
                        auto& f = (loc + pt).field_at();
                        if (f.type == fd_fire && f.density == 3) smoke++;
                        else if (f.type == fd_smoke) nosmoke = false;
                    };

                    // \todo should be allowed to have smoke over water
                    // If we're not spreading, maybe we'll stick out some smoke, huh?
                    if (0 < smoking[ub].move_cost() &&
                        (!one_in(smoke) || (nosmoke && one_in(40))) &&
                        rng(3, 35) < cur.density * 10 && 1000 > cur.age) {  // \todo? does this age cutoff make sense
                        smoke--;
                        smoking[ub].add(field(fd_smoke, rng(1, cur.density))); // C:Whales: can self-extinguish!
                    }
                }

                ub = spreading.size();
                while (0 <= --ub) {
                    spreading[ub].add(field(fd_fire));
                };

                ub = exploding.size();
                while (0 <= --ub) {
                    auto& t = exploding[ub].ter();
                    t = ter_id(t + 1);
                    exploding[ub].explosion(40, 0, true);
                };
            }
            break;

            // \todo The smoke/gas fields need to be split out from the main implementation so they can mix properly
            case fd_smoke:
                clear_nearby_scent(loc);
                if (loc.is_outside()) cur.age += 50;
                if (0 < cur.age && one_in(2)) {
                    inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> spread;

                    // Pick all eligible points to spread to
                    for (decltype(auto) dir : Direction::vector) {
                        auto dest = loc + dir;
                        auto& f = dest.field_at();
                        if ((fd_smoke == f.type && 3 > f.density) ||
                            (f.is_null() && 0 < dest.move_cost()))
                            spread.push(dest);
                    }

                    if (const auto ub = spread.size()) {
                        auto dest = spread[rng(0, spread.size() - 1)];
                        auto& f = dest.field_at();
                        if (fd_smoke == f.type) {
                            f.density++;
                            cur.density--;
                        } else if (dest.add(field(fd_smoke, 1, cur.age))) cur.density--;
                    }
                }
                break;

            case fd_tear_gas:
                clear_nearby_scent(loc);
                if (loc.is_outside()) cur.age += 30;
                // One in three chance that it spreads (less than smoke!)
                if (0 < cur.age && one_in(3)) {
                    inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> spread;

                    // Pick all eligible points to spread to
                    for (decltype(auto) dir : Direction::vector) {
                        auto dest = loc + dir;
                        auto& f = dest.field_at();
                        if (((fd_smoke == f.type || fd_tear_gas == f.type) && f.density < 3) ||
                            (f.is_null() && 0 < dest.move_cost()))
                            spread.push(dest);
                    }

                    // Then, spread to a nearby point
                    if (const auto ub = spread.size()) {
                        auto dest = spread[rng(0, spread.size() - 1)];
                        auto& f = dest.field_at();
                        switch (f.type) {
                        case fd_tear_gas:	// Nearby teargas grows thicker
                            f.density++;
                            cur.density--;
                            break;
                        case fd_smoke:	// Nearby smoke is converted into teargas
                            f.type = fd_tear_gas;
                            break;
                        default:	// Or, just create a new field.
                            if (dest.add(field(fd_tear_gas, 1, cur.age))) cur.density--;
                        }
                    }
                }
                break;

            case fd_toxic_gas:
                clear_nearby_scent(loc);
                if (loc.is_outside()) cur.age += 40;
                if (0 < cur.age && one_in(2)) {
                    inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> spread;

                    // Pick all eligible points to spread to
                    for (decltype(auto) dir : Direction::vector) {
                        auto dest = loc + dir;
                        auto& f = dest.field_at();
                        if (((fd_smoke == f.type || fd_tear_gas == f.type || fd_toxic_gas == f.type) && f.density < 3) ||
                            (f.is_null() && 0 < dest.move_cost()))
                            spread.push(dest);
                    }

                    // Then, spread to a nearby point
                    if (!spread.empty()) {
                        auto dest = spread[rng(0, spread.size() - 1)];
                        auto& f = dest.field_at();
                        switch (f.type) {
                        case fd_toxic_gas:	// Nearby toxic gas grows thicker
                            f.density++;
                            cur.density--;
                            break;
                        case fd_smoke:	// Nearby smoke & teargas is converted into toxic gas
                        case fd_tear_gas:
                            f.type = fd_toxic_gas;
                            break;
                        default:	// Or, just create a new field.
                            if (dest.add(field(fd_toxic_gas, 1, cur.age))) cur.density--;
                        }
                    }
                }
                break;

            case fd_nuke_gas:
                clear_nearby_scent(loc);
                if (loc.is_outside()) cur.age += 40;
                // Increase long-term radiation in the land underneath
                rad[locx][locy] += rng(0, cur.density);
                if (0 < cur.age && one_in(2)) {
                    inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> spread;

                    // Pick all eligible points to spread to
                    for (decltype(auto) dir : Direction::vector) {
                        auto dest = loc + dir;
                        auto& f = dest.field_at();
                        if (((fd_smoke == f.type || fd_tear_gas == f.type || fd_toxic_gas == f.type || fd_nuke_gas == f.type) && 3 > f.density) ||
                            (f.is_null() && 0 < dest.move_cost()))
                            spread.push(dest);
                    }

                    // Then, spread to a nearby point
                    if (!spread.empty()) {
                        auto dest = spread[rng(0, spread.size() - 1)];
                        auto& f = dest.field_at();
                        switch (f.type) {
                        case fd_nuke_gas:	// Nearby nukegas grows thicker
                            if (f.density < 3) {
                                f.density++;
                                cur.density--;
                            } break;
                        case fd_smoke:	// Nearby smoke, tear, and toxic gas is converted into nukegas
                        case fd_toxic_gas:
                        case fd_tear_gas:
                            f.type = fd_nuke_gas;
                            break;
                        default:	// Or, just create a new field.
                            if (dest.add(field(fd_nuke_gas, 1, cur.age))) cur.density--;
                        }
                    }
                }
                break;

   case fd_gas_vent:
    { // Used only by the mine shaft; fires on entrance into reality bubble
       for (decltype(auto) dir : Direction::vector) {
           auto dest = loc + dir;
           auto& fd = loc.field_at();
           if (fd_toxic_gas == fd.type) fd.density++; // C:Whales grace (would expect 3)
           else dest.add(field(fd_toxic_gas, 3));
       }
       loc.add(field(fd_toxic_gas, 3)); // C:Whales: self-erasing
    }
    break;

   case fd_fire_vent:
    if (1 < cur.density) {
     if (one_in(3)) cur.density--;
    } else {
     cur.type = fd_flame_burst;
     cur.density = 3;
    }
    break;

   case fd_flame_burst:
    if (1 < cur.density) cur.density--;
    else {
     cur.type = fd_fire_vent;
     cur.density = 3;
    }
    break;

   case fd_electricity:
       if (one_in(5)) break; // 4 in 5 chance to spread
       {    // interpret our grounding, or lack thereof
           bool did_something = false;
retry:
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> grounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> ungrounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_less_grounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_less_ungrounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_same_grounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_same_ungrounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_more_grounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_more_ungrounded;
           inline_stack<GPS_loc, std::end(Direction::vector) - std::begin(Direction::vector)> electrified_overcharged;
           bool any_electrified = false;
           // \todo want more resolution on electrified
           for (decltype(auto) dir : Direction::vector) {
               auto dest_loc = loc + dir;
               // \todo? check whether otherwise grounded terrain is electrically conductive
               const bool is_grounded = 0 == dest_loc.move_cost();
               const auto& fd = dest_loc.field_at();
               if (!dest_loc.field_at().is_null()) {
                   if (fd_electricity == fd.type) {
                       any_electrified = true;
                       if (fd.density < cur.density) {
                           if (is_grounded) electrified_less_grounded.push(dest_loc);
                           else electrified_less_ungrounded.push(dest_loc);
                       } else if (fd.density > cur.density) {
                           if (2 == fd.density - cur.density) electrified_overcharged.push(dest_loc);
                           if (is_grounded) electrified_more_grounded.push(dest_loc);
                           else electrified_more_ungrounded.push(dest_loc);
                       } else {
                           if (is_grounded) electrified_same_grounded.push(dest_loc);
                           else electrified_same_ungrounded.push(dest_loc);
                       }
                   }
                   continue; // no destructive overwrite
               }
               if (is_grounded) grounded.push(dest_loc);
               else ungrounded.push(dest_loc);
           };
           // electricity is configured to have a short half-life, so do not be too concerned about being too old to spread

           if (0 == loc.move_cost()) { // We're grounded
               if (const auto ub = electrified_overcharged.size()) {
                   // C:Z: suck up overcharge regardless of grounding
                   const auto index = rng(0, ub - 1);
                   auto& remote_fd = electrified_overcharged[index].field_at();
                   remote_fd.density--;
                   remote_fd.age = 0;
                   cur.density++;
                   cur.age = 0;
                   did_something = true;
                   goto retry;
               }

               if (const auto ub = electrified_more_ungrounded.size()) {
                   // C:Z: suck up free charge
                   const auto index = rng(0, ub - 1);
                   auto& remote_fd = electrified_more_ungrounded[index].field_at();
                   remote_fd.density--;
                   remote_fd.age = 0;
                   cur.density++;
                   cur.age = 0;
                   did_something = true;
                   goto retry;
               }

               // C:Whales: spark into ungrounded areas, if possible
               if (1 < cur.density) {
                   if (auto ub = ungrounded.size()) {
                       auto index = rng(0, ub - 1);
                       ungrounded[index].add(field(fd_electricity));
                       cur.density--;
                       cur.age = 0;
                       did_something = true;
                       goto retry;
                   }
               }
               if (!any_electrified) {
                   if (auto ub = grounded.size()) {
                       // C:Z: discharge into other grounded tiles
                       if (1 == cur.density) {
                           cur = field();  // gone
                           --field_count;
                           continue;
                       }
                       grounded[rng(0, ub - 1)].add(field(fd_electricity));
                       cur.density--;
                       cur.age = 0;
                       continue; // processed
                   }
               }
           } else { // ungrounded.
               if (const auto ub = grounded.size()) {
                   int index = rng(0, ub);
                   if (const auto ub2 = electrified_overcharged.size()) {
                       // we are sustained by the overcharge
                       grounded[index].add(field(fd_electricity));
                       auto& remote_fd = electrified_overcharged[rng(0, ub2 - 1)].field_at();
                       remote_fd.density--;
                       remote_fd.age = 0;
                       cur.age = 0;
                       did_something = true;
                       goto retry;
                   }
                   if (const auto ub2 = electrified_more_ungrounded.size()) {
                       // we are sustained by a larger ungrounded charge
                       grounded[index].add(field(fd_electricity));
                       auto& remote_fd = electrified_more_ungrounded[rng(0, ub2 - 1)].field_at();
                       remote_fd.density--;
                       remote_fd.age = 0;
                       cur.age = 0;
                       did_something = true;
                       goto retry;
                   }

                   if (1 == cur.density) {
                       if (did_something) continue; // don't wink out completely right after doing something
                       grounded[index].add(field(fd_electricity));
                       cur = field();  // gone
                       --field_count;
                       continue;
                   }
                   grounded[index].add(field(fd_electricity));
                   cur.density--;
                   cur.age = 0;
                   did_something = true;
                   goto retry;
               }
               // repeat above, but into ungrounded areas
               if (const auto ub = ungrounded.size()) {
                   int index = rng(0, ub);
                   if (const auto ub2 = electrified_overcharged.size()) {
                       // we are sustained by the overcharge
                       ungrounded[index].add(field(fd_electricity));
                       auto& remote_fd = electrified_overcharged[rng(0, ub2 - 1)].field_at();
                       remote_fd.density--;
                       remote_fd.age = 0;
                       cur.age = 0;
                       did_something = true;
                       goto retry;
                   }
                   if (const auto ub2 = electrified_more_ungrounded.size()) {
                       // we are sustained by a larger ungrounded charge
                       ungrounded[index].add(field(fd_electricity));
                       auto& remote_fd = electrified_more_ungrounded[rng(0, ub2 - 1)].field_at();
                       remote_fd.density--;
                       remote_fd.age = 0;
                       cur.age = 0;
                       did_something = true;
                       goto retry;
                   }

                   if (1 == cur.density) {
                       if (did_something) continue; // don't wink out completely right after doing something
                       ungrounded[index].add(field(fd_electricity));
                       cur = field();  // gone
                       field_count--;
                       continue;
                   }
                   ungrounded[index].add(field(fd_electricity));
                   cur.density--;
                   cur.age = 0;
                   did_something = true;
                   goto retry;
               }
           }
           if (did_something) continue;    // new processing kicked in, so don't need legacy processing
       }
    break;

   case fd_fatigue:
       if (3 > cur.density) {
           if (int(messages.turn) % HOURS(6) == 0 && one_in(10)) cur.density++;
       } else {
            if (one_in(HOURS(1))) { // Spawn nether creature!
                auto dest = loc + rng(within_rldist<3>);
                if (auto pt_dest = game::active()->toScreen(dest)) {
                    mon_id type = mon_id(rng(mon_flying_polyp, mon_blank));
                    g->z.push_back(monster(mtype::types[type], *pt_dest));
                }
                // \todo? handle outside of reality bubble; map generation won't trigger this but the artifact can
            }
       }
       break;

   case fd_push_items: {
       std::vector<GPS_loc> valid2(1, loc);
       for (decltype(auto) delta : Direction::vector) {
           const auto dest(delta + loc);
           if (fd_push_items == dest.field_at().type) valid2.push_back(dest);
       }
       if (1 == valid2.size() && !g->mob_at(loc)) break;    // no-op

       std::vector<item>& stack = loc.items_at();
       int i = stack.size();
       while (0 <= --i) {
           item& obj = stack[i];
           if (itm_rock != obj.type->id || int(messages.turn) - 1 <= obj.bday) continue;
           item tmp(std::move(obj));
           stack.erase(stack.begin() + i); // obj invalidated
           auto dest = valid2[rng(0, valid2.size() - 1)];
           // \todo convert to std::visit
           if (const auto whom = g->mob_at(dest)) {
               if (auto u = std::get_if<pc*>(&*whom)) {
                   messages.add("A %s hits you!", tmp.tname().c_str());
                   (*u)->hit(g, random_body_part(), rng(0, 1), 6, 0);
               } else if (auto p = std::get_if<npc*>(&*whom)) {
                   (*p)->hit(g, random_body_part(), rng(0, 1), 6, 0);
                   if (g->u.see(dest)) messages.add("A %s hits %s!", tmp.tname().c_str(), (*p)->name.c_str());
               } else {
                   auto mon = std::get<monster*>(*whom);
                   mon->hurt(6 - mon->armor_bash());
                   if (g->u.see(dest)) messages.add("A %s hits the %s!", tmp.tname().c_str(), mon->name().c_str());
               }
           }
           dest.add(std::move(tmp));
       }
   } break;

   case fd_shock_vent: // similar to AEA_STORM
       if (1 < cur.density) {
           if (one_in(5)) cur.density--;
       } else {
           cur.density = 3;
           int num_bolts = rng(3, 6);
           for (int i = 0; i < num_bolts; i++) {
               point dir = Direction::vector[rng(0, std::end(Direction::vector) - std::begin(Direction::vector) - 1)];
               auto bolt = loc;
               int n = rng(4, 12);
               while (0 <= --n) {
                   bolt += dir;
                   bolt.add(field(fd_electricity, rng(2, 3)));
                   if (one_in(4)) dir.x = (0 == dir.x) ? rng(0, 1) * 2 - 1 : 0;
                   if (one_in(4)) dir.y = (0 == dir.y) ? rng(0, 1) * 2 - 1 : 0;
               }
           }
       }
       break;

   case fd_acid_vent:
       if (1 < cur.density) {
           if (MINUTES(1) <= cur.age) {
               cur.density--;
               cur.age = 0;
           }
       }
       else {
           cur.density = 3;

           static auto drench = [&](point pt) {
               auto dest = loc + pt;
               const auto& fd = dest.field_at();
               if (fd.type == fd_null || fd.density == 0) {
                   int newdens = 3 - (Linf_dist(pt) / 2) + (one_in(3) ? 1 : 0);
                   if (newdens > 3) newdens = 3;
                   if (newdens > 0) dest.add(field(fd_acid, newdens));
               }
           };

           forall_do_inclusive(within_rldist<5>, drench);
       }
       break;

   } // switch (curtype)
  
   cur.age++;
   const int half_life = field::list[cur.type].halflife;
   if (0 < half_life) {
       if (0 < cur.age && dice(3, cur.age) > dice(3, half_life)) {
           cur.age = 0;
           cur.density--;
       }
       if (0 >= cur.density) { // Totally dissipated.
           field_count--;
           cur = field();
       }
   }
  }
 }
 return found_field;
}

void map::step_in_field(game* g, player& u)
{
    assert(!u.is_npc());    // \todo V0.2.5+ eliminate this precondition
    const point pt = u.pos;

    field& cur = field_at(pt);
    switch (cur.type) {
    case fd_null:
    case fd_blood:	// It doesn't actually do anything
    case fd_bile:		// Ditto
        return;

    case fd_web: {
        if (!u.has_trait(PF_WEB_WALKER)) {
            int web = cur.density * 5 - u.disease_level(DI_WEBBED);
            if (0 < web) u.add_disease(DI_WEBBED, web);
            remove_field(pt);
        }
    } break;

    case fd_acid:
        if (cur.density == 3) {
            messages.add("The acid burns your legs and feet!");
            u.hit(g, bp_feet, 0, 0, rng(4, 10));
            u.hit(g, bp_feet, 1, 0, rng(4, 10));
            u.hit(g, bp_legs, 0, 0, rng(2, 8));
            u.hit(g, bp_legs, 1, 0, rng(2, 8));
        } else {
            messages.add("The acid burns your feet!");
            u.hit(g, bp_feet, 0, 0, rng(cur.density, 4 * cur.density));
            u.hit(g, bp_feet, 1, 0, rng(cur.density, 4 * cur.density));
        }
        break;

    case fd_sap:
        messages.add("The sap sticks to you!");
        u.add_disease(DI_SAP, cur.density * 2);
        if (1 >= cur.density)
            remove_field(pt);
        else
            cur.density--;
        break;

    case fd_fire:
        if (!u.has_active_bionic(bio_heatsink)) {
            if (cur.density == 1) {
                messages.add("You burn your legs and feet!");
                u.hit(g, bp_feet, 0, 0, rng(2, 6));
                u.hit(g, bp_feet, 1, 0, rng(2, 6));
                u.hit(g, bp_legs, 0, 0, rng(1, 4));
                u.hit(g, bp_legs, 1, 0, rng(1, 4));
            } else if (cur.density == 2) {
                messages.add("You're burning up!");
                u.hit(g, bp_legs, 0, 0, rng(2, 6));
                u.hit(g, bp_legs, 1, 0, rng(2, 6));
                u.hit(g, bp_torso, 0, 4, rng(4, 9));
            } else if (cur.density == 3) {
                messages.add("You're set ablaze!");
                u.hit(g, bp_legs, 0, 0, rng(2, 6));
                u.hit(g, bp_legs, 1, 0, rng(2, 6));
                u.hit(g, bp_torso, 0, 4, rng(4, 9));
                u.add_disease(DI_ONFIRE, 5);
            }
            if (cur.density == 2)
                u.infect(DI_SMOKE, bp_mouth, 5, 20);
            else if (cur.density == 3)
                u.infect(DI_SMOKE, bp_mouth, 7, 30);
        }
        break;

    case fd_smoke:
        if (cur.density == 3)
            u.infect(DI_SMOKE, bp_mouth, 4, 15);
        break;

    case fd_tear_gas:
        if (cur.density > 1 || !one_in(3))
            u.infect(DI_TEARGAS, bp_mouth, 5, 20);
        if (cur.density > 1)
            u.infect(DI_BLIND, bp_eyes, cur.density * 2, 10);
        break;

    case fd_toxic_gas:
        if (cur.density == 2)
            u.infect(DI_POISON, bp_mouth, 5, 30);
        else if (cur.density == 3)
            u.infect(DI_BADPOISON, bp_mouth, 5, 30);
        break;

    case fd_nuke_gas:
        u.radiation += rng(0, cur.density * (cur.density + 1));
        if (cur.density == 3) {
            messages.add("This radioactive gas burns!");
            u.hurtall(rng(1, 3));
        }
        break;

    case fd_flame_burst:
        if (!u.has_active_bionic(bio_heatsink)) {
            messages.add("You're torched by flames!");
            u.hit(g, bp_legs, 0, 0, rng(2, 6));
            u.hit(g, bp_legs, 1, 0, rng(2, 6));
            u.hit(g, bp_torso, 0, 4, rng(4, 9));
        } else
            messages.add("These flames do not burn you.");
        break;

    case fd_electricity:
        if (u.has_artifact_with(AEP_RESIST_ELECTRICITY))
            messages.add("The electricity flows around you.");
        else {
            messages.add("You're electrocuted!");
            u.hurtall(rng(1, cur.density));
            if (one_in(8 - cur.density) && !one_in(30 - u.str_cur)) {
                messages.add("You're paralyzed!");
                u.moves -= rng(cur.density * 50, cur.density * 150);
            }
        }
        break;

    case fd_fatigue:
        if (rng(0, 2) < cur.density) {
            messages.add("You're violently teleported!");
            u.hurtall(cur.density);
            g->teleport(&u);
        }
        break;

    case fd_shock_vent:
    case fd_acid_vent:
        remove_field(pt);
        break;
    }
}

void map::mon_in_field(game* g, monster& z)
{
    if (z.has_flag(MF_DIGS)) return;	// Digging monsters are immune to fields
    const point pt = z.pos;
    field& cur = field_at(pt);
    int dam = 0;
    switch (cur.type) {
    case fd_null:
    case fd_blood:	// It doesn't actually do anything
    case fd_bile:		// Ditto
        break;

    case fd_web:
        if (!z.has_flag(MF_WEBWALK)) {
            cataclysm::rational_scale<4, 5>(z.speed);
            remove_field(pt);
        }
        break;

    case fd_acid:
        if (/* !z.has_flag(MF_DIGS) && */ !z.has_flag(MF_FLIES) && !z.has_flag(MF_ACIDPROOF)) {
            if (cur.density == 3)
                dam = rng(4, 10) + rng(2, 8);
            else
                dam = rng(cur.density, cur.density * 4);
        }
        break;

    case fd_sap:
        z.speed -= cur.density * 5;
        if (1 >= cur.density)
            remove_field(pt);
        else
            cur.density--;
        break;

    case fd_fire:
        if (z.made_of(FLESH))
            dam = 3;
        if (z.made_of(VEGGY))
            dam = 12;
        if (z.made_of(PAPER) || z.made_of(LIQUID) || z.made_of(POWDER) ||
            z.made_of(WOOD) || z.made_of(COTTON) || z.made_of(WOOL))
            dam = 20;
        if (z.made_of(STONE) || z.made_of(KEVLAR) || z.made_of(STEEL))
            dam = -20;
        if (z.has_flag(MF_FLIES))
            dam -= 15;

        if (cur.density == 1)
            dam += rng(2, 6);
        else if (cur.density == 2) {
            dam += rng(6, 12);
            if (!z.has_flag(MF_FLIES)) {
                z.moves -= mobile::mp_turn / 5;
                if (!z.ignitable()) z.add_effect(ME_ONFIRE, rng(3, 8));
            }
        }
        else if (cur.density == 3) {
            dam += rng(10, 20);
            if (!z.has_flag(MF_FLIES) || one_in(3)) {
                z.moves -= (mobile::mp_turn / 5) * 2;
                if (!z.ignitable()) z.add_effect(ME_ONFIRE, rng(8, 12));
            }
        }
        [[fallthrough]]; // Drop through to smoke

    case fd_smoke:
        if (cur.density == 3)
            z.speed -= rng(10, 20);
        if (z.made_of(VEGGY))	// Plants suffer from smoke even worse
            z.speed -= rng(1, cur.density * 12);
        break;

    case fd_tear_gas:
        if (z.made_of(FLESH) || z.made_of(VEGGY)) {
            z.add_effect(ME_BLIND, cur.density * 8);
            if (cur.density == 3) {
                z.add_effect(ME_STUNNED, rng(10, 20));
                dam = rng(4, 10);
            }
            else if (cur.density == 2) {
                z.add_effect(ME_STUNNED, rng(5, 10));
                dam = rng(2, 5);
            }
            else
                z.add_effect(ME_STUNNED, rng(1, 5));
            if (z.made_of(VEGGY)) {
                z.speed -= rng(cur.density * 5, cur.density * 12);
                dam += cur.density * rng(8, 14);
            }
        }
        break;

    case fd_toxic_gas:
        dam = cur.density;
        z.speed -= cur.density;
        break;

    case fd_nuke_gas:
        if (cur.density == 3) {
            z.speed -= rng(60, 120);
            dam = rng(30, 50);
        }
        else if (cur.density == 2) {
            z.speed -= rng(20, 50);
            dam = rng(10, 25);
        }
        else {
            z.speed -= rng(0, 15);
            dam = rng(0, 12);
        }
        if (z.made_of(VEGGY)) {
            z.speed -= rng(cur.density * 5, cur.density * 12);
            dam *= cur.density;
        }
        break;

    case fd_flame_burst:
        if (z.made_of(FLESH))
            dam = 3;
        if (z.made_of(VEGGY))
            dam = 12;
        if (z.made_of(PAPER) || z.made_of(LIQUID) || z.made_of(POWDER) ||
            z.made_of(WOOD) || z.made_of(COTTON) || z.made_of(WOOL))
            dam = 50;
        if (z.made_of(STONE) || z.made_of(KEVLAR) || z.made_of(STEEL))
            dam = -25;
        dam += rng(0, 8);
        z.moves -= 20;
        break;

    case fd_electricity:
        dam = rng(1, cur.density);
        if (one_in(8 - cur.density)) z.moves -= cur.density * 150;
        break;

    case fd_fatigue:
        if (rng(0, 2) < cur.density) {
            dam = cur.density;
            g->teleport_to(z, g->teleport_destination_unsafe(z.pos, 10));
        }
        break;

 }
    if (0 < dam && z.hurt(dam)) g->kill_mon(z);
}
