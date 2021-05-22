#ifndef TRAP_HANDLER_HPP
#define TRAP_HANDLER_HPP 1

class game;
class monster;
struct point;

namespace trapfunc {
	void bubble(game* g, int x, int y);
	void beartrap(game* g, int x, int y);
	void board(game* g, int x, int y);
	void tripwire(game* g, int x, int y);
	void crossbow(game* g, int x, int y);
	void shotgun(game* g, int x, int y);
	void blade(game* g, int x, int y);
	void landmine(game* g, int x, int y);
	void telepad(game* g, int x, int y);
	void goo(game* g, int x, int y);
	void dissector(game* g, int x, int y);
	void sinkhole(game* g, int x, int y);
	void pit(game* g, int x, int y);
	void pit_spikes(game* g, int x, int y);
	void lava(game* g, int x, int y);
	void ledge(game* g, int x, int y);
	void boobytrap(game* g, int x, int y);
	void temple_flood(game* g, int x, int y);
	void temple_toggle(game* g, int x, int y);
	void glow(game* g, int x, int y);
	void shadow(game* g, int x, int y);
	void drain(game* g, int x, int y);
	void snake(game* g, int x, int y);

	void hum(const point& pt);
};

namespace trapfuncm {
	void bubble(game* g, monster* z);
	void beartrap(game* g, monster* z);
	void board(game* g, monster* z);
	void tripwire(game* g, monster* z);
	void crossbow(game* g, monster* z);
	void shotgun(game* g, monster* z);
	void blade(game* g, monster* z);
	void landmine(game* g, monster* z);
	void telepad(game* g, monster* z);
	void goo(game* g, monster* z);
	void dissector(game* g, monster* z);
	void pit(game* g, monster* z);
	void pit_spikes(game* g, monster* z);
	void lava(game* g, monster* z);
	void ledge(game* g, monster* z);
	void boobytrap(game* g, monster* z);
	void glow(game* g, monster* z);
	void drain(game* g, monster* z);
	void snake(game* g, monster* z);
};

#endif
