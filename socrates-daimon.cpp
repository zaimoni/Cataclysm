/* Main Loop for Socrates' Daimon, Cataclysm analysis utility */

#include "color.h"
#include "mtype.h"
#include "crafting.h"
#include "construction.h"
#include "item.h"
#include "output.h"
#include "ios_file.h"
#include "json.h"
#include "html.hpp"
#include "mapdata.h"
#include "pldata.h"
#include "mutation.h"
#include <stdexcept>
#include <stdlib.h>
#include <time.h>

#include <fstream>
#include <filesystem>
#include <algorithm>

using namespace cataclysm;

// from saveload.cpp
bool fromJSON(const JSON& _in, item& dest);
JSON toJSON(const item& src);

const char* JSON_key(material src);
const char* JSON_key(add_type src);

static void check_roundtrip_JSON(const item& src)
{
	item staging;
	if (!fromJSON(toJSON(src), staging)) throw std::logic_error("critical round-trip failure");
	if (src != staging) throw std::logic_error("round-trip was not invariant");
}

template<class T>
static void to_desc(const std::vector<T*>& src, std::map<std::string, std::string>& dest, std::map<std::string, int>& ids)
{
	static_assert(std::is_base_of_v<itype, T>);
	for (auto it : src) {
		item test(it, 0);
		auto name(test.tname());
		dest[name] = test.info(true);
		ids[std::move(name)] = it->id;
		if constexpr (std::is_same_v<T, it_comest> || std::is_same_v<T, it_software>) {
			auto test2 = test.in_its_container();
			if (test2.type != test.type) {
				name = test2.tname();
				dest[name] = test2.info(true);
				ids[std::move(name)] = test2.type->id;
			}
		}
	}
}

static const char* addiction_target(add_type cur)
{
	switch(cur) {
	case ADD_NULL:	return nullptr;
	case ADD_CIG:		return "Nicotine";
	case ADD_CAFFEINE:	return "Caffeine";	// technically also includes both theophylline and theobromine
	case ADD_ALCOHOL:	return "Alcohol";
	case ADD_SLEEP:	return "Sleeping Pills";	// benzodiazepenes or barbituates (ignore technical differences here)
	case ADD_PKILLER:	return "Opiates";
	case ADD_SPEED:	return "Amphetamine";
	case ADD_COKE:	return "Cocaine";
	case ADD_THC:	return "Marijuana";
	}
	throw std::logic_error("unhandled addiction type");
}

static const char* crafting_category(craft_cat cat)
{
	switch (cat)
	{
	case CC_WEAPON: return "weapon";
	case CC_FOOD: return "food";
	case CC_ELECTRONIC: return "electronic";
	case CC_ARMOR: return "armor";
	case CC_MISC: return "misc";
	default: throw std::logic_error("unhandled crafting category");
	};

}

static auto min_term_to_s(const std::vector<component>& src)
{
	std::string ret;
	for (decltype(auto) min_term : src) {
		const auto json = JSON_key(min_term.type);
		if (!json) continue;
		if (!ret.empty()) ret += ", OR ";
		if (0 < min_term.count) ret += "<nobr>"+std::to_string(min_term.count) + ' ';
		ret += json; // \todo hyperlink to item
		if (0 < min_term.count) ret += "</nobr>";
	}
	return ret;
}

static auto to_s(const std::vector<std::vector<component> >& src)
{
	if (src.empty()) throw std::logic_error("precondition: can only format non-empty");
	std::string ret;
	for (decltype(auto) min_term : src) {
		decltype(auto) line = min_term_to_s(min_term);
		if (line.empty()) continue;
		if (!ret.empty()) ret += "<br>\n";
		ret += line;
	}
	return ret;
}

static auto min_term_to_s(const std::vector<itype_id>& src)
{
	std::string ret;
	for (decltype(auto) min_term : src) {
		const auto json = JSON_key(min_term);
		if (!json) continue;
		if (!ret.empty()) ret += ", OR ";
		ret += json; // \todo hyperlink to item
	}
	return ret;
}

static auto to_s(const std::vector<std::vector<itype_id> >& src)
{
	if (src.empty()) throw std::logic_error("precondition: can only format non-empty");
	std::string ret;
	for (decltype(auto) min_term : src) {
		decltype(auto) line = min_term_to_s(min_term);
		if (line.empty()) continue;
		if (!ret.empty()) ret += "<br>\n";
		ret += line;
	}
	return ret;
}

static auto to_table(const std::vector<construction_stage>& src)
{
	static constexpr const char* table_headers[] = { "Terrain" , "Minutes", "Tools", "Components" };
	html::tag table("table");

	{
	// C++20: extract this using std::range
	html::tag table_header("tr");
	table_header.set("align", "center");
	for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
	table.append(std::move(table_header));
	}

	html::tag cell("td");
	html::tag table_row("tr");
	table_row.set("align", "left");
	table_row.set("valign", "top");
	for (decltype(auto) th : table_headers) table_row.append(cell);

	for (decltype(auto) c_stage : src) {
		if (const auto json = JSON_key(c_stage.terrain)) table_row[0].append(html::tag::wrap(json));
		table_row[1].append(std::to_string(c_stage.time));
		if (!c_stage.tools.empty()) table_row[2].append(to_s(c_stage.tools));
		if (!c_stage.components.empty()) table_row[3].append(to_s(c_stage.components));

		table.append(table_row);
		for (decltype(auto) tr : table_row) tr.clear();
	}

	return table;
}

static auto typicalMenuLink(std::string&& li_id, std::string&& a_name, std::string&& a_url)
{
	html::tag ret_li("li");

	std::string link_id(li_id + "_link");
	ret_li.set("id", std::move(li_id));

	html::tag a_tag("a", std::move(a_name));
	a_tag.set("href", std::move(a_url));
	a_tag.set("id", std::move(link_id));
	ret_li.append(std::move(a_tag));

	return ret_li;
}

static auto swapDOM(const std::string& selector, html::tag& src, html::tag&& node)
{
	auto subheader = src.querySelector(selector);
#ifndef NDEBUG
	if (!subheader) throw std::logic_error("missing update target");
#endif
	auto backup(std::move(*subheader));
	*subheader = std::move(node);
	return std::pair(subheader, std::move(backup));
}

// HTML
static auto wrap_in_anchor(const std::string& src, const char* const anchor)
{
	if (!anchor || !anchor[0]) return html::tag::wrap(src);
	auto ret = html::tag("a", src);
	ret.set("name", anchor);
	return ret;
}

static auto link_to(const std::string& src, const char* const page, const char* const anchor)
{
	if (!anchor || !anchor[0]) return html::tag::wrap(src);
	if (!page || !page[0]) return html::tag::wrap(src);
	auto ret = html::tag("a", src);
	ret.set("href", std::string(page) + '#' + anchor);
	return ret;
}

static void inline_script(html::to_text& page, const char* const src)
{
	// wrapper for inline scripts
	static const html::tag _script("script");
	// would be better to punt to OS but we're in-progress
	if (FILE* in = fopen(src, "r")) {
		page.start_print(_script);
		page.raw_copy(in);
		page.end_print();
	}
}

// rely on redefinition being an error
#define SKILLS_HTML "skills.html"

std::string skill_link(skill sk)
{
	const auto sk_name = skill_name(sk);
	return link_to(sk_name, SKILLS_HTML, html::encode_id(sk_name).c_str()).to_s();
}

int main(int argc, char *argv[])
{
	// these do not belong here
	static const std::string CSS_sep("; ");
	static const std::string attr_valign("valign");
	static const std::string val_top("top");
	static const std::string attr_align("align");
	static const std::string val_center("center");
	static const std::string val_left("left");
	static const std::string attr_style("style");
	static const std::string val_thin_border("border: solid 1px black");
	static const std::string val_left_align("float:left");
	static const std::string val_list_none("list-style: none");

	{  // force html subdirectory to exist (function extraction target)
	std::error_code ec;
	const std::filesystem::path target("html");
	if (!std::filesystem::exists(target, ec) && !ec) create_directory(target, ec);	// do not want exception-throwing here
	}

	// \todo parse command line options

	srand(time(nullptr));

	// ncurses stuff
	initscr(); // Initialize ncurses
	noecho();  // Don't echo keypresses
	cbreak();  // C-style breaks (e.g. ^C to SIGINT)
	keypad(stdscr, true); // Numpad is numbers
	init_colors(); // See color.cpp
	curs_set(0); // Invisible cursor

	// roundtrip tests to schedule: mon_id, itype_id, item_flag, technique_id, art_charge, art_effect_active, art_effect_passive

	// bootstrap
	mutation_branch::init();
	item::init();
	mtype::init();
//	mtype::init_items();     need to do this but at a later stage
	trap::init();
	recipe::init();
	constructable::init();

	// item HTML setup
	const html::tag _html("html");	// stage-printed
	// head tag will be mostly "common" between pages, but title will need adjusting
	html::tag _head("head");
	_head.append(html::tag("title"));
	// ....
	html::tag* const _title = _head.querySelector("title");
#ifndef NDEBUG
	if (!_title) throw std::logic_error("title tag AWOL");
#endif
	// this is where the JS for any dynamic HTML, etc. has to be inlined
	html::tag _body("body");	// stage-printed
	// navigation sidebar will be "common", at least between page types
	html::tag global_nav("ul");
	global_nav.set(attr_style, val_thin_border + CSS_sep + val_left_align + CSS_sep + val_list_none + CSS_sep + "margin:0px; padding:10px");

#define HTML_DIR "html/"

#define HOME_HTML "index.html"
#define HOME_ID "home"
#define HOME_LINK_NAME "Home"

#define ITEMS_HTML "items.html"
#define ITEMS_ID "items"
#define ITEMS_LINK_NAME "Items"

#define NAVIGATION_HTML "navigation.html"
#define NAVIGATION_ID "navigation"
#define NAVIGATION_LINK_NAME "Navigation"

#define STATUS_HTML "status.html"
#define STATUS_ID "status"
#define STATUS_LINK_NAME "Status"

	html::tag working_li("li");

	// wrap up global navigation menu
	global_nav.append(typicalMenuLink(HOME_ID, HOME_LINK_NAME, "./" HOME_HTML));
	global_nav.append(typicalMenuLink(ITEMS_ID, ITEMS_LINK_NAME, "./" ITEMS_HTML));
	global_nav.append(typicalMenuLink(NAVIGATION_ID, NAVIGATION_LINK_NAME, "./" NAVIGATION_HTML));
	global_nav.append(typicalMenuLink(STATUS_ID, STATUS_LINK_NAME, "./" STATUS_HTML));

	auto home_revert = swapDOM("#" HOME_ID, global_nav, html::tag("b", HOME_LINK_NAME));

#define HTML_TARGET HTML_DIR HOME_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
		html::to_text page(out);
		page.start_print(_html);
		_title->append(html::tag::wrap("Cataclysm:Z " HOME_LINK_NAME));
		page.print(_head);
		_title->clear();
		page.start_print(_body);
		page.print(global_nav);
		while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

	*home_revert.first = std::move(home_revert.second);

	// full item navigation menu
	html::tag item_point("li");
	item_point.set("id", ITEMS_ID);
	{
	html::tag a_tag("a", ITEMS_LINK_NAME);
	a_tag.set("href", "./" ITEMS_HTML);
	a_tag.set("id", ITEMS_ID "_link");
	item_point.append(std::move(a_tag));
	}

	html::tag item_nav("ul");
	item_nav.set(attr_style, val_list_none);

#define AMMO_HTML "ammo.html"
#define AMMO_ID "ammo"
#define AMMO_LINK_NAME "Ammo"
#define ARMOR_HTML "armor.html"
#define ARMOR_ID "armor"
#define ARMOR_LINK_NAME "Armor"
#define BIONICS_HTML "bionics.html"
#define BIONICS_ID "bionics"
#define BIONICS_LINK_NAME "Bionics"
#define BOOKS_HTML "books.html"
#define BOOKS_ID "books"
#define BOOKS_LINK_NAME "Books"
#define CONTAINERS_HTML "containers.html"
#define CONTAINERS_ID "containers"
#define CONTAINERS_LINK_NAME "Containers"
#define DRINKS_HTML "drinks.html"
#define DRINKS_ID "drinks"
#define DRINKS_LINK_NAME "Drinks"
#define EDIBLE_HTML "edible.html"
#define EDIBLE_ID "edible"
#define EDIBLE_LINK_NAME "Edible"
#define FUEL_HTML "fuel.html"
#define FUEL_ID "fuel"
#define FUEL_LINK_NAME "Fuel"
#define GUNS_HTML "guns.html"
#define GUNS_ID "guns"
#define GUNS_LINK_NAME "Guns"
#define GUN_MODS_HTML "gun_mods.html"
#define GUN_MODS_ID "gun_mods"
#define GUN_MODS_LINK_NAME "Gun Mods"
#define MARTIAL_ARTS_HTML "ma_styles.html"
#define MARTIAL_ARTS_ID "ma_styles"
#define MARTIAL_ARTS_LINK_NAME "Martial Arts"
#define MACGUFFIN_HTML "macguffin.html"
#define MACGUFFIN_ID "macguffin"
#define MACGUFFIN_LINK_NAME "MacGuffins"
#define PHARMA_HTML "pharma.html"
#define PHARMA_ID "pharma"
#define PHARMA_LINK_NAME "Pharmaceuticals and Medical Supplies"
#define SOFTWARE_HTML "software.html"
#define SOFTWARE_ID "software"
#define SOFTWARE_LINK_NAME "Software"
#define TOOLS_HTML "tools.html"
#define TOOLS_ID "tools"
#define TOOLS_LINK_NAME "Tools"
#define UNCLASSIFIED_HTML "unclassified.html"
#define UNCLASSIFIED_ID "unclassified"
#define UNCLASSIFIED_LINK_NAME "Unclassified"

	item_nav.append(typicalMenuLink(AMMO_ID, AMMO_LINK_NAME, "./" AMMO_HTML));
	item_nav.append(typicalMenuLink(ARMOR_ID, ARMOR_LINK_NAME, "./" ARMOR_HTML));
	item_nav.append(typicalMenuLink(BIONICS_ID, BIONICS_LINK_NAME, "./" BIONICS_HTML));
	item_nav.append(typicalMenuLink(BOOKS_ID, BOOKS_LINK_NAME, "./" BOOKS_HTML));
	item_nav.append(typicalMenuLink(CONTAINERS_ID, CONTAINERS_LINK_NAME, "./" CONTAINERS_HTML));
	item_nav.append(typicalMenuLink(DRINKS_ID, DRINKS_LINK_NAME, "./" DRINKS_HTML));
	item_nav.append(typicalMenuLink(EDIBLE_ID, EDIBLE_LINK_NAME, "./" EDIBLE_HTML));
	item_nav.append(typicalMenuLink(FUEL_ID, FUEL_LINK_NAME, "./" FUEL_HTML));
	item_nav.append(typicalMenuLink(GUNS_ID, GUNS_LINK_NAME, "./" GUNS_HTML));
	item_nav.append(typicalMenuLink(GUN_MODS_ID, GUN_MODS_LINK_NAME, "./" GUN_MODS_HTML));
	item_nav.append(typicalMenuLink(MARTIAL_ARTS_ID, MARTIAL_ARTS_LINK_NAME, "./" MARTIAL_ARTS_HTML));
	item_nav.append(typicalMenuLink(MACGUFFIN_ID, MACGUFFIN_LINK_NAME, "./" MACGUFFIN_HTML));
	item_nav.append(typicalMenuLink(PHARMA_ID, PHARMA_LINK_NAME, "./" PHARMA_HTML));
	item_nav.append(typicalMenuLink(SOFTWARE_ID, SOFTWARE_LINK_NAME, "./" SOFTWARE_HTML));
	item_nav.append(typicalMenuLink(TOOLS_ID, TOOLS_LINK_NAME, "./" TOOLS_HTML));
	item_nav.append(typicalMenuLink(UNCLASSIFIED_ID, UNCLASSIFIED_LINK_NAME, "./" UNCLASSIFIED_HTML));

	working_li.unset("id");

	item_point.append(item_nav);

	// set up items submenu
	home_revert = swapDOM("#" ITEMS_ID, global_nav, std::move(item_point));

#define HTML_TARGET HTML_DIR ITEMS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " ITEMS_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			{
			auto revert = swapDOM("#" ITEMS_ID "_link", global_nav, html::tag("b", ITEMS_LINK_NAME));
			page.print(global_nav);
			*revert.first = std::move(revert.second);
			}
			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

	const html::tag _data_table("table");	// stage-printed

	std::vector<const it_ammo*> ammunition;
	std::vector<const it_armor*> armor;
	std::vector<const it_bionic*> bionics;
	std::vector<const it_book*> books;
	std::vector<const it_container*> containers;
	std::vector<const it_comest*> drinks;
	std::vector<const it_comest*> edible;
	std::vector<const it_ammo*> fuel;	// XXX conflation in type system
	std::vector<const it_gun*> guns;
	std::vector<const it_gunmod*> gun_mods;
	std::vector<const it_style*> ma_styles;	// martial arts styles
	std::vector<const it_macguffin*> macguffins;
	std::vector<const it_comest*> pharma;
	std::vector<const it_software*> software;
	std::vector<const it_tool*> tools;

	std::vector<itype*> unclassified;

	std::vector<std::pair<add_type, const it_comest*> > xref_addictions;

	std::map<std::string, std::string> name_desc;
	std::map<std::string, int> name_id;

	// item type scan
	auto ub = item::types.size();
	while (0 < ub) {
		bool will_handle_as_html = false;
		const auto it = item::types[--ub];
		if (!it) throw std::logic_error("null item type");
        if (it->id != ub) throw std::logic_error("reverse lookup failure: " + std::to_string(it->id)+" for " + std::to_string(ub));
        // item material reality checks
		if (!it->m1 && it->m2) throw std::logic_error("non-null secondary material for null primary material");
		if (const auto style = it->is_style()) {
			// constructor hard-coding (specification)
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

			ma_styles.push_back(style);
			will_handle_as_html = true;
		} else if (const auto ammo = it->is_ammo()) {
			// constructor hard-coding (specification)
			if (it->m2) throw std::logic_error("unexpected secondary material");
			if (0 != it->melee_cut) throw std::logic_error("unexpected cutting damage");
			if (0 != it->m_to_hit) throw std::logic_error("unexpected melee accuracy");
			// UPS power source responsible for itm_charge_shot
			if (itm_charge_shot != it->id && !ammo->type) throw std::logic_error("ammo doesn't actually provide ammo");
			// macro hard-coding (may want to change these but currently invariant)
			// ammo and fuel are conflated in the type system.
			// 2020-03-12: All fuel is material type LIQUID (this likely will be adjusted;
			// it would make more sense for this to be a phase than a material type)
			if (LIQUID == it->m1) {
				if (1 != it->volume) throw std::logic_error("unexpected volume");
				if (1 != it->weight) throw std::logic_error("unexpected weight");
				if (0 != it->melee_dam) throw std::logic_error("unexpected melee damage");
				if ('~' != it->sym) throw std::logic_error("unexpected symbol");
				fuel.push_back(ammo);
				will_handle_as_html = true;
			} else {
				if (1 != it->melee_dam) throw std::logic_error("unexpected melee damage");
				if ('=' != it->sym) throw std::logic_error("unexpected symbol");
				// exceptions are for plasma state
				if (!it->m1 && itm_bio_fusion != it->id && itm_charge_shot!=it->id) throw std::logic_error("null material for ammo");
				ammunition.push_back(ammo);
				will_handle_as_html = true;
			}
		} else if (const auto sw = it->is_software()) {
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
			software.push_back(sw);
			will_handle_as_html = true;
		} else if (!it->m1) {
			if (itm_toolset < it->id && num_items != it->id && num_all_items != it->id) {
                throw std::logic_error("unexpected null material: "+std::to_string(it->id));
            }
		}

		if (const auto cont = it->is_container()) {
			containers.push_back(cont);
			will_handle_as_html = true;
		} else if (const auto book = it->is_book()) {
			books.push_back(book);
			will_handle_as_html = true;
		} else if (const auto food = it->is_food()) {
			// actually, all of food/drink/medicine -- should split this into 3 pages
			if (LIQUID == it->m1) {
				drinks.push_back(food);
				if (food->addict) {
					if (!food->add) throw std::logic_error(it->name + ": addicting drink without addiction");
					xref_addictions.push_back(std::pair(food->add, food));
				}
			}
			else if (FLESH != it->m1 && VEGGY != it->m1 && PAPER != it->m1 && POWDER != it->m1) {
				pharma.push_back(food);
				if (food->addict) {
					if (!food->add) throw std::logic_error(it->name + ": addicting drink without addiction");
					xref_addictions.push_back(std::pair(food->add, food));
				}
			} else if (food->addict) {
				if (!food->add) throw std::logic_error(it->name + ": addicting food without addiction");
				pharma.push_back(food);
				xref_addictions.push_back(std::pair(food->add, food));
			} else edible.push_back(food);
			if (!food->addict && food->add) throw std::logic_error(it->name + ": non-addicting addiction");
			// \todo the addiction consistency tests should be constructor, but itypedef.cpp doesn't use assert or SUCCEED_OR_DIE
			will_handle_as_html = true;
		// these two implicitly cover artifacts
		} else if (const auto armour = it->is_armor()) {
			armor.push_back(armour);
			will_handle_as_html = true;
		} else if (const auto tool = it->is_tool()) {
			tools.push_back(tool);
			will_handle_as_html = true;
		} else if (const auto gun = it->is_gun()) {
			if (!gun) throw std::logic_error(it->name + ": static cast to gun failed");

			guns.push_back(gun);
			will_handle_as_html = true;
		} else if (const auto mod = it->is_gunmod()) {
			gun_mods.push_back(mod);
			will_handle_as_html = true;
		} else if (const auto bionic = it->is_bionic()) {
			if (!bionic) throw std::logic_error(it->name + ": static cast to bionic failed");

			bionics.push_back(bionic);
			will_handle_as_html = true;
		} else if (const auto macguffin = it->is_macguffin()) {
			macguffins.push_back(macguffin);
			will_handle_as_html = true;
		} else if (itm_corpse == it->id) continue;	// corpses need their own testing path
		else if (!will_handle_as_html) {
			unclassified.push_back(it);
			will_handle_as_html = true;
		}
		// at this point will_handle_as_html is constant true

		// check what happens when item is created.
			item test(it, 0);
			name_id[test.tname()] = it->id;
			if (test.my_preferred_container()) {	// i.e., can create in own container
				auto test2 = test.in_its_container();
				if (test2.type != test.type) check_roundtrip_JSON(test2);
			}
			if (num_items != it->id && itm_null != it->id) check_roundtrip_JSON(test);
	}

	if (!ma_styles.empty()) {
		to_desc(ma_styles, name_desc, name_id);
#define HTML_TARGET HTML_DIR MARTIAL_ARTS_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " MARTIAL_ARTS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" MARTIAL_ARTS_ID, global_nav, html::tag("b", MARTIAL_ARTS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!armor.empty()) {
		to_desc(armor, name_desc, name_id);
#define HTML_TARGET HTML_DIR ARMOR_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " ARMOR_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" ARMOR_ID, global_nav, html::tag("b", ARMOR_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!containers.empty()) {
		to_desc(containers, name_desc, name_id);
#define HTML_TARGET HTML_DIR CONTAINERS_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " CONTAINERS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" CONTAINERS_ID, global_nav, html::tag("b", CONTAINERS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!books.empty()) {
		to_desc(books, name_desc, name_id);
#define HTML_TARGET HTML_DIR BOOKS_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " BOOKS_LINK_NAME));
				page.print(_head);
				_title->clear();

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_body);
				{
				auto revert = swapDOM("#" BOOKS_ID, global_nav, html::tag("b", BOOKS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!drinks.empty()) {
		to_desc(drinks, name_desc, name_id);
#define HTML_TARGET HTML_DIR DRINKS_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " DRINKS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" DRINKS_ID, global_nav, html::tag("b", DRINKS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

// duplicate definition, for hyperlinking
#define ADDICTIONS_HTML "addictions.html"

	// \todo need to document addictions related to these
	if (!pharma.empty()) {
		to_desc(pharma, name_desc, name_id);
#define HTML_TARGET HTML_DIR PHARMA_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " PHARMA_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" PHARMA_ID, global_nav, html::tag("b", PHARMA_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material", "Addiction" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						auto med = dynamic_cast<it_comest*>(item::types.at(name_id[x.first]));
						if (!med) continue;	// in container
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						if (auto med = dynamic_cast<it_comest*>(item::types.at(name_id[x.first]))) {
							if (auto add = addiction_target(med->add)) tr_alias[3]->append(link_to(add, ADDICTIONS_HTML, JSON_key(med->add)));
						} else throw std::logic_error(x.first+" not really consumable");
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!edible.empty()) {
		to_desc(edible, name_desc, name_id);
#define HTML_TARGET HTML_DIR EDIBLE_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " EDIBLE_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" EDIBLE_ID, global_nav, html::tag("b", EDIBLE_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!guns.empty()) {
		to_desc(guns, name_desc, name_id);
#define HTML_TARGET HTML_DIR GUNS_HTML

		// \todo cross-link to what it reloads, etc.
		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " GUNS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" GUNS_ID, global_nav, html::tag("b", GUNS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	// \todo this may warrant a full planner, not just static cross-references
	if (!gun_mods.empty()) {
		to_desc(gun_mods, name_desc, name_id);
#define HTML_TARGET HTML_DIR GUN_MODS_HTML

		// \todo cross-link to what it reloads, etc.
		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " GUN_MODS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" GUN_MODS_ID, global_nav, html::tag("b", GUN_MODS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	// \todo this should cross-link to the ranged weapons they fit
	if (!ammunition.empty()) {
		to_desc(ammunition, name_desc, name_id);
#define HTML_TARGET HTML_DIR AMMO_HTML

		// XXX AT_ARROW must have range at least 10 to avoid breaking the longbow
		// \todo make this fully computed
		for (decltype(auto) x : ammunition) {
			if (AT_ARROW == x->type && 10 > x->range) throw std::logic_error("longbow cannot handle weak arrows");
		}

		// \todo cross-link to what it reloads, etc.
		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " AMMO_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" AMMO_ID, global_nav, html::tag("b", AMMO_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!fuel.empty()) {
		to_desc(fuel, name_desc, name_id);
#define HTML_TARGET HTML_DIR FUEL_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " FUEL_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" FUEL_ID, global_nav, html::tag("b", FUEL_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!tools.empty()) {
		to_desc(tools, name_desc, name_id);
#define HTML_TARGET HTML_DIR TOOLS_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " TOOLS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" TOOLS_ID, global_nav, html::tag("b", TOOLS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!bionics.empty()) {
		to_desc(bionics, name_desc, name_id);
#define HTML_TARGET HTML_DIR BIONICS_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " BIONICS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" BIONICS_ID, global_nav, html::tag("b", BIONICS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!software.empty()) {
		to_desc(software, name_desc, name_id);
#define HTML_TARGET HTML_DIR SOFTWARE_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " SOFTWARE_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" SOFTWARE_ID, global_nav, html::tag("b", SOFTWARE_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!macguffins.empty()) {
		to_desc(macguffins, name_desc, name_id);
#define HTML_TARGET HTML_DIR MACGUFFIN_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " MACGUFFIN_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" MACGUFFIN_ID, global_nav, html::tag("b", MACGUFFIN_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}

				static constexpr const char* table_headers[] = { "Name", "Description", "Material" };
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	if (!unclassified.empty()) {
		to_desc(unclassified, name_desc, name_id);
#define HTML_TARGET HTML_DIR UNCLASSIFIED_HTML

		if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " UNCLASSIFIED_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
				auto revert = swapDOM("#" UNCLASSIFIED_ID, global_nav, html::tag("b", UNCLASSIFIED_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
				}
				page.start_print(_data_table);

				static constexpr const char* table_headers[] = { "Name" , "Description" , "Material" };
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					for (decltype(auto) th : table_headers) table_row.append(cell);
					decltype(auto) tr_alias = table_row.alias();
					tr_alias[1]->append(html::tag("pre"));
					tr_alias[1] = tr_alias[1]->querySelector("pre");

					for (const auto& x : name_desc) {
						tr_alias[0]->append(html::tag::wrap(x.first));
						tr_alias[1]->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) tr_alias[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						for (decltype(auto) tr : tr_alias) tr->clear();
					}
				}

				while (page.end_print());
			}
			unlink(HTML_TARGET);
			rename(HTML_TARGET ".tmp", HTML_TARGET);
		}

#undef HTML_TARGET
		decltype(name_desc) discard;
		name_desc.swap(discard);
	}

	*home_revert.first = std::move(home_revert.second);	// items page family handled

	// full terrain navigation menu
	html::tag mapnav_point("li");
	mapnav_point.set("id", NAVIGATION_ID);
	{
		html::tag a_tag("a", NAVIGATION_LINK_NAME);
		a_tag.set("href", "./" NAVIGATION_HTML);
		a_tag.set("id", NAVIGATION_ID "_link");
		mapnav_point.append(std::move(a_tag));
	}

	html::tag mapnav_nav("ul");
	mapnav_nav.set(attr_style, val_list_none);

#define FIELDS_HTML "fields.html"
#define FIELDS_ID "fields"
#define FIELDS_LINK_NAME "Fields"
#define TERRAIN_HTML "terrain.html"
#define TERRAIN_ID "terrain"
#define TERRAIN_LINK_NAME "Terrain"
#define TRAPS_HTML "traps.html"
#define TRAPS_ID "traps"
#define TRAPS_LINK_NAME "Traps"

	mapnav_nav.append(typicalMenuLink(FIELDS_ID, FIELDS_LINK_NAME, "./" FIELDS_HTML));
	mapnav_nav.append(typicalMenuLink(TERRAIN_ID, TERRAIN_LINK_NAME, "./" TERRAIN_HTML));
	mapnav_nav.append(typicalMenuLink(TRAPS_ID, TRAPS_LINK_NAME, "./" TRAPS_HTML));

	mapnav_point.append(mapnav_nav);

	// set up mapnav submenu
	home_revert = swapDOM("#" NAVIGATION_ID, global_nav, std::move(mapnav_point));

#define HTML_TARGET HTML_DIR NAVIGATION_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
		html::to_text page(out);
		page.start_print(_html);
		_title->append(html::tag::wrap("Cataclysm:Z " NAVIGATION_LINK_NAME));
		page.print(_head);
		_title->clear();
		page.start_print(_body);
		{
		auto revert = swapDOM("#" NAVIGATION_ID "_link", global_nav, html::tag("b", NAVIGATION_LINK_NAME));
		page.print(global_nav);
		*revert.first = std::move(revert.second);
		}
		while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR FIELDS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
		html::to_text page(out);
		page.start_print(_html);
		_title->append(html::tag::wrap("Cataclysm:Z " FIELDS_LINK_NAME));
		page.print(_head);
		_title->clear();
		page.start_print(_body);
		{
		auto revert = swapDOM("#" FIELDS_ID "_link", global_nav, html::tag("b", FIELDS_LINK_NAME));
		page.print(global_nav);
		*revert.first = std::move(revert.second);
		}

		static constexpr const char* table_headers[] = { "Name", "ASCII", "Transparent?", "Dangerous?" };
		page.start_print(_data_table);
		// actual content
		{
			html::tag table_header("tr");
			table_header.set(attr_align, val_center);
			for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
			page.print(table_header);
		}

		{
			static const std::string yes("yes");
			static const std::string color("color: ");
			static const std::string background("; background-color:");
			html::tag cell("td");
			html::tag table_row("tr");
			table_row.set(attr_align, val_left);
			for (decltype(auto) th : table_headers) table_row.append(cell);

			size_t ub = num_fields;
			const char* css_fg = nullptr;
			const char* css_bg = nullptr;
			while (0 < --ub) {
				const field_t& x = field::list[ub];
				table_row[0].append(html::tag::wrap(x.name[0]));
				if (to_css_colors(x.color[0], css_fg, css_bg)) {
					html::tag colorize("span", std::string(1, x.sym));
					colorize.set("style", color+ css_fg + background + css_bg);
					table_row[1].append(std::move(colorize));
				} else {
					table_row[1].append(html::tag::wrap(std::string(1, x.sym)));
				}
				table_row[2].append(html::tag::wrap(x.transparent[0] ? "yes" : ""));
				table_row[3].append(html::tag::wrap(x.dangerous[0] ? "yes" : ""));
				page.print(table_row);
				for (decltype(auto) tr : table_row) tr.clear();
				table_row[0].append(html::tag::wrap(x.name[1]));
				if (to_css_colors(x.color[1], css_fg, css_bg)) {
					html::tag colorize("span", std::string(1, x.sym));
					colorize.set("style", color + css_fg + background + css_bg);
					table_row[1].append(std::move(colorize));
				} else {
					table_row[1].append(html::tag::wrap(std::string(1, x.sym)));
				}
				table_row[2].append(html::tag::wrap(x.transparent[1] ? "yes" : ""));
				table_row[3].append(html::tag::wrap(x.dangerous[1] ? "yes" : ""));
				page.print(table_row);
				for (decltype(auto) tr : table_row) tr.clear();
				table_row[0].append(html::tag::wrap(x.name[2]));
				if (to_css_colors(x.color[2], css_fg, css_bg)) {
					html::tag colorize("span", std::string(1, x.sym));
					colorize.set("style", color + css_fg + background + css_bg);
					table_row[1].append(std::move(colorize));
				} else {
					table_row[1].append(html::tag::wrap(std::string(1, x.sym)));
				}
				table_row[2].append(html::tag::wrap(x.transparent[2] ? "yes" : ""));
				table_row[3].append(html::tag::wrap(x.dangerous[2] ? "yes" : ""));
				page.print(table_row);
				for (decltype(auto) tr : table_row) tr.clear();
			}
		}

		while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR TERRAIN_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
		html::to_text page(out);
		page.start_print(_html);
		_title->append(html::tag::wrap("Cataclysm:Z " TERRAIN_LINK_NAME));
		page.print(_head);
		_title->clear();
		page.start_print(_body);
		{
		auto revert = swapDOM("#" TERRAIN_ID "_link", global_nav, html::tag("b", TERRAIN_LINK_NAME));
		page.print(global_nav);
		*revert.first = std::move(revert.second);
		}

		static constexpr const char* table_headers[] = { "Name", "ASCII", "time cost", "Trap?" };
		page.start_print(_data_table);
		// actual content
		{
			html::tag table_header("tr");
			table_header.set(attr_align, val_center);
			for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
			page.print(table_header);
		}

		{
			static const std::string color("color: ");
			static const std::string background("; background-color:");
			html::tag cell("td");
			html::tag table_row("tr");
			table_row.set(attr_align, val_left);
			for (decltype(auto) th : table_headers) table_row.append(cell);

			size_t ub = num_terrain_types;
			const char* css_fg = nullptr;
			const char* css_bg = nullptr;
			while (0 < --ub) {
				const ter_t& x = ter_t::list[ub];
				/*
	unsigned long flags;// : num_t_flags;	// expanding this would be useful, but would be new support
				*/
				table_row[0].append(html::tag::wrap(x.name));
				if (to_css_colors(x.color, css_fg, css_bg)) {
					html::tag colorize("span", std::string(1, x.sym));
					colorize.set("style", color+ css_fg + background + css_bg);
					table_row[1].append(std::move(colorize));
				} else {
					table_row[1].append(html::tag::wrap(std::string(1, x.sym)));
				}
				if (x.movecost) table_row[2].append(html::tag::wrap(std::to_string((int)x.movecost)));
				if (const auto json = JSON_key(x.trap)) table_row[3].append(link_to(json, TRAPS_HTML, json));
				page.print(table_row);
				for (decltype(auto) tr : table_row) tr.clear();

				// swimmable terrains must be water sources
				if (x.flags & mfb(swimmable)) {
					bool ok = false;
					for (decltype(auto) x : ter_t::water_from_terrain) {
						if (x.first == ub) {
							ok = true;
							break;
						}
					}
					if (!ok) throw std::logic_error(x.name + " is swimmable, but not a water source");
				}
			}
		}
		while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR TRAPS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
		html::to_text page(out);
		page.start_print(_html);
		_title->append(html::tag::wrap("Cataclysm:Z " TRAPS_LINK_NAME));
		page.print(_head);
		_title->clear();
		page.start_print(_body);
		inline_script(page, "data/js/ZQuery.js");
		{
		auto revert = swapDOM("#" TRAPS_ID "_link", global_nav, html::tag("b", TRAPS_LINK_NAME));
		page.print(global_nav);
		*revert.first = std::move(revert.second);
		}

		static constexpr const char* table_headers[] = { "Name", "ASCII", "visibility", "avoidance", "difficulty", "disarmed parts", "triggered parts" };
		page.start_print(_data_table);
		// actual content
		{
			html::tag table_header("tr");
			table_header.set(attr_align, val_center);
			for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
			page.print(table_header);
		}

		{
			static const std::string no_disarm("cannot disarm");

			static const std::string color("color: ");
			static const std::string background("; background-color:");
			html::tag cell("td");
			html::tag table_row("tr");
			table_row.set(attr_align, val_left);
			for (decltype(auto) th : table_headers) table_row.append(cell);

			size_t ub = num_trap_types;
			const char* css_fg = nullptr;
			const char* css_bg = nullptr;
			while (0 < --ub) {
				const trap& x = *trap::traps[ub];
				table_row[0].append(wrap_in_anchor(x.name, JSON_key((trap_id)ub)));
				if (to_css_colors(x.color, css_fg, css_bg)) {
					html::tag colorize("span", std::string(1, x.sym));
					colorize.set("style", color + css_fg + background + css_bg);
					table_row[1].append(std::move(colorize));
				} else {
					table_row[1].append(html::tag::wrap(std::string(1, x.sym)));
				}
				table_row[2].append(std::to_string(x.visibility));
				table_row[3].append(std::to_string(x.avoidance));
				if (x.disarm_legal()) {
					table_row[4].append(std::to_string(x.difficulty));
				} else {
					table_row[4].append(no_disarm);
				}
				if (!x.disarm_components.empty()) {
					std::string desc;
					// align with map::disarm_trap
					for (const itype_id it : x.disarm_components) {
						const itype& info = *item::types[it];
						if (!desc.empty()) desc += "<br>\n";
						desc += "1 ";
						desc += info.name;
					}
					table_row[5].append(std::move(desc));
				}
				if (!x.trigger_components.empty()) {
					std::string desc;
					for (decltype(auto) it : x.trigger_components) {
						if (!desc.empty()) desc += "<br>\n";
						desc += it.to_s();
					}
					table_row[6].append(std::move(desc));
				}

				page.print(table_row);
				for (decltype(auto) tr : table_row) tr.clear();
			}
			}
		inline_script(page, "data/js/colorize_landing.js");
		while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

	*home_revert.first = std::move(home_revert.second);	// mapnav page family handled

	// player status pages
	// full terrain navigation menu
	html::tag statusnav_point("li");
	statusnav_point.set("id", STATUS_ID);
	{
		html::tag a_tag("a", STATUS_LINK_NAME);
		a_tag.set("href", "./" STATUS_HTML);
		a_tag.set("id", STATUS_ID "_link");
		statusnav_point.append(std::move(a_tag));
	}

	html::tag statusnav_nav("ul");
	statusnav_nav.set(attr_style, val_list_none);

#define ADDICTIONS_HTML "addictions.html"
#define ADDICTIONS_ID "addictions"
#define ADDICTIONS_LINK_NAME "Addictions"
// these two may rate their own heading
#define CRAFTING_HTML "crafting.html"
#define CRAFTING_ID "crafting"
#define CRAFTING_LINK_NAME "Crafting"
#define CONSTRUCTION_HTML "construction.html"
#define CONSTRUCTION_ID "construction"
#define CONSTRUCTION_LINK_NAME "Construction"
#define MUTATIONS_HTML "mutations.html"
#define MUTATIONS_ID "mutations"
#define MUTATIONS_LINK_NAME "Mutations"
#define SKILLS_HTML "skills.html"
#define SKILLS_ID "skills"
#define SKILLS_LINK_NAME "Skills"
#define SKILL_PLAN_HTML "skill_plan.html"
#define SKILL_PLAN_ID "skill_plan"
#define SKILL_PLAN_LINK_NAME "Skill planner"

	statusnav_nav.append(typicalMenuLink(ADDICTIONS_ID, ADDICTIONS_LINK_NAME, "./" ADDICTIONS_HTML));
	statusnav_nav.append(typicalMenuLink(CONSTRUCTION_ID, CONSTRUCTION_LINK_NAME, "./" CONSTRUCTION_HTML));
	statusnav_nav.append(typicalMenuLink(CRAFTING_ID, CRAFTING_LINK_NAME, "./" CRAFTING_HTML));
	statusnav_nav.append(typicalMenuLink(MUTATIONS_ID, MUTATIONS_LINK_NAME, "./" MUTATIONS_HTML));
	statusnav_nav.append(typicalMenuLink(SKILLS_ID, SKILLS_LINK_NAME, "./" SKILLS_HTML));
	statusnav_nav.append(typicalMenuLink(SKILL_PLAN_ID, SKILL_PLAN_LINK_NAME, "./" SKILL_PLAN_HTML));

	statusnav_point.append(statusnav_nav);

	// set up statusnav submenu
	home_revert = swapDOM("#" STATUS_ID, global_nav, std::move(statusnav_point));

#define HTML_TARGET HTML_DIR STATUS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " STATUS_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			{
				auto revert = swapDOM("#" STATUS_ID "_link", global_nav, html::tag("b", STATUS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}
			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR ADDICTIONS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " ADDICTIONS_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			inline_script(page, "data/js/ZQuery.js");
			{
				auto revert = swapDOM("#" ADDICTIONS_ID "_link", global_nav, html::tag("b", ADDICTIONS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}

			static constexpr const char* table_headers[] = { "Name" , "Description", "Items" };
			page.start_print(_data_table);
			// actual content
			{
				html::tag table_header("tr");
				table_header.set(attr_align, val_center);
				for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
				page.print(table_header);
			}

			{
				static const std::string color("color: ");
				static const std::string background("; background-color:");
				html::tag cell("td");
				html::tag table_row("tr");
				table_row.set(attr_align, val_left);
				for (decltype(auto) th : table_headers) table_row.append(cell);
				decltype(auto) tr_alias = table_row.alias();
				tr_alias[1]->append(html::tag("pre"));
				tr_alias[1] = tr_alias[1]->querySelector("pre");

				size_t ub = NUM_ADDICTIONS;
				const char* css_fg = nullptr;
				const char* css_bg = nullptr;
				while (0 < --ub) {
					std::string what;
					addiction test((add_type)ub);
					tr_alias[0]->append(wrap_in_anchor(addiction_name(test), JSON_key((add_type)ub)));
					tr_alias[1]->append(html::tag::wrap(addiction_text(test)));

					size_t xref_ub = xref_addictions.size();
					while (0 < xref_ub) {
						decltype(auto) entry = xref_addictions[--xref_ub];
						if (entry.first != ub) continue;
						if (what.empty()) what = entry.second->name;
						else what += ", " + entry.second->name;
						xref_addictions.erase(xref_addictions.begin() + xref_ub);
					}
					if (!what.empty()) tr_alias[2]->append(html::tag::wrap(std::move(what)));

					page.print(table_row);
					for (decltype(auto) tr : tr_alias) tr->clear();
				}
			}
			inline_script(page, "data/js/colorize_landing.js");
			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR SKILLS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " SKILLS_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			inline_script(page, "data/js/ZQuery.js");
			{
				auto revert = swapDOM("#" SKILLS_ID "_link", global_nav, html::tag("b", SKILLS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}

			static constexpr const char* table_headers[] = { "Name" , "Description" };
			page.start_print(_data_table);
			// actual content
			{
				html::tag table_header("tr");
				table_header.set(attr_align, val_center);
				for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
				page.print(table_header);
			}

			{
				html::tag cell("td");
				html::tag table_row("tr");
				table_row.set(attr_align, val_left);
				for (decltype(auto) th : table_headers) table_row.append(cell);
				decltype(auto) tr_alias = table_row.alias();
				tr_alias[1]->append(html::tag("pre"));
				tr_alias[1] = tr_alias[1]->querySelector("pre");

				size_t ub = num_skill_types;
				while (0 < --ub) {
					skill test((skill)ub);
					const auto sk_name = skill_name(test);
					tr_alias[0]->append(wrap_in_anchor(sk_name, html::encode_id(sk_name).c_str()));
					tr_alias[1]->append(html::tag::wrap(skill_description(test)));

					page.print(table_row);
					for (decltype(auto) tr : tr_alias) tr->clear();
				}
			}
			inline_script(page, "data/js/colorize_landing.js");
			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR CRAFTING_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " CRAFTING_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			{
				auto revert = swapDOM("#" CRAFTING_ID "_link", global_nav, html::tag("b", CRAFTING_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}

			static constexpr const char* table_headers[] = { "Result" , "Category", "Primary Skill", "Secondary Skill", "Difficulty", "Time", "Tools", "Components" };
			page.start_print(_data_table);
			// actual content
			{
				html::tag table_header("tr");
				table_header.set(attr_align, val_center);
				for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
				page.print(table_header);
			}

			{
				html::tag cell("td");
				html::tag table_row("tr");
				table_row.set(attr_align, val_left);
				table_row.set(attr_valign, val_top);
				for (decltype(auto) th : table_headers) table_row.append(cell);

				int ub = recipe::recipes.size();
				while (0 <= --ub) {
					const recipe* const test = recipe::recipes[ub];
					if (auto json = JSON_key(test->result)) {
						table_row[0].append(html::tag::wrap(json));
						table_row[1].append(html::tag::wrap(crafting_category(test->category)));
						if (auto primary = JSON_key((test->sk_primary))) table_row[2].append(html::tag::wrap(primary));
						if (auto secondary = JSON_key((test->sk_secondary))) table_row[3].append(html::tag::wrap(secondary));
						table_row[4].append(html::tag::wrap(std::to_string(test->difficulty)));
						table_row[5].append(html::tag::wrap("<nobr>" + test->time_desc() + "</nobr>"));
						if (!test->tools.empty()) table_row[6].append(html::tag::wrap(to_s(test->tools)));
						if (!test->components.empty()) table_row[7].append(html::tag::wrap(to_s(test->components)));
					} else throw std::logic_error("unidentified crafting result");

					page.print(table_row);
					for (decltype(auto) tr : table_row) tr.clear();
				}
			}
			page.end_print();

			page.print(html::tag::wrap("Water may be taken from some terrain types.  Purifying before drinking recommended."));

			static constexpr const char* water_table_headers[] = { "Terrain", "Chance of food poisoning when drunk", "Severity" };
			page.start_print(_data_table);
			// actual content
			{
				html::tag table_header("tr");
				table_header.set(attr_align, val_center);
				for (decltype(auto) th : water_table_headers) table_header.append(html::tag("th", th));
				page.print(table_header);
			}

			{
				html::tag cell("td");
				html::tag table_row("tr");
				table_row.set(attr_align, val_left);
				table_row.set(attr_valign, val_top);
				for (decltype(auto) th : water_table_headers) table_row.append(cell);

				for (decltype(auto) x : ter_t::water_from_terrain) {
					if (auto json = JSON_key(x.first)) {
						table_row[0].append(html::tag::wrap(json));
						table_row[1].append(html::tag::wrap(std::to_string(x.second.first.numerator()) + " in " + std::to_string(x.second.first.denominator())));
						table_row[2].append(html::tag::wrap(std::to_string(x.second.second.first) + " ... " + std::to_string(x.second.second.second)));
					}
					else throw std::logic_error("unhandled terrain as water source");

					page.print(table_row);
					for (decltype(auto) tr : table_row) tr.clear();
				}
			}

			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR CONSTRUCTION_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " CONSTRUCTION_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			{
				auto revert = swapDOM("#" CONSTRUCTION_ID "_link", global_nav, html::tag("b", CONSTRUCTION_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}

			page.print(html::tag::wrap("The (primary) skill for all constructions, is carpentry.")); // \todo hyperlink to carpentry skill

			static constexpr const char* table_headers[] = { "Result" , "Difficulty", "Stages" };
			page.start_print(_data_table);
			// actual content
			{
				html::tag table_header("tr");
				table_header.set(attr_align, val_center);
				for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
				page.print(table_header);
			}

			{
				html::tag cell("td");
				html::tag table_row("tr");
				table_row.set(attr_align, val_left);
				table_row.set(attr_valign, val_top);
				for (decltype(auto) th : table_headers) table_row.append(cell);

				int ub = constructable::constructions.size();
				while (0 <= --ub) {
					const constructable* const test = constructable::constructions[ub];
					table_row[0].append(test->name);
					table_row[1].append(html::tag::wrap(std::to_string(test->difficulty)));
					table_row[2].append(to_table(test->stages));

					page.print(table_row);
					for (decltype(auto) tr : table_row) tr.clear();
				}
			}
			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR SKILL_PLAN_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " SKILL_PLAN_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			{
				auto revert = swapDOM("#" SKILL_PLAN_ID "_link", global_nav, html::tag("b", SKILL_PLAN_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}

			page.print(html::tag("p","All constructions train carpentry, including the difficulty zero ones.")); // \todo hyperlink to carpentry skill

			// actual content -- unusual in that we analyze multiple sources here
			if (!books.empty()) {
				decltype(books) educational;
				for (const auto book : books) if (book->type) educational.push_back(book);
				std::sort(educational.begin(), educational.end(), [](auto x, auto y) {
					if (auto test = strcmp(JSON_key(x->type), JSON_key(y->type))) return 0 < test;
					return x->req > y->req;
				});

				page.start_print(_data_table);
				static constexpr const char* table_headers[] = { "Name" , "Skill", "INT", "start at", "educates to"};
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					{
					html::tag header_cell("th", "Learning from books");
					header_cell.set("colspan", std::to_string(std::end(table_headers)-std::begin(table_headers)));
					table_header.append(std::move(header_cell));
					}
					page.print(table_header);
					table_header.clear();
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag cell("td");
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.set(attr_valign, val_top);
					for (decltype(auto) th : table_headers) table_row.append(cell);


					int ub = educational.size();
					while (0 <= --ub) {
						const it_book* const book = educational[ub];
						if (!book->type) continue;

						table_row[0].append(book->name);
						if (auto sk = JSON_key((book->type))) table_row[1].append(html::tag::wrap(sk));
						if (book->intel) table_row[2].append(std::to_string((int)(book->intel)));
						if (book->req) table_row[3].append(std::to_string((int)(book->req)));
						if (book->level) table_row[4].append(std::to_string((int)(book->level)));

						page.print(table_row);
						for (decltype(auto) tr : table_row) tr.clear();
					}
				}
				// page.end_print();
			}

			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

#define HTML_TARGET HTML_DIR MUTATIONS_HTML

	if (FILE* out = fopen(HTML_TARGET ".tmp", "w")) {
		{
			html::to_text page(out);
			page.start_print(_html);
			_title->append(html::tag::wrap("Cataclysm:Z " MUTATIONS_LINK_NAME));
			page.print(_head);
			_title->clear();
			page.start_print(_body);
			{
				auto revert = swapDOM("#" MUTATIONS_ID "_link", global_nav, html::tag("b", MUTATIONS_LINK_NAME));
				page.print(global_nav);
				*revert.first = std::move(revert.second);
			}

			// base display
			{
				page.start_print(_data_table);
				static constexpr const char* table_headers[] = { "Name" , "Points", "Visibility", "Ugliness", "Description" };
				{
					html::tag table_header("tr");
					for (decltype(auto) th : table_headers) table_header.append(html::tag("th", th));
					page.print(table_header);
				}
				{
					html::tag cell("td");
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.set(attr_valign, val_top);
					for (decltype(auto) th : table_headers) table_row.append(cell);

					for (decltype(auto) mut : mutation_branch::traits) {
						table_row[0].append(mut.name);
						table_row[1].append(std::to_string(mut.points));
						table_row[2].append(std::to_string(mut.visiblity));
						table_row[3].append(std::to_string(mut.ugliness));
						table_row[4].append(mut.description);

						page.print(table_row);
						for (decltype(auto) tr : table_row) tr.clear();
					}
				}
//				page.end_print();
			}
			// \todo dependency graph
			while (page.end_print());
		}

		unlink(HTML_TARGET);
		rename(HTML_TARGET ".tmp", HTML_TARGET);
	}

#undef HTML_TARGET

	*home_revert.first = std::move(home_revert.second);	// statusnav page family handled

	// \todo monster types page (blocked by items)
	// for now, just audit
	for (auto x : mtype::types) {
		if (x->anger & x->placate) throw new std::logic_error("unreasonable to be both angered and placated by same trigger");
		if ((x->flags & mfb(MF_GOODHEARING)) && !(x->flags & mfb(MF_HEARS))) throw std::logic_error("the deaf cannot have good hearing");
		if ((x->flags & mfb(MF_VENOM)) && (x->flags & mfb(MF_BADVENOM))) throw std::logic_error("no multiple-strength venoms");
	}

	// try to replicate some issues
	std::vector<item> res;
	std::vector<item> stack0;
	stack0.push_back(item(item::types[itm_mag_guns], 0));
	stack0.push_back(item(item::types[itm_sandwich_t], 0));
	if (!JSON::encode(stack0).decode(res)) throw std::logic_error("critical round-trip failure");
	if (res != stack0) throw std::logic_error("round-trip failure");

	// route the command line options here

	erase(); // Clear screen
	endwin(); // End ncurses
	system("clear"); // Tell the terminal to clear itself
	return 0;
}
