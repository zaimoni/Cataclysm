#ifndef MOBILE_H
#define MOBILE_H

#include "GPS_loc.hpp"
#include "grammar.h"
#include <optional>
#include <type_traits>

namespace cataclysm {

	class JSON;

}

// intended base class for things that move
class mobile : public grammar::noun
{
public:
	static constexpr const int mp_turn = 100; // move points/standard turn

	// ultimately would be handled centrally but for now, forward; not necessarily sensible for vehicles
	enum class effect {
		DOWNED = 1,
		STUNNED,
		DEAF,
		BLIND,
		POISONED,
		ONFIRE
	};

	GPS_loc GPSpos; // absolute location
	int moves;

	// Waterfall lifecycle here: unused member functions ok
	// throwing interfaces only appropriate for vehicles and player; monsters and NPCs may be slightly out of bounds
	std::optional<point> screen_pos() const;
	point screenPos() const;
	std::optional<reality_bubble_loc> bubble_pos() const;
	reality_bubble_loc bubblePos() const;
	void set_screenpos(const GPS_loc& loc);

	// unified effects/diseases
	virtual void add(effect src, int duration) = 0;
	virtual bool has(effect src) const = 0;

	// unified knockback (flying back one tile)
	void knockback_from(const GPS_loc& loc);
	virtual int knockback_size() const = 0;
	virtual bool hurt(int dam) = 0; // Deals this dam damage; returns true if we died
	virtual bool hitall(int dam, int vary = 0) = 0;
	virtual int dodge_roll() const = 0; // For comparison to hit_roll()
	virtual int melee_skill() const = 0;
	virtual int min_technique_power() const = 0;
	virtual int current_speed() const = 0; // Number of movement points we get a turn

	// std::visit assistants
	struct cast
	{
		cast() = default;
		cast(const cast& src) = delete;
		cast(cast&& src) = delete;
		cast& operator=(const cast& src) = delete;
		cast& operator=(cast&& src) = delete;
		~cast() = default;

		auto operator()(mobile* target) { return target; }
		auto operator()(const mobile* target) { return target; }
	};

protected:
	mobile() noexcept : GPSpos(_ref<GPS_loc>::invalid),moves(0) {}
	mobile(const GPS_loc& origin, int m) noexcept : GPSpos(origin), moves(m) {}
	mobile(const mobile& src) = default;
	mobile(mobile&& src) = default;

	mobile(const cataclysm::JSON& src);

	virtual ~mobile() = default;
	mobile& operator=(const mobile& src) = default;
	mobile& operator=(mobile&& src) = default;

	void set_screenpos(point pt); // could be public once synchronization with legacy point pos not needed
	virtual void _set_screenpos() = 0;

	/// <returns>true iff continuing</returns>
	bool flung(int& flvel, GPS_loc& loc);

private:
	virtual bool handle_knockback_into_impassable(const GPS_loc& dest) = 0;
};

template<class T>
class countdown
{
public:
	T x;
	int remaining;	// will want more access control after save/load conversion

	countdown() = default;
	countdown(const T& src, int t) noexcept(std::is_nothrow_copy_constructible_v<T>) : x(src), remaining(t) {}
	countdown(const countdown& src) = default;
	countdown(countdown&& src) = default;
	~countdown() = default;
	countdown& operator=(const countdown& src) = default;
	countdown& operator=(countdown&& src) = default;

	void set(const T& src, int t) {
		x = src;
		remaining = t;
	}
	void set(T&& src, int t) {
		x = std::move(src);
		remaining = t;
	}

	void tick() { if (0 < remaining) --remaining; }
	bool live() const { return 0 < remaining; }
};

#endif
