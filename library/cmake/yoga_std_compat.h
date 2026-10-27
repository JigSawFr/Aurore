// Forced into Yoga's sources: Yoga 3 uses C++20 library parts that the
// standard libraries of some toolchains lack (OpenOrbis' libc++ on PS4, the
// Android NDK's libc++, the Vita's libstdc++). Each one is defined here only
// when the library does not have it.
#pragma once

#include <cstring>
#include <iterator>
#include <limits>
#include <type_traits>
#if __has_include(<bit>)
#include <bit>
#endif
#if __has_include(<concepts>)
#include <concepts>
#endif

namespace std
{
#if !defined(__cpp_lib_bit_cast)
template <class To, class From>
#if __has_builtin(__builtin_bit_cast)
constexpr To bit_cast(const From& from) noexcept
{
    return __builtin_bit_cast(To, from);
}
#else
inline To bit_cast(const From& from) noexcept
{
    To to;
    std::memcpy(&to, &from, sizeof(To));
    return to;
}
#endif
#endif

#if !defined(__cpp_lib_int_pow2)
template <class T>
constexpr int bit_width(T value) noexcept
{
    int width = 0;
    for (; value != 0; value >>= 1)
        width++;
    return width;
}
#endif

#if !defined(__cpp_lib_concepts)
template <class T>
concept floating_point = is_floating_point_v<T>;

template <class I>
concept input_iterator = requires(I i) {
    typename iterator_traits<I>::value_type;
    *i;
    ++i;
};
#endif
} // namespace std
