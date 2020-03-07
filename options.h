#ifndef _OPTIONS_H_
#define _OPTIONS_H_

// This is not a correct location for this integrity test, but the no-op Socrates' Daimon had a *VERY* limited set of mandatory include files.
// This one is both game-specific and actually needed by the no-op Socrates' Daimon.

#ifdef CATACLYSM
#ifdef SOCRATES_DAIMON
#error cannot signal that we are building both Cataclysm and Socrates Daimon
#endif
#elif SOCRATES_DAIMON
#else
#error must signal which executable we are building
#endif

#include <string>

enum option_key {
OPT_NULL = 0,
OPT_FORCE_YN, // Y/N versus y/n
OPT_USE_CELSIUS, // Display temp as C not F
OPT_USE_METRIC_SYS, // Display speed as Km/h not mph
OPT_NO_CBLINK, // No bright backgrounds
OPT_24_HOUR, // 24 hour time
OPT_SNAP_TO_TARGET, // game::firing snaps to target
OPT_SAFEMODE, // Safemode on by default?
OPT_AUTOSAFEMODE, // Autosafemode on by default?
OPT_NPCS,	// NPCs generated in game world
OPT_LOAD_TILES,	// use tileset
OPT_FONT_HEIGHT,	// font height (ASCII)
OPT_EXTRA_MARGIN,	// correction to margin to avoid clipping text
NUM_OPTION_KEYS,
OPT_FONT = NUM_OPTION_KEYS,
OPT_FONTPATH
};

enum {
    MAX_OPTIONS = OPT_FONTPATH+1
};

enum {
	OPTTYPE_BOOL = 0,
	OPTTYPE_INT,
	OPTTYPE_DOUBLE,
    OPTTYPE_STRING  // requires hard-coding support
};

// Historically, this was intended to be a singleton.  Enforce this.
class option_table
{
private:
 double options[NUM_OPTION_KEYS];
 std::string str_opts[MAX_OPTIONS - NUM_OPTION_KEYS];

 option_table();
 option_table(const option_table& src) = delete;
 option_table(option_table&& src) = delete;
 ~option_table() = default;
 option_table& operator=(const option_table& src) = delete;

public:
 double operator[](option_key i) const { return options[i]; }
 double operator[](int i) const { return options[i]; }
 const std::string& getopt(option_key i) const { return str_opts[i - NUM_OPTION_KEYS]; }
 const std::string& getopt(int i) const { return str_opts[i - NUM_OPTION_KEYS]; }
 void set(option_key i, double val);

 static constexpr int type_code(option_key id)
 {
	 switch (id)
	 {
	 case OPT_FONT_HEIGHT:	return OPTTYPE_INT;
	 case OPT_EXTRA_MARGIN:	return OPTTYPE_INT;
     case OPT_FONT: return OPTTYPE_STRING;
     case OPT_FONTPATH: return OPTTYPE_STRING;
     default: return OPTTYPE_BOOL;	// default: boolean
	 }
 }

 static option_table& get();
};

void save_options();
std::string option_name(option_key key);

#endif
