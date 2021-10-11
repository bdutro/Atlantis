#ifndef TYPE_UTILS
#define TYPE_UTILS

#include <type_traits>

namespace type_utils {
    template <class T, class... Ts>
    struct is_any : std::disjunction<std::is_same<T, Ts>...> {};

    template <class T, class... Ts>
    struct are_same : std::conjunction<std::is_same<T, Ts>...> {};
} // end namespace type_utils

#endif
