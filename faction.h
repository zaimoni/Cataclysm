#ifndef _FACTION_H_
#define _FACTION_H_

#include "enums.h"
#include "enum_json.h"
#include "c_bitmap.h"
#include <vector>

// TODO: Redefine?
#define MAX_FAC_NAME_SIZE 40

std::string fac_ranking_text(int val);
std::string fac_respect_text(int val);

class game;

namespace cataclysm {
	class JSON;
}

enum faction_goal {
 FACGOAL_NULL = 0,
 FACGOAL_NONE,
 FACGOAL_WEALTH,
 FACGOAL_DOMINANCE,
 FACGOAL_CLEANSE,
 FACGOAL_SHADOW,
 FACGOAL_APOCALYPSE,
 FACGOAL_ANARCHY,
 FACGOAL_KNOWLEDGE,
 FACGOAL_NATURE,
 FACGOAL_CIVILIZATION,
 FACGOAL_FUNGUS,
 NUM_FACGOALS
};

DECLARE_JSON_ENUM_SUPPORT(faction_goal)

enum faction_job {
 FACJOB_NULL = 0,
 FACJOB_EXTORTION,	// Protection rackets, etc
 FACJOB_INFORMATION,	// Gathering & sale of information
 FACJOB_TRADE,		// Bazaars, etc.
 FACJOB_CARAVANS,	// Transportation of goods
 FACJOB_SCAVENGE,	// Scavenging & sale of general supplies
 FACJOB_MERCENARIES,	// General fighters-for-hire
 FACJOB_ASSASSINS,	// Targeted discreet killing
 FACJOB_RAIDERS,	// Raiding settlements, trade routes, &c
 FACJOB_THIEVES,	// Less violent; theft of property without killing
 FACJOB_GAMBLING,	// Maitenance of gambling parlors
 FACJOB_DOCTORS,	// Doctors for hire
 FACJOB_FARMERS,	// Farming & sale of food
 FACJOB_DRUGS,		// Drug dealing
 FACJOB_MANUFACTURE,	// Manufacturing & selling goods
 NUM_FACJOBS
};

DECLARE_JSON_ENUM_SUPPORT(faction_job)

enum faction_value {
 FACVAL_NULL = 0,
 FACVAL_CHARITABLE,	// Give their job for free (often)
 FACVAL_LONERS,		// Avoid associating with outsiders
 FACVAL_EXPLORATION,	// Like to explore new territory
 FACVAL_ARTIFACTS,	// Seek out unique items
 FACVAL_BIONICS,	// Collection & installation of bionic parts
 FACVAL_BOOKS,		// Collection of books
 FACVAL_TRAINING,	// Training among themselves and others
 FACVAL_ROBOTS,		// Creation / use of robots
 FACVAL_TREACHERY,	// Untrustworthy
 FACVAL_STRAIGHTEDGE,	// No drugs, alcohol, etc.
 FACVAL_LAWFUL,		// Abide by the old laws
 FACVAL_CRUELTY,	// Torture, murder, etc.
 NUM_FACVALS
};

DECLARE_JSON_ENUM_BITFLAG_SUPPORT(faction_value)

struct faction {
#undef MIN_ID
    enum {
        MIN_ID = 0
    };

 faction(int uid = -1) noexcept;
 faction(const faction& src) = default;
 faction(faction&& src) = default;
 ~faction() = default;
 faction& operator=(const faction& src) = default;
 faction& operator=(faction && src) = default;

 friend bool fromJSON(const cataclysm::JSON& _in, faction& dest);
 friend cataclysm::JSON toJSON(const faction& src);
 static faction* from_id(int uid);
 static auto assign_id() { return next_id++; }

 void randomize();
 void make_army();
 bool has_job(faction_job j) const;
 bool has_value(faction_value v) const { return values & mfb(v); };
 bool matches_us(faction_value v) const;
 std::string describe() const;

 unsigned int response_time(tripoint dest) const;	// Time it takes for them to get to u

 std::string name;
 typename cataclysm::bitmap<NUM_FACVALS>::type values; // Bitfield of values
 faction_goal goal;
 faction_job job1, job2;
 std::vector<int> opinion_of;
 int likes_u;
 int respects_u;
 bool known_by_u;
 int id;
 int strength, sneak, crime, cult, good;	// Defining values
 point om;	// Which overmap are we based in? \todo retype to OM_loc?
 point map; // Where in that overmap are we? (coordinate ranges 0...OMAPX/Y)
 int size;	// How big is our sphere of influence?
 int power;	// General measure of our power

 static void global_reset();
 static void global_fromJSON(const cataclysm::JSON& src);
 static void global_toJSON(cataclysm::JSON& dest);
private:
 static int next_id;   // mininum/default next mission id
};

#endif
