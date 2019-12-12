#ifndef OM_CACHE_HPP
#define OM_CACHE_HPP 1

#include <map>
#include <utility>
#include "enums.h"

class overmap;

// singleton
class om_cache
{
private:
	std::map<tripoint, std::pair<bool,overmap*> > _cache;	// could use std::unique_ptr in place of raw pointer here and default destructor instead
	
	om_cache() = default;
	~om_cache();
	om_cache(const om_cache& src) = delete;
	om_cache(om_cache&& src) = delete;
	om_cache& operator=(const om_cache& src) = delete;
	om_cache& operator=(om_cache&& src) = delete;
public:
	static om_cache& get();
	overmap* get(const tripoint& x);
	overmap* create(const tripoint& x);	// only if needed
	const overmap* r_get(const tripoint& x);
	const overmap* r_create(const tripoint& x);	// only if needed
	void expire();	// all overmaps not used flushed to hard drive; usage flag reset
	void save();
};

#endif

