#include "options.h"
#include "output.h"
#include "json.h"
#include "ios_file.h"

#include <fstream>

using cataclysm::JSON;

// return value true does imply boolean option
static constexpr bool default_true(option_key opt)
{
	switch (opt)
	{
	case OPT_FORCE_YN:
	case OPT_SAFEMODE: 
	case OPT_LOAD_TILES:
	case OPT_NPCS: return true;
	default: return false;
	}
}

static constexpr int default_int(option_key opt)
{
	switch (opt)
	{
	case OPT_FONT_HEIGHT: return 16;
	case OPT_VIEW: return 25;
	case OPT_PANELX: return 55;
	case OPT_SCREENWIDTH: return default_int(OPT_VIEW) + default_int(OPT_PANELX);
	default: return 0;
	}
}

static constexpr double default_numeric(option_key opt)
{
/*	switch (opt)
	{
	default: */
		return default_int(opt);
//	}
}

static constexpr double min_numeric(option_key opt)
{
	switch (opt)
	{
	case OPT_FONT_HEIGHT: return 10;	// historically deemed unreadable if calculated width 4 or lower; likewise height 4 or lower
	default: return 0;
	}
}

static constexpr const char* JSON_key(option_key opt)	// \todo micro-optimize this and following by converting this to guarded array dereference?
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
	case OPT_NPCS: return "NPCs";
	case OPT_LOAD_TILES: return "load tiles";
	case OPT_FONT_HEIGHT: return "font height";
	case OPT_EXTRA_MARGIN: return "extra bottom-right margin";
/*	case OPT_VIEW: return "screen height, i.e. view diameter"; // don't want to be able to set these by normal UI
	case OPT_PANELX: return "side panel width";
	case OPT_SCREENWIDTH: return "screen width"; */
	case OPT_FONT:	return "font";
	case OPT_FONTPATH:	return "font path";
	default: return nullptr;
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

#define OPT_FILE "data/options.json"

static JSON& get_JSON_opts() {
	static JSON* x = nullptr;
	if (!x) {
		std::ifstream fin(OPT_FILE);
		if (fin) x = new JSON(fin);
		else x = new JSON();
		fin.close();
		// audit our own keys.  Since we will be used to initialize the option_table we can't reference it.
		if (!x->has_key("lang")) x->set("lang", "en");	// not used right now but we do want to allow configuring translations
		if (!x->has_key("font")) x->set("font", "Terminus");	// these two should agree with xlat.fonts.json
		if (!x->has_key("font path")) x->set("lang", "data/termfont");
		int opt = OPT_FORCE_YN;
		do  {
			option_key option = (option_key)opt;
			const std::string key(JSON_key(option));
			if (!x->has_key(key)) {
				switch (option_table::type_code(option))
				{
				case OPTTYPE_INT: x->set(key, std::to_string(default_int(option)));
					break;
				case OPTTYPE_DOUBLE: x->set(key, std::to_string(default_numeric(option)));
					break;
				default: x->set(key, default_true(option) ? "true" : "false");
					break;
				}
			}
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
				else options[i] = strtod(val.c_str(),nullptr);	// also handles int
			}
		}
	}
};

option_table& option_table::get()
{
	static option_table* x = nullptr;
	if (!x) x = new option_table();
	return *x;
}

void option_table::set(option_key i, double val) 
{ 
	const auto key = JSON_key(i);
	if (!key) return;	// invalid in some way
	switch (type_code(i)) {
	case OPTTYPE_DOUBLE:
	case OPTTYPE_INT: 
		if (min_numeric(i) > val) val = min_numeric(i);	// clamp numeric options; don't worry about CPU, this is UI
		break;
	}
	options[i] = val;
	switch (type_code(i)) {
	case OPTTYPE_DOUBLE:
		get_JSON_opts().set(key, std::to_string(val));
		break;
	case OPTTYPE_INT:
		get_JSON_opts().set(key, std::to_string((int)val));
		break;
	case OPTTYPE_BOOL:
	default:
		get_JSON_opts().set(key, val ? "true" : "false");
		break;
	}
}

// intended interpretation is that x and y are the return values from curses' getmaxx(stdscr) and getmaxy(stdscr)
// logic error to call from option_table constructor? inability to guarantee initscr runs first in general
bool option_table::set_screen_options(int x, int y)
{
	if (default_int(OPT_SCREENWIDTH) > x || default_int(OPT_VIEW) > y) {
		// Fail-safe to C:Whales values, even though they don't fit.
		options[OPT_VIEW] = default_int(OPT_VIEW);
		options[OPT_PANELX] = default_int(OPT_PANELX);
		options[OPT_SCREENWIDTH] = default_int(OPT_SCREENWIDTH);
		return false;
	}
	int ub_view = x - default_int(OPT_PANELX);
	if (y < ub_view) ub_view = y;
	if (0 == ub_view % 2) --ub_view;

	options[OPT_VIEW] = ub_view;
	options[OPT_PANELX] = x - ub_view;
	options[OPT_SCREENWIDTH] = x;
	return true;
}

const char* option_name(option_key key)
{
 switch (key) {
  case OPT_USE_CELSIUS:		return "Use Celsius";
  case OPT_USE_METRIC_SYS: return "Use Metric System";
  case OPT_FORCE_YN:		return "Force Y/N in prompts";
  case OPT_NO_CBLINK:		return "No Bright Backgrounds";
  case OPT_24_HOUR:			return "24 Hour Time";
  case OPT_SNAP_TO_TARGET:	return "Snap to Target";
  case OPT_SAFEMODE:		return "Safemode on by default";
  case OPT_AUTOSAFEMODE:	return "Auto-Safemode on by default";
  case OPT_NPCS:			return "Generate NPCs";
  case OPT_LOAD_TILES:		return "use tileset (requires restart)";
  case OPT_FONT_HEIGHT:		return "Font height (requires restart)";
  case OPT_EXTRA_MARGIN:	return "Extra bottom-right margin (requires restart)";
  case OPT_FONT:	return "Font (requires restart)";
  default:			return "Unknown Option (BUG)";
 }
}

void save_options()
{
 DECLARE_AND_ACID_OPEN(std::ofstream, fout, OPT_FILE, return;)
 fout << get_JSON_opts() << std::endl;
 OFSTREAM_ACID_CLOSE(fout, OPT_FILE)
}
#undef OPT_FILE
