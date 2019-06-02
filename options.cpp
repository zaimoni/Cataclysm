#include "options.h"
#include "output.h"
#include "JSON.h"

#include <fstream>

using cataclysm::JSON;

static bool default_true(option_key opt)
{
	switch (opt)
	{
	case OPT_FORCE_YN:
	case OPT_SAFEMODE: return true;
	default: return false;
	}
}

static const char* JSON_key(option_key opt)
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
	for (int i = 0; i < NUM_OPTION_KEYS; i++) options[i] = default_true((option_key)i);
};

option_table& option_table::get()
{
	static option_table* x = 0;
	if (!x) {
		x = new option_table();
		x->load();
	}
	return *x;
}

option_key lookup_option_key(std::string id)
{
	if (id == "use_celsius") return OPT_USE_CELSIUS;
	if (id == "use_metric_system") return OPT_USE_METRIC_SYS;
	if (id == "force_capital_yn") return OPT_FORCE_YN;
	if (id == "no_bright_backgrounds") return OPT_NO_CBLINK;
	if (id == "24_hour") return OPT_24_HOUR;
	if (id == "snap_to_target") return OPT_SNAP_TO_TARGET;
	if (id == "safemode") return OPT_SAFEMODE;
	if (id == "autosafemode") return OPT_AUTOSAFEMODE;
	return OPT_NULL;
}

bool option_is_bool(option_key id)
{
#if 0
	switch (id) {
	default: return true;
	}
#endif
	return true;
}

std::string options_header()
{
	return "\
# This is the options file.  It works similarly to keymap.txt: the format is\n\
# <option name> <option value>\n\
# <option value> may be any number, positive or negative.  If you use a\n\
# negative sign, do not put a space between it and the number.\n\
#\n\
# If # is at the start of a line, it is considered a comment and is ignored.\n\
# In-line commenting is not allowed.  I think.\n\
#\n\
# If you want to restore the default options, simply delete this file.\n\
# A new options.txt will be created next time you play.\n\
\n";
}

void create_default_options()
{
	std::ofstream fout;
	fout.open("data/options.txt");
	if (!fout.is_open()) return;

	fout << options_header() << "\n\
# If true, use C not F\n\
use_celsius F\n\
# If true, use Km/h not mph\
use_metric_system F\n\
# If true, y/n prompts are case-sensitive, y and n are not accepted\n\
force_capital_yn T\n\
# If true, bright backgrounds are not used--some consoles are not compatible\n\
no_bright_backgrounds F\n\
# If true, use military time, not AM/PM\n\
24_hour F\n\
# If true, automatically follow the crosshair when firing/throwing\n\
snap_to_target F\n\
# If true, safemode will be on after starting a new game or loading\n\
safemode T\n\
# If true, auto-safemode will be on after starting a new game or loading\n\
autosafemode F\n\
";
	fout.close();
}

// formerly void load_options()  As a singleton we handle this on first access
void option_table::load()
{
 std::ifstream fin;
 fin.open("data/options.txt");
 if (!fin.is_open()) {
  fin.close();
  create_default_options();	// our constructor works, so this doesn't need reading.
  return;
 }

 while (!fin.eof()) {
  std::string id;
  fin >> id;
  if (id == "") getline(fin, id); // Empty line, chomp it
  else if (id[0] == '#') // # indicates a comment
   getline(fin, id);
  else {
   option_key key = lookup_option_key(id);
   if (key == OPT_NULL) {
    debugmsg("Bad option: %s", id.c_str());
    getline(fin, id);
   } else if (option_is_bool(key)) {
    std::string val;
    fin >> val;
	options[key] = (val == "T") ? 1. : 0. ;
   } else {
    double val;
    fin >> val;
	options[key] = val;
   }
  }
  if (fin.peek() == '\n') getline(fin, id); // Chomp
 }
 fin.close();
}

const char* option_string(option_key key)
{
 switch (key) {
  case OPT_USE_CELSIUS:		return "use_celsius";
  case OPT_USE_METRIC_SYS: return "use_metric_system";
  case OPT_FORCE_YN:		return "force_capital_yn";
  case OPT_NO_CBLINK:		return "no_bright_backgrounds";
  case OPT_24_HOUR:		return "24_hour";
  case OPT_SNAP_TO_TARGET:	return "snap_to_target";
  case OPT_SAFEMODE:		return "safemode";
  case OPT_AUTOSAFEMODE:	return "autosafemode";
  default:			return 0;
 }
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
 fout.open("data/options.txt");
 if (!fout.is_open()) return;

 auto& OPTIONS = option_table::get();

 fout << options_header() << std::endl;
 for (int i = 1; i < NUM_OPTION_KEYS; i++) {
  option_key key = option_key(i);
  const auto label = option_string(key);
  if (!label) continue;
  fout << option_string(key) << " ";
  if (option_is_bool(key))
   fout << (OPTIONS[key] ? "T" : "F");
  else
   fout << OPTIONS[key];
  fout << "\n";
 }

 // prepare JSON output
 JSON opts;
 opts.set("use Celsius",OPTIONS[OPT_USE_CELSIUS] ? "true" : "false");
 opts.set("use SI i.e. metric units", OPTIONS[OPT_USE_METRIC_SYS] ? "true" : "false");
 opts.set("force Y/N", OPTIONS[OPT_FORCE_YN] ? "true" : "false");
 opts.set("no cblink", OPTIONS[OPT_NO_CBLINK] ? "true" : "false");
 opts.set("24 hour clock", OPTIONS[OPT_24_HOUR] ? "true" : "false");
 opts.set("snap to target", OPTIONS[OPT_SNAP_TO_TARGET] ? "true" : "false");
 opts.set("safe mode", OPTIONS[OPT_SAFEMODE] ? "true" : "false");
 opts.set("auto safe mode", OPTIONS[OPT_AUTOSAFEMODE] ? "true" : "false");
 opts.set("NPCs", "true");	// don't want to mess with the file-exist hack here
 opts.set("lang", "en");	// not used right now but we do want to allow configuring translations

 fout.close();
 fout.open("data/options.json");
 if (!fout.is_open()) return;
 fout << opts << std::endl;
}
