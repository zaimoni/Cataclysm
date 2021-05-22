#include "trap.h"
#ifndef SOCRATES_DAIMON
#include "trap_handler.hpp"
#endif
#include <string.h>
#include "Zaimoni.STL/Logging.h"

static const char* const JSON_transcode_traps[] = {
	"bubblewrap",
	"beartrap",
	"beartrap_buried",
	"snare",
	"nailboard",
	"tripwire",
	"crossbow",
	"shotgun_2",
	"shotgun_1",
	"engine",
	"blade",
	"landmine",
	"telepad",
	"goo",
	"dissector",
	"sinkhole",
	"pit",
	"spike_pit",
	"lava",
	"portal",
	"ledge",
	"boobytrap",
	"temple_flood",
	"temple_toggle",
	"glow",
	"hum",
	"shadow",
	"drain",
	"snake"
};

DEFINE_JSON_ENUM_SUPPORT_TYPICAL(trap_id, JSON_transcode_traps)

std::vector<const trap*> trap::traps;

void trap::init()
{
 int id = -1;
#ifndef SOCRATES_DAIMON
#define TRAP_HANDLERS(...) , __VA_ARGS__
#else
#define TRAP_HANDLERS(...)
#endif

//	Name			Symbol	Color		Vis Avd Diff
 traps.push_back(new trap(++id, "none", '?', c_white, 20, 0, 0, {}, {}));
 traps.push_back(new trap(++id, "bubblewrap", '_', c_ltcyan, 0, 8, 0, { itm_bubblewrap }, {} TRAP_HANDLERS(&trapfunc::bubble, &trapfuncm::bubble)));
 traps.push_back(new trap(++id, "bear trap", '^', c_blue, 2, 7, 3, { itm_beartrap }, { { itm_beartrap } } TRAP_HANDLERS(&trapfunc::beartrap, &trapfuncm::beartrap)));
 traps.push_back(new trap(++id, "buried bear trap", '^', c_blue, 9, 8, 4, { itm_beartrap }, { { itm_beartrap } } TRAP_HANDLERS(&trapfunc::beartrap, &trapfuncm::beartrap)));
 traps.push_back(new trap(++id, "rabbit snare", '\\', c_brown, 5, 10, 2, { itm_stick, itm_string_36 }, {})); // \todo implement ...::snare
 traps.push_back(new trap(++id, "spiked board", '_', c_ltgray, 1, 6, 0, { itm_board_trap }, {} TRAP_HANDLERS(&trapfunc::board, &trapfuncm::board)));
 traps.push_back(new trap(++id, "tripwire", '^', c_ltred, 6, 4, 3, { itm_string_36 }, {} TRAP_HANDLERS(&trapfunc::tripwire, &trapfuncm::tripwire)));
 traps.push_back(new trap(++id, "crossbow trap", '^', c_green, 5, 4, 5, { itm_string_36, itm_crossbow }, { {itm_crossbow}, {itm_string_6} } TRAP_HANDLERS(&trapfunc::crossbow, &trapfuncm::crossbow)));
 traps.push_back(new trap(++id, "shotgun trap", '^', c_red, 4, 5, 6, { itm_string_36, itm_shotgun_sawn }, { {itm_shotgun_sawn}, {itm_string_6} } TRAP_HANDLERS(&trapfunc::shotgun, &trapfuncm::shotgun))); // 2 shots
 traps.push_back(new trap(++id, "shotgun trap", '^', c_red, 4, 5, 6, { itm_string_36, itm_shotgun_sawn }, { {itm_shotgun_sawn}, {itm_string_6} } TRAP_HANDLERS(&trapfunc::shotgun, &trapfuncm::shotgun))); // 1 shot
 traps.push_back(new trap(++id, "spinning blade engine", '_', c_ltred, 0, 0, 2, { itm_motor, itm_machete, itm_machete }, {}));
 traps.push_back(new trap(++id, "spinning blade", '\\', c_cyan, 0, 4, 99, {}, {} TRAP_HANDLERS(&trapfunc::blade, &trapfuncm::blade)));
 traps.push_back(new trap(++id, "land mine", '_', c_red, 10, 14, 10, { itm_landmine }, {} TRAP_HANDLERS(&trapfunc::landmine, &trapfuncm::landmine)));
 traps.push_back(new trap(++id, "teleport pad", '_', c_magenta, 0, 15, 20, {}, {} TRAP_HANDLERS(&trapfunc::telepad, &trapfuncm::telepad)));
 traps.push_back(new trap(++id, "goo pit", '_', c_dkgray, 0, 15, 15, {}, {} TRAP_HANDLERS(&trapfunc::goo, &trapfuncm::goo)));
 traps.push_back(new trap(++id, "dissector", '7', c_cyan, 2, 20, 99, {}, {} TRAP_HANDLERS(&trapfunc::dissector, &trapfuncm::dissector)));
 traps.push_back(new trap(++id, "sinkhole", '_', c_brown, 10, 14, 99, {}, {} TRAP_HANDLERS(&trapfunc::sinkhole)));
 traps.push_back(new trap(++id, "pit", '0', c_brown, 0, 8, 99, {}, {} TRAP_HANDLERS(&trapfunc::pit, &trapfuncm::pit)));
 traps.push_back(new trap(++id, "spiked pit", '0', c_blue, 0, 8, 99, {}, { {itm_spear_wood, 4, 3} } TRAP_HANDLERS(&trapfunc::pit_spikes, &trapfuncm::pit_spikes)));
 traps.push_back(new trap(++id, "lava", '~', c_red, 0, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::lava, &trapfuncm::lava)));
 // The '%' symbol makes the portal cycle through ~*0&
 traps.push_back(new trap(++id, "shimmering portal", '%', c_magenta, 0, 30, 99, {}, {})); // \todo implement major game feature; ...::portal
 traps.push_back(new trap(++id, "ledge", ' ', c_black, 0, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::ledge, &trapfuncm::ledge)));
 traps.push_back(new trap(++id, "boobytrap", '^', c_ltcyan, 5, 4, 7, {}, {} TRAP_HANDLERS(&trapfunc::boobytrap, &trapfuncm::boobytrap)));
 traps.push_back(new trap(++id, "raised tile", '^', c_ltgray, 9, 20, 99, {}, {} TRAP_HANDLERS(&trapfunc::temple_flood)));
 // Toggles through states of RGB walls
 traps.push_back(new trap(++id, "", '^', c_white, 99, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::temple_toggle)));
 traps.push_back(new trap(++id, "", '^', c_white, 99, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::glow, &trapfuncm::glow))); // Glow attack
 traps.push_back(new trap(++id, "", '^', c_white, 99, 99, 99, {}, {} TRAP_HANDLERS(nullptr, nullptr, &trapfunc::hum))); // Hum attack
 traps.push_back(new trap(++id, "", '^', c_white, 99, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::shadow))); // Shadow spawn
 traps.push_back(new trap(++id, "", '^', c_white, 99, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::drain, &trapfuncm::drain))); // Drain attack
 traps.push_back(new trap(++id, "", '^', c_white, 99, 99, 99, {}, {} TRAP_HANDLERS(&trapfunc::snake, &trapfuncm::snake))); // Snake spawn / hisssss
#undef TRAP_HANDLERS

	assert(num_trap_types == traps.size());	// postcondition check
}
