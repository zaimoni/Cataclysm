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

overmap* om_cache::get(const tripoint& x)
{
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = true;
		return ret.second;
	}
	return 0;
}

overmap* om_cache::create(const tripoint& x)	// only if needed
{
	if (_cache.count(x)) {
		auto& ret = _cache[x];
		ret.first = true;
		return ret.second;
	}
	std::unique_ptr<overmap> ret(new overmap(game::active(), x.x, x.y, x.z));
	_cache[x] = std::pair(true, ret.get());
	return ret.release();
}

void om_cache::expire()	// all overmaps not used flushed to hard drive; usage flag reset
{
	std::vector<tripoint> discard;
	for (const auto& x : _cache) if (!x.second.first) discard.push_back(x.first);
	if (!discard.empty()) for (const auto& kill : discard) {
		_cache[kill].second->save(game::active()->u.name);
		_cache.erase(kill);
	}
}

void om_cache::save()
{
	std::vector<tripoint> discard;
	for (auto& x : _cache) {
		x.second.second->save(game::active()->u.name);
		if (x.second.first) x.second.first = false;
		else discard.push_back(x.first);
	}
	if (!discard.empty()) for (const auto& kill : discard) _cache.erase(kill);
}
