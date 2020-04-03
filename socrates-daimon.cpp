/* Main Loop for Socrates' Daimon, Cataclysm analysis utility */

#include "color.h"
#include "mtype.h"
#include "item.h"
#include "output.h"
#include "ios_file.h"
#include "json.h"
#include "html.hpp"
#include <stdexcept>
#include <stdlib.h>
#include <time.h>

#include <fstream>

using namespace cataclysm;

// from saveload.cpp
bool fromJSON(const JSON& _in, item& dest);
JSON toJSON(const item& src);

const char* JSON_key(material src);

static void check_roundtrip_JSON(const item& src)
{
	item staging;
	if (!fromJSON(toJSON(src), staging)) throw std::logic_error("critical round-trip failure");
	if (src != staging) throw std::logic_error("round-trip was not invariant");
}

template<class T>
static void to_desc(const std::vector<T*>& src, std::map<std::string, std::string>& dest)
{
	static_assert(std::is_base_of_v<itype, T>);
	for (auto it : src) {
		item test(it, 0);
		dest[test.tname()] = test.info(true);
	}
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
	const html::tag _body("body");	// stage-printed
	// navigation sidebar will be "common", at least between page types
	html::tag global_nav("ul");
	global_nav.set(attr_style, std::move(val_thin_border + CSS_sep + val_left_align + CSS_sep + val_list_none + CSS_sep + "padding:10px"));

	html::tag working_li("li");

	html::tag item_nav("ul");
	item_nav.set(attr_style, val_list_none);
	item_nav.set("id", "items");

#define AMMO_HTML "ammo.html"
#define AMMO_ID "ammo"
#define AMMO_LINK_NAME "Ammo"
#define BOOKS_HTML "books.html"
#define BOOKS_ID "books"
#define BOOKS_LINK_NAME "Books"
#define CONTAINERS_HTML "containers.html"
#define CONTAINERS_ID "containers"
#define CONTAINERS_LINK_NAME "Containers"
#define FUEL_HTML "fuel.html"
#define FUEL_ID "fuel"
#define FUEL_LINK_NAME "Fuel"
#define MARTIAL_ARTS_HTML "ma_styles.html"
#define MARTIAL_ARTS_ID "ma_styles"
#define MARTIAL_ARTS_LINK_NAME "Martial Arts"

	working_li.set("id", AMMO_ID);
	{
	html::tag a_tag("a", AMMO_LINK_NAME);
	a_tag.set("href", "./" AMMO_HTML);
	working_li.append(std::move(a_tag));
	}
	item_nav.append(working_li);
	working_li.clear();

	working_li.set("id", BOOKS_ID);
	{
	html::tag a_tag("a", BOOKS_LINK_NAME);
	a_tag.set("href", "./" BOOKS_HTML);
	working_li.append(std::move(a_tag));
	}
	item_nav.append(working_li);
	working_li.clear();

	working_li.set("id", CONTAINERS_ID);
	{
	html::tag a_tag("a", CONTAINERS_LINK_NAME);
	a_tag.set("href", "./" CONTAINERS_HTML);
	working_li.append(std::move(a_tag));
	}
	item_nav.append(working_li);
	working_li.clear();

	working_li.set("id", FUEL_ID);
	{
	html::tag a_tag("a", FUEL_LINK_NAME);
	a_tag.set("href", "./" FUEL_HTML);
	working_li.append(std::move(a_tag));
	}
	item_nav.append(working_li);
	working_li.clear();

	working_li.set("id", MARTIAL_ARTS_ID);
	{
	html::tag a_tag("a", MARTIAL_ARTS_LINK_NAME);
	a_tag.set("href", "./" MARTIAL_ARTS_HTML);
	working_li.append(std::move(a_tag));
	}
	item_nav.append(working_li);

	working_li.clear();
	working_li.unset("id");
	working_li.append(html::tag::wrap("Items"));
	working_li.append(item_nav);
	global_nav.append(working_li);

	const html::tag _data_table("table");	// stage-printed

	std::vector<it_ammo*> ammunition;
	std::vector<it_book*> books;
	std::vector<it_container*> containers;
	std::vector<it_ammo*> fuel;	// XXX conflation in type system
	std::vector<it_style*> ma_styles;	// martial arts styles

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

			ma_styles.push_back(style);
			will_handle_as_html = true;
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

		if (it->is_container()) {
			const auto cont = static_cast<it_container*>(it);
			if (!cont) throw std::logic_error(it->name + ": static cast to container failed");

			containers.push_back(cont);
			will_handle_as_html = true;
		} else if (it->is_book()) {
			const auto book = static_cast<it_book*>(it);
			if (!book) throw std::logic_error(it->name + ": static cast to book failed");

			books.push_back(book);
			will_handle_as_html = true;
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
			if (!will_handle_as_html) fout << test.tname() << std::endl << test.info(true) << std::endl << "====" << std::endl;
			name_id[test.tname()] = it->id;
			if (it->is_food() || it->is_software()) {	// i.e., can create in own container
				auto test2 = test.in_its_container();
				if (test2.type != test.type) {
					if (!will_handle_as_html) fout << test2.tname() << std::endl << test2.info(true) << std::endl << "====" << std::endl;
				}
				check_roundtrip_JSON(test2);
			}
			if (num_items != it->id && itm_null != it->id) check_roundtrip_JSON(test);
		}
	}

	OFSTREAM_ACID_CLOSE(fout, "data\\items_raw.txt")

	if (!ma_styles.empty()) {
		to_desc(ma_styles, name_desc);
#define HTML_TARGET "data\\" MARTIAL_ARTS_HTML

		FILE* out = fopen(HTML_TARGET ".tmp", "w");
		if (out) {
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
				auto subheader = global_nav.querySelector("#" MARTIAL_ARTS_ID);
#ifndef NDEBUG
				if (!subheader) throw new std::logic_error("missing update target");
#endif
				auto backup(std::move(*subheader));
				*subheader = html::tag("b", MARTIAL_ARTS_LINK_NAME);
				page.print(global_nav);
				*subheader = std::move(backup);
				}
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					table_header.append(html::tag("th", "Name"));
					table_header.append(html::tag("th", "Description"));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.append(cell);
					table_row.append(cell);
					table_row[1]->append(html::tag("pre"));
					auto _pre = table_row.querySelector("pre");

					for (const auto& x : name_desc) {
						table_row[0]->append(html::tag::wrap(x.first));
						_pre->append(html::tag::wrap(x.second));
						page.print(table_row);
						table_row[0]->clear();
						_pre->clear();
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
		to_desc(containers, name_desc);
#define HTML_TARGET "data\\" CONTAINERS_HTML

		FILE* out = fopen(HTML_TARGET ".tmp", "w");
		if (out) {
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
				auto subheader = global_nav.querySelector("#" CONTAINERS_ID);
#ifndef NDEBUG
				if (!subheader) throw new std::logic_error("missing update target");
#endif
				auto backup(std::move(*subheader));
				*subheader = html::tag("b", CONTAINERS_LINK_NAME);
				page.print(global_nav);
				*subheader = std::move(backup);
				}
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					table_header.append(html::tag("th", "Name"));
					table_header.append(html::tag("th", "Description"));
					table_header.append(html::tag("th", "Material"));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.append(cell);
					table_row.append(cell);
					table_row.append(cell);
					table_row[1]->append(html::tag("pre"));
					auto _pre = table_row.querySelector("pre");

					for (const auto& x : name_desc) {
						table_row[0]->append(html::tag::wrap(x.first));
						_pre->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) table_row[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						table_row[0]->clear();
						_pre->clear();
						table_row[2]->clear();
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

	// \todo skills page should provide book learning chain
	if (!books.empty()) {
		to_desc(books, name_desc);
#define HTML_TARGET "data\\" BOOKS_HTML

		FILE* out = fopen(HTML_TARGET ".tmp", "w");
		if (out) {
			html::tag cell("td");
			cell.set(attr_valign, val_top);

			{
				html::to_text page(out);
				page.start_print(_html);
				_title->append(html::tag::wrap("Cataclysm:Z " BOOKS_LINK_NAME));
				page.print(_head);
				_title->clear();
				page.start_print(_body);
				{
					auto subheader = global_nav.querySelector("#" BOOKS_ID);
#ifndef NDEBUG
					if (!subheader) throw new std::logic_error("missing update target");
#endif
					auto backup(std::move(*subheader));
					*subheader = html::tag("b", BOOKS_LINK_NAME);
					page.print(global_nav);
					*subheader = std::move(backup);
				}
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					table_header.append(html::tag("th", "Name"));
					table_header.append(html::tag("th", "Description"));
					table_header.append(html::tag("th", "Material"));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.append(cell);
					table_row.append(cell);
					table_row.append(cell);
					table_row[1]->append(html::tag("pre"));
					auto _pre = table_row.querySelector("pre");

					for (const auto& x : name_desc) {
						table_row[0]->append(html::tag::wrap(x.first));
						_pre->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) table_row[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						table_row[0]->clear();
						_pre->clear();
						table_row[2]->clear();
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

	if (!ammunition.empty()) {
		to_desc(ammunition, name_desc);
#define HTML_TARGET "data\\" AMMO_HTML

		FILE* out = fopen(HTML_TARGET ".tmp", "w");
		if (out) {
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
				auto subheader = global_nav.querySelector("#" AMMO_ID);
#ifndef NDEBUG
				if (!subheader) throw new std::logic_error("missing update target");
#endif
				auto backup(std::move(*subheader));
				*subheader = html::tag("b", AMMO_LINK_NAME);
				page.print(global_nav);
				*subheader = std::move(backup);
				}
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					table_header.append(html::tag("th", "Name"));
					table_header.append(html::tag("th", "Description"));
					table_header.append(html::tag("th", "Material"));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.append(cell);
					table_row.append(cell);
					table_row.append(cell);
					table_row[1]->append(html::tag("pre"));
					auto _pre = table_row.querySelector("pre");

					for (const auto& x : name_desc) {
						table_row[0]->append(html::tag::wrap(x.first));
						_pre->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) table_row[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						table_row[0]->clear();
						_pre->clear();
						table_row[2]->clear();
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
		to_desc(fuel, name_desc);
#define HTML_TARGET "data\\" FUEL_HTML

		FILE* out = fopen(HTML_TARGET ".tmp", "w");
		if (out) {
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
				auto subheader = global_nav.querySelector("#" FUEL_ID);
#ifndef NDEBUG
				if (!subheader) throw new std::logic_error("missing update target");
#endif
				auto backup(std::move(*subheader));
				*subheader = html::tag("b", FUEL_LINK_NAME);
				page.print(global_nav);
				*subheader = std::move(backup);
				}
				page.start_print(_data_table);
				// actual content
				{
					html::tag table_header("tr");
					table_header.set(attr_align, val_center);
					table_header.append(html::tag("th", "Name"));
					table_header.append(html::tag("th", "Description"));
					table_header.append(html::tag("th", "Material"));
					page.print(table_header);
				}
				{
					html::tag table_row("tr");
					table_row.set(attr_align, val_left);
					table_row.append(cell);
					table_row.append(cell);
					table_row.append(cell);
					table_row[1]->append(html::tag("pre"));
					auto _pre = table_row.querySelector("pre");

					for (const auto& x : name_desc) {
						table_row[0]->append(html::tag::wrap(x.first));
						_pre->append(html::tag::wrap(x.second));
						if (auto mat = JSON_key((material)item::types[name_id[x.first]]->m1)) table_row[2]->append(html::tag::wrap(mat));
						page.print(table_row);
						table_row[0]->clear();
						_pre->clear();
						table_row[2]->clear();
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

