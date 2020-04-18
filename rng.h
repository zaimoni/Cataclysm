#ifndef _RNG_H_
#define _RNG_H_
long rng(long low, long high);
int dice(int number, int sides);

inline bool one_in(int chance) { return (chance <= 1 || rng(0, chance - 1) == 0); };
inline bool rng_lte(long low, long high, long ub) { return (high <= ub) ? true : (ub < low ? false : rng(low, high) <= ub); }

#endif
