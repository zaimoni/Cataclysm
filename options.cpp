#include "options.h"
#include "output.h"
#include "JSON.h"

#include <fstream>

using cataclysm::JSON;

static bool is_bool(option_key id)
{
#if 0
	switch (id) {
	default: return true;
	}
#endif
	return true;
}

// return value true does imply boolean option
static bool default_true(option_key opt)
{
	switch (opt)
	{
	case OPT_FORCE_YN:
	case OPT_SAFEMODE: return true;
	default: return false;
	}
}

static const char* JSON_key(option_key opt)	// \todo micro-optimize this and following by converting this to guarded array dereference?
{
	switch (opt)
	{
	case OPT_FORCE_YN: return "force Y/N";
	case OPT_USE_CELSIUS: return "use Celsius";
	case OPT_USE_METRIC_SYS: return "use SI i.e. metric units";
	case OPT_NO_CBLINK: return "no cblink";
	case OPT_24_HOUR: return "24 hour clock";
	case OPT_SNAP_TO_TARGET: return "snap to target";
	case OPT_SAFEMODE: return "safe mode";
	case OPT_AUTOSAFEMODE: return "auto safe mode";
	default: return 0;
	}
}

static option_key opt_key(const char* const x)
{
	if (!x) return OPT_NULL;
	int opt = OPT_FORCE_YN;
	do {
		option_key option = (option_key)opt;
		const auto key = JSON_key(option);
		if (key && !strcmp(x, key)) return option;
	} while (NUM_OPTION_KEYS > ++opt);
	return OPT_NULL;
}

static JSON& get_JSON_opts() {
	static JSON* x = 0;
	if (!x) {
		std::ifstream fin;
		fin.open("data/options.json");
		if (fin.is_open()) x = new JSON(fin);
		else x = new JSON();
		fin.close();
		// audit our own keys.  Since we will be used to initialize the option_table we can't reference it.
		if (!x->has_key("lang")) x->set("lang", "en");	// not used right now but we do want to allow configuring translations
		if (!x->has_key("NPCs")) x->set("NPCs", "true");	// currently on file-exists kludge in main program
		int opt = OPT_FORCE_YN;
		do  {
			option_key option = (option_key)opt;
			const std::string key(JSON_key(option));
			if (!x->has_key(key)) x->set(key, default_true(option) ? "true" : "false");
			}
		while(NUM_OPTION_KEYS > ++opt);	// coincidentally all options are boolean currently
	}
	return *x;
}

option_table::option_table() {
	const auto& opts = get_JSON_opts();
	for (int i = 0; i < NUM_OPTION_KEYS; i++) {
		const auto key = JSON_key(option_key(i));
		if (!key) options[i] = false;	// ok even after numeric options allowed; auto-converts to 0.0
		else {
			auto& test = opts[key];
			if (!test.is_scalar()) options[i] = false;	// only should handle numerals or true/false
			else {
				auto val = test.scalar();
				if ("true" == val) options[i] = true;
				else if ("false" == val) options[i] = false;
				else options[i] = false;	// \todo implement reading double as an option value
			}
		}
	}
};

option_table& option_table::get()
{
	static option_table* x = 0;
	if (!x) x = new option_table();
	return *x;
}

void option_table::set(option_key i, double val) 
{ 
	const auto key = JSON_key(i);
	if (!key) return;	// invalid in some way
	options[i] = val;
	// currently all options are actually boolean
	get_JSON_opts().set(key, val ? "true" : "false");
}

std::string option_name(option_key key)
{
 switch (key) {
  case OPT_USE_CELSIUS:		return "Use Celsius";
  case OPT_USE_METRIC_SYS: return "Use Metric System";
  case OPT_FORCE_YN:		return "Force Y/N in prompts";
  case OPT_NO_CBLINK:		return "No Bright Backgrounds";
  case OPT_24_HOUR:		return "24 Hour Time";
  case OPT_SNAP_TO_TARGET:	return "Snap to Target";
  case OPT_SAFEMODE:		return "Safemode on by default";
  case OPT_AUTOSAFEMODE:	return "Auto-Safemode on by default";
  default:			return "Unknown Option (BUG)";
 }
 return "Big ol Bug";
}

void save_options()
{
 std::ofstream fout;
 fout.open("data/options.json");
 if (fout.is_open()) fout << get_JSON_opts() << std::endl;
}
