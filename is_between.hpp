#ifndef IS_BETWEEN_HPP
#define IS_BETWEEN_HPP 1

constexpr auto is_between(auto lb, auto src, auto ub)
{
	return lb <= src && src <= ub;
}

template<auto lb, auto ub> constexpr bool is_between(auto src) {
	static_assert(lb < ub);
	return lb <= src && ub >= src;
}

#endif
