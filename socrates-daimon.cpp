/* Main Loop for Socrates' Daimon, Cataclysm analysis utility */

#include "color.h"
#include "mtype.h"
#include "item.h"
#include "output.h"
#include "ios_file.h"
#include "json.h"
#include <stdexcept>
#include <stdlib.h>
#include <time.h>

#include <fstream>

using namespace cataclysm;

// from saveload.cpp
bool fromJSON(const JSON& _in, item& dest);
JSON toJSON(const item& src);

static void check_roundtrip_JSON(const item& src)
{
	item staging;
	if (!fromJSON(toJSON(src), staging)) throw std::logic_error("critical round-trip failure");
	if (src != staging) throw std::logic_error("round-trip was not invariant");
}

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

	DECLARE_AND_ACID_OPEN(std::ofstream, fout, "data\\items_raw.txt", return EXIT_FAILURE;)

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
			// constructor hard-coding (specification)
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
				if ('~' != it->sym) throw std::logic_error("unexpected symbol");
			} else {
				if (1 != it->melee_dam) throw std::logic_error("unexpected melee damage");
				if ('=' != it->sym) throw std::logic_error("unexpected symbol");
				// exceptions are for plasma state
				if (!it->m1 && itm_bio_fusion != it->id && itm_charge_shot!=it->id) throw std::logic_error("null material for ammo");
			}
		} else if (it->is_software()) {
			const auto sw = static_cast<it_software*>(it);
			if (!sw) throw std::logic_error(it->name + ": static cast to software failed");
			// constructor hard-coding (specification)
			if (0 != it->rarity) throw std::logic_error("unexpected rarity");	// 2020-03-14 these don't randomly spawn; might want to change that
			// the container may have volume, etc. but the software itself isn't very physical
			if (it->m1) throw std::logic_error("unexpectedly material");
			if (0 != it->volume) throw std::logic_error("unexpected volume");
			if (0 != it->weight) throw std::logic_error("unexpected weight");
			if (0 != it->melee_dam) throw std::logic_error("unexpected melee damage");
			if (0 != it->melee_cut) throw std::logic_error("unexpected cutting damage");
			if (0 != it->m_to_hit) throw std::logic_error("unexpected melee accuracy");
			if (0 != it->item_flags) throw std::logic_error("unexpected flags");	// 2020-03-14 not physical enough to have current flags
			// macro hard-coding (may want to change these but currently invariant)
			if (' ' != it->sym) throw std::logic_error("unexpected symbol");
			if (c_white != it->color) throw std::logic_error("unexpected color");
		} else if (!it->m1) {
			if (itm_toolset < it->id && num_items != it->id && num_all_items != it->id) {
                throw std::logic_error("unexpected null material: "+std::to_string(it->id));
            }
		}
/*
 virtual bool is_food() const    { return false; }
 virtual bool is_gun() const     { return false; }
 virtual bool is_gunmod() const  { return false; }
 virtual bool is_bionic() const  { return false; }
 virtual bool is_armor() const   { return false; }
 virtual bool is_book() const    { return false; }
 virtual bool is_tool() const    { return false; }
 virtual bool is_container() const { return false; }
 virtual bool is_macguffin() const { return false; }
 virtual bool is_artifact() const { return false; }
*/
		// check what happens when item is created.  VAPORWARE generate web page system from this
		if (itm_corpse == it->id) {	// corpses need their own testing path
		} else {
			item test(it, 0);
			fout << test.tname() << std::endl << test.info(true) << std::endl << "====" << std::endl;
			if (it->is_food() || it->is_software()) {	// i.e., can create in own container
				auto test2 = test.in_its_container();
				if (test2.type != test.type) {
					fout << test2.tname() << std::endl << test2.info(true) << std::endl << "====" << std::endl;
				}
				check_roundtrip_JSON(test2);
			}
			if (num_items != it->id && itm_null != it->id) check_roundtrip_JSON(test);
		}
	}

	OFSTREAM_ACID_CLOSE(fout, "data\\items_raw.txt")

	// route the command line options here

	erase(); // Clear screen
	endwin(); // End ncurses
	system("clear"); // Tell the terminal to clear itself
	return 0;
}

