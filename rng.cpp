#include "rng.h"
#include <stdlib.h>

long rng(long low, long high)
{
 return low + long((high - low + 1) * double(rand() / double(RAND_MAX + 1.0)));
}

int dice(int number, int sides)
{
 int ret = 0;
 for (int i = 0; i < number; i++)
  ret += rng(1, sides);
 return ret;
}
