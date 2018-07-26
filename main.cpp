/* Main Loop for cataclysm */

#include "game.h"
#include "options.h"
#include "mapbuffer.h"
#include "file.h"
#include "JSON.h"
#include <time.h>
#include <fstream>
#include <iostream>
#include <exception>
#if (defined _WIN32 || defined WINDOWS)
#include <combaseapi.h>
#endif

using namespace cataclysm;

// case for function objects is here:
static bool scalar_on_hard_drive(const JSON& x)
{
	if (!x.is_scalar()) return false;
	if (x.empty()) return false;
	OS_dir working;
	const auto hash_tag = x.scalar().find('#');
	// we use a hashtag to cope with tilesheets.
	return working.exists((hash_tag == std::string::npos ? x.scalar() : std::string(x.scalar(), 0, hash_tag)).c_str());
}

static bool preload_image(const JSON& x)
{
	if (!x.is_scalar() || x.empty()) return false;
	return load_tile(x.scalar().c_str());
}

static void load_JSON(const char* const src, const char* const key, bool (ok)(const JSON&))
{
#ifdef OS_dir
	OS_dir working;
	if (!working.exists(src)) return;

	std::ifstream fin;
	fin.open(src);
	if (!fin.is_open()) return;
	try {
		JSON incoming(fin);
		fin.close();
		if (incoming.empty()) return;
		if (!incoming.destructive_grep(ok)) return;
		JSON::cache[key] = std::move(incoming);
	} catch (const std::exception& e) {
		fin.close();
		std::cerr << src << ": " << e.what();
	}
#endif
}

// goes into destructive_grep of a JSON array so being applied in reverse order
static bool load_tiles(const JSON& src)
{
	if (!src.is_scalar()) return false;
	std::ifstream fin;
	fin.open(src.scalar());
	if (!fin.is_open()) return false;
	try {
		JSON incoming(fin);
		fin.close();
		// key we need to know about here is tiles
		if (!incoming.become_key("tiles")) return false;
		if (JSON::object != incoming.mode()) return false;
		const std::vector<std::string> null_keys(incoming.grep(JSON::is_null).keys());
		incoming.unset(null_keys);
		incoming.destructive_grep(scalar_on_hard_drive);
		if (incoming.empty()) return false;
		if (JSON::cache.count("tiles")) {
			auto& tiles = JSON::cache["tiles"];
			tiles.unset(null_keys);
			tiles.destructive_merge(incoming);	// assumes reverse order of iteration
		} else JSON::cache["tiles"] = std::move(incoming);
		return true;
	}
	catch (const std::exception& e) {
		fin.close();
		std::cerr << src.scalar() << ": " << e.what();
		return false;
	}
}

int main(int argc, char *argv[])
{
#if (defined _WIN32 || defined WINDOWS)
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);	// probably should be in main()
	if (!SUCCEEDED(hr)) exit(EXIT_FAILURE);	// fail
#endif

 srand(time(NULL));

 // XXX want to load tiles before initscr; implies errors before initscr() go to C stderr or C stdout	\todo IMPLEMENT
 // want a stderr.txt as well (cf. Wesnoth 1.12- [went away in Wesnoth 1.14])

 // C:DDA retained the data directory, but has a very different layout
 // following code was based on one file/tile, but C:DDA has pre-prepared tilesheets

// final format is an object of key-id, value-image filepath pairs  To fully support C:DDA tilesheets, 
// we have to specify what order tilesets are to be checked in (array) and then merge the objects accordingly
 load_JSON("data/json/tilespec.json", "tilespec", scalar_on_hard_drive);	// expected format array of JSON files
 // expected format of the files named by tilespec are
 // key name: value is display name of tileset
 // key tiles: value is an object, keys are enumeration values, values are tile images.  For the DeonApocalypse tileset we need to
 // use hashtag notation, and possibly also a CSS-like syntax for specifying rotations and flips
 // so ....32.png[#index][!rotate|!flip]
 // index says which image in tilesheet to extract
 // ! notation says what operations to apply to the image
 if (JSON::cache.count("tilespec") && !JSON::cache["tilespec"].destructive_grep(load_tiles)) JSON::cache.erase("tilespec");

 // when we support mods, we load their tiles configuration from tiles.json as well (and check their filepaths are ok
 // value null could be used to unset a pre-existing value

// ncurses stuff
 initscr(); // Initialize ncurses

 // need some sort of screen to complete image preloading on Windows
 if (JSON::cache.count("tiles") && !JSON::cache["tiles"].destructive_grep(preload_image)) JSON::cache.erase("tiles");	// wires tiles to types
 flush_tilesheets();	// don't want to pay RAM overhead for tilesheets after we've extracted the tiles from them

 noecho();  // Don't echo keypresses
 cbreak();  // C-style breaks (e.g. ^C to SIGINT)
 keypad(stdscr, true); // Numpad is numbers
 init_colors(); // See color.cpp
 curs_set(0); // Invisible cursor

 rand();  // For some reason a call to rand() seems to be necessary to avoid
          // repetion.
 bool quit_game = false;
 game *g = new game;
 MAPBUFFER = mapbuffer(g);
 MAPBUFFER.load();
 load_options();
 do {
  g->setup();
  while (!g->do_turn());
  if (g->game_quit())
   quit_game = true;
 } while (!quit_game);
 MAPBUFFER.save();
 erase(); // Clear screen
 endwin(); // End ncurses
 system("clear"); // Tell the terminal to clear itself
#if (defined _WIN32 || defined WINDOWS)
 CoUninitialize();
#endif
 return 0;
}

