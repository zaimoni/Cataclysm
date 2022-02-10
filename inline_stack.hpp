#ifndef INLINE_STACK_HPP
#define INLINE_STACK_HPP 1

#include <stddef.h>

template<class T, size_t N>
class inline_stack final
{
    size_t ub;
    T x[N];

public:
    inline_stack() : ub(0) {}
    // implicit defaults for now

    bool empty() const { return 0 >= ub; }
    auto size() const { return ub; }
    void push(const T& src) { x[ub++] = src; }
    void push(T&& src) { x[ub++] = src; }
    T& operator[](ptrdiff_t n) { return x[n]; }
    const T& operator[](ptrdiff_t n) const { return x[n]; }
};


#endif
