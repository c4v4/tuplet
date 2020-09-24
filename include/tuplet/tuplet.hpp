#include <type_traits>
#include <cstddef>
#include <utility>

namespace std {
template<class T>
class reference_wrapper;
}

namespace tuplet {
template<class... T>
struct tuple;

template<size_t I>
using index = std::integral_constant<size_t, I>;

namespace detail {
template<size_t I, class T>
struct tuple_elem {
    static T decl_elem(index<I>);

    [[no_unique_address]] T elem;

    decltype(auto) operator[](index<I>) & {
        return (elem);
    }
    decltype(auto) operator[](index<I>) const& {
        return (elem);
    }
    decltype(auto) operator[](index<I>) && {
        return (std::move(*this).elem);
    }
};

template<size_t I, class... T>
struct partial_tuple;

template<size_t I, class T>
struct partial_tuple<I, T> : tuple_elem<I, T> {
    using tuple_elem<I, T>::decl_elem;
    using tuple_elem<I, T>::operator[];
};

template<size_t I, class T, class... Rest>
struct partial_tuple<I, T, Rest...> : tuple_elem<I, T>, partial_tuple<I + 1, Rest...> {
    using tuple_elem<I, T>::decl_elem;
    using tuple_elem<I, T>::operator[];
    using partial_tuple<I + 1, Rest...>::decl_elem;
    using partial_tuple<I + 1, Rest...>::operator[];
};

template<class T>
struct unwrap_type {
    using type = T;
};
template<class T>
struct unwrap_type<std::reference_wrapper<T>> {
    using type = T&;
};
template<class T>
using unwrap_t = typename unwrap_type<T>::type;

template<size_t... I, class Dest, class... T>
void assign(Dest& dest, std::index_sequence<I...>, T&&... elems) {
    ((void)(dest[index<I>()] = std::forward<T>(elems)) , ...);
}
template<class Product, class Source, size_t... I>
Product convert(Source&& source, std::index_sequence<I...>) {
    return Product{std::forward<Source>(source)[index<I>()]... };
}
template<class Func, class Tuple, size_t... I>
decltype(auto) apply(Tuple&& t, Func&& func, std::index_sequence<I...>) {
    return std::forward<Func>(func)(t[index<I>()] ...);
}
}

template<class... T>
struct tuple : detail::partial_tuple<0, T...> {
   private:
    constexpr static size_t N = sizeof...(T);
    constexpr static auto indicies = std::make_index_sequence<N>();
   public:
    using detail::partial_tuple<0, T...>::operator[];
    using detail::partial_tuple<0, T...>::decl_elem;

    template<class... U>
    void assign(U&&... values) {
        detail::assign(*this, indicies, std::forward<U>(values)...);
    }
    template<class F>
    decltype(auto) apply(F&& func) & {
        return detail::apply(*this, std::forward<F>(func), indicies);
    }
    template<class F>
    decltype(auto) apply(F&& func) const& {
        return detail::apply(*this, std::forward<F>(func), indicies);
    }
    template<class F>
    decltype(auto) apply(F&& func) && {
        return detail::apply(std::move(*this), std::forward<F>(func), indicies);
    }
    template<class... U>
    operator tuple<U...>() & {
        return convert(*this, indicies);
    }
    template<class... U>
    operator tuple<U...>() const & {
        return convert(*this, indicies);
    }
    template<class... U>
    operator tuple<U...>() && {
        return convert(std::move(*this), indicies);
    }
};
template<class... T>
tuple(T...) -> tuple<detail::unwrap_t<T>...>;

template<size_t I, class... T>
decltype(auto) get(tuple<T...>& tup) {
    return tup[index<I>()];
}
template<size_t I, class... T>
decltype(auto) get(tuple<T...> const& tup) {
    return tup[index<I>()];
}
template<size_t I, class... T>
decltype(auto) get(tuple<T...>&& tup) {
    return std::move(tup)[index<I>()];
}

template<class... T>
tuple<T&...> tie(T&... args) {
    return tuple<T&...>{args...};
}

namespace literals {
template<char... digits>
constexpr size_t from_digits() {
    size_t num = 0;
    constexpr bool digits_good = (('0' <= digits && digits <= '9') && ...);
    static_assert(digits_good, "Must be integral literal");
    return ((num = num * 10 + (digits - '0')) , ... , num);
}
template<char... digits>
constexpr auto operator""_tag() -> index<from_digits<digits...>()> {
    return {};
}
template<char... digits>
constexpr auto operator""_st() -> index<from_digits<digits...>()> {
    return {};
}
template<char... digits>
constexpr auto operator""_nd() -> index<from_digits<digits...>()> {
    return {};
}
template<char... digits>
constexpr auto operator""_rd() -> index<from_digits<digits...>()> {
    return {};
}
template<char... digits>
constexpr auto operator""_th() -> index<from_digits<digits...>()> {
    return {};
}
}
}

namespace std {
template<class... T>
struct tuple_size<tuplet::tuple<T...>>
  : std::integral_constant<size_t, sizeof...(T)> {};

template<size_t I, class... T>
struct tuple_element<I, tuplet::tuple<T...>> {
    using type = decltype(tuplet::tuple<T...>::decl_elem(tuplet::index<I>()));
};
}

