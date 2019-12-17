#ifndef STL_LIMITS_H
#define STL_LIMITS_H

#include <limits>

namespace cataclysm {

	template<int n, int d, class T>
	void rational_scale(T& x)
	{
		if (std::numeric_limits<T>::max()/n >= x) {
			x *= n;
			x /= d;
		} else {
			x /= d;
			x *= n;
		}
	}

}

#endif
