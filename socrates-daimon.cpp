/* Main Loop for Socrates' Daimon, Cataclysm analysis utility */

#include "color.h"
#include "mtype.h"
#include "item.h"
#include <stdexcept>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
	// \todo parse command line options

	srand(time(NULL));

	// ncurses stuff
	initscr(); // Initialize ncurses
	noecho();  // Don't echo keypresses
	cbreak();  // C-style breaks (e.g. ^C to SIGINT)
	keypad(stdscr, true); // Numpad is numbers
	init_colors(); // See color.cpp
	curs_set(0); // Invisible cursor

	// roundtrip tests to schedule: mon_id, itype_id, item_flag, technique_id, art_charge, art_effect_active, art_effect_passive

	// bootstrap
	item::init();
	mtype::init();
//	mtype::init_items();     need to do this but at a later stage

	// item type scan
	auto ub = item::types.size();
	while (0 < ub) {
		const auto it = item::types[--ub];
		if (!it) throw std::logic_error("null item type");
        if (it->id != ub) throw std::logic_error("reverse lookup failure: " + std::to_string(it->id)+" for " + std::to_string(ub));
        // item material reality checks
		if (!it->m1 && it->m2) throw std::logic_error("non-null secondary material for null primary material");
		if (it->is_style()) {
			// constructor hard-coding (specification)
			const auto style = static_cast<it_style*>(it);
			if (!style) throw std::logic_error(it->name + ": static cast to style failed");
			if (0 != it->rarity) throw std::logic_error("unexpected rarity");
			if (0 != it->price) throw std::logic_error("unexpected pre-apocalypse value");
			if (it->m1) throw std::logic_error("unexpected material");
			if (it->m2) throw std::logic_error("unexpected secondary material");
			if (0 != it->volume) throw std::logic_error("unexpected volume");
			if (0 != it->weight) throw std::logic_error("unexpected weight");
			// macro hard-coding (may want to change these but currently invariant)
			if (1 > style->moves.size()) throw std::logic_error("style " + it->name + " has too few moves: " + std::to_string(style->moves.size()));
			if ('$' != it->sym) throw std::logic_error("unexpected symbol");
			if (c_white != it->color) throw std::logic_error("unexpected color");
			if (0 != it->melee_cut) throw std::logic_error("unexpected cutting damage");
			if (0 != it->m_to_hit) throw std::logic_error("unexpected melee accuracy");
			if (mfb(IF_UNARMED_WEAPON) != it->item_flags) throw std::logic_error("unexpected flags");
		} else if (it->is_ammo()) {
			// constructor hard-coding (specification)
			const auto ammo = static_cast<it_ammo*>(it);
			if (!ammo) throw std::logic_error(it->name + ": static cast to ammo failed");
			if (it->m2) throw std::logic_error("unexpected secondary material");
			if (0 != it->melee_cut) throw std::logic_error("unexpected cutting damage");
			if (0 != it->m_to_hit) throw std::logic_error("unexpected melee accuracy");
			// macro hard-coding (may want to change these but currently invariant)
			// ammo and fuel are conflated in the type system.
			// 2020-03-12: All fuel is material type LIQUID (this likely will be adjusted;
			// it would make more sense for this to be a phase than a material type)
			if (LIQUID == it->m1) {
				if (1 != it->volume) throw std::logic_error("unexpected volume");
				if (1 != it->weight) throw std::logic_error("unexpected weight");
				if (0 != it->melee_dam) throw std::logic_error("unexpected melee damage");
			} else {
				if (1 != it->melee_dam) throw std::logic_error("unexpected melee damage");
				// exceptions are for plasma state
				if (!it->m1 && itm_bio_fusion != it->id && itm_charge_shot!=it->id) throw std::logic_error("null material for ammo");
			}
#if 0
		} else if (!it->m1) {
			if (itm_toolset < it->id && num_items != it->id && num_all_items != it->id) {
                throw std::logic_error("unexpected null material: "+std::to_string(it->id));
            }
#endif
		}
	}

	// route the command line options here

	erase(); // Clear screen
	endwin(); // End ncurses
	system("clear"); // Tell the terminal to clear itself
	return 0;
}

