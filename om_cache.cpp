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
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = 2;
		return ret.second;
	}
	return 0;
}

const overmap* om_cache::r_get(const tripoint& x)
{
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = 1;	// would need const cast to update
		return ret.second;
	}
	return 0;
}

overmap& om_cache::create(const tripoint& x)	// only if needed
{
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
