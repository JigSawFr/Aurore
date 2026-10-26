// ps4 platform
// OpenOrbis' libc++ predates parts of C++20 that Yoga 3 uses; this header,
// forced into Yoga's sources on PS4 only, adds those that are missing.
#pragma once

#include <cstring>
#include <iterator>
#include <type_traits>

namespace std
{
#if !defined(__cpp_lib_bit_cast)
template <class To, class From>
constexpr To bit_cast(const From& from) noexcept
{
    return __builtin_bit_cast(To, from);
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
