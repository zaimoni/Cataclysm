#ifndef FUNCTIONAL_HPP

#include <functional>

namespace zaimoni {

    // modeled after C# Action, Func delegates
    template<class R, class Arg1, class Arg2>
    class handler {
        std::function<R(Arg1)> handle_1;
        std::function<R(Arg2)> handle_2;
    public:
        handler(decltype(handle_1) handle_1, decltype(handle_2) handle_2) : handle_1(handle_1), handle_2(handle_2) {}

        R operator()(Arg1 x) requires(std::is_same_v<void, R>) { handle_1(x); }
        R operator()(Arg2 x) requires(std::is_same_v<void, R>) { handle_2(x); }
        R operator()(Arg1 x) requires(!std::is_same_v<void, R>) { return handle_1(x); }
        R operator()(Arg2 x) requires(!std::is_same_v<void, R>) { return handle_2(x); }
    };

}

#endif
