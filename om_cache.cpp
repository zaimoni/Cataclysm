#include "om_cache.hpp"
#include "game.h"

om_cache::~om_cache()
{
	for (auto& x : _cache) {
		delete x.second.second;
		x.second.second = 0;
	}
}

om_cache& om_cache::get()
{
	static om_cache ooao;
	return ooao;
}

// status coding: 0 unread, 1 read, 2 written to
overmap* om_cache::get(const tripoint& x)
{
	if (x == game::active()->cur_om.pos) return &(game::active()->cur_om);
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = 2;
		return ret.second;
	}
	return 0;
}

const overmap* om_cache::r_get(const tripoint& x)
{
	if (x == game::active()->cur_om.pos) return &(game::active()->cur_om);
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = 1;	// would need const cast to update
		return ret.second;
	}
	return 0;
}

// \todo get/r_get variants that target overmap properties, rather than the tripoint coordinate
// required functions
// bool preview(ifstream&) : for each overmap file being tested, report back whether the overmap file should be loaded for a detailed check.
// bool test(overmap&) : reports whether the overmap actually implements the desired property
// both of these may need to be function objects, i.e. std::function may be required

// Would also need way to retrieve all overmap filenames, and exclude those already loaded from consideration

overmap& om_cache::create(const tripoint& x)	// only if needed
{
	if (x == game::active()->cur_om.pos) return game::active()->cur_om;
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = 2;
		return *ret.second;
	}
	std::unique_ptr<overmap> ret(new overmap(game::active(), x.x, x.y, x.z));
	_cache[x] = std::pair(2, ret.get());
	return *(ret.release());
}

const overmap& om_cache::r_create(const tripoint& x)	// only if needed
{
	if (x == game::active()->cur_om.pos) return game::active()->cur_om;
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = 1;	// would need const cast to update
		return *ret.second;
	}
	std::unique_ptr<overmap> ret(new overmap(game::active(), x.x, x.y, x.z));
	_cache[x] = std::pair(1, ret.get());	// creation saved to hard drive already
	return *(ret.release());
}


void om_cache::expire()
{
	std::vector<tripoint> discard;
	for (auto& x : _cache) {
		if (0 >= x.second.first) discard.push_back(x.first);	// not even accessed
		else if (1 == x.second.first) {
			// read but not written to: demote
			x.second.first = 0;
		}
		// was written to...do not have save command so leave alone
	}
	if (!discard.empty()) for (const auto& kill : discard) _cache.erase(kill);
}

void om_cache::save()
{
	std::vector<tripoint> discard;
	for (auto& x : _cache) {
		if (0 >= x.second.first) discard.push_back(x.first);	// not even accessed
		else if (2 <= x.second.first) {	// was written to
			x.second.second->save(game::active()->u.name);
			x.second.first = 1;	// treat as read-from now
		}
	}
	if (!discard.empty()) for (const auto& kill : discard) _cache.erase(kill);
}

void om_cache::load(overmap& dest, const tripoint& x)	// dest is typically game::cur_om
{
	if (x == dest.pos) return;
	if (_cache.count(dest.pos)) {	// should not happen
		auto& working = _cache[dest.pos];
		*working.second = std::move(dest);
		working.first = 1;
	}
	if (_cache.count(x)) {
		auto& working = _cache[x];
		dest = std::move(*working.second);
		delete working.second;
		_cache.erase(x);
		return;
	}
	dest = overmap(game::active(), x.x, x.y, x.z);
}
