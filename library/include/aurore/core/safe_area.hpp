// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

// The safe area: the part of the window clear of display cutouts, rounded
// corners, system bars and a TV's overscan (docs/safe-areas.md).

#include <cstdint>
#include <string>
#include <utility>

namespace aurore
{

/// Space at each edge of a rectangle
struct Insets
{
    float top    = 0;
    float right  = 0;
    float bottom = 0;
    float left   = 0;

    bool empty() const { return top <= 0 && right <= 0 && bottom <= 0 && left <= 0; }
    bool operator==(const Insets& other) const
    {
        return top == other.top && right == other.right && bottom == other.bottom && left == other.left;
    }
    bool operator!=(const Insets& other) const { return !(*this == other); }

    /// Read as CSS reads a padding: "a" (every edge), "v,h", "t,h,b" or
    /// "t,r,b,l", numbers separated by commas or spaces, none negative.
    /// False, `out` unchanged, for anything else.
    static bool parse(const std::string& text, Insets* out);
};

/// The edges of a box that keep clear of the safe area (Box::setSafeArea)
enum class SafeAreaEdges : uint8_t
{
    NONE       = 0,
    TOP        = 1,
    RIGHT      = 2,
    BOTTOM     = 4,
    LEFT       = 8,
    VERTICAL   = TOP | BOTTOM,
    HORIZONTAL = LEFT | RIGHT,
    ALL        = TOP | RIGHT | BOTTOM | LEFT,
    /// None, but every edge for the content view of an activity: the default
    AUTO = 16,
};

constexpr SafeAreaEdges operator|(SafeAreaEdges a, SafeAreaEdges b)
{
    return (SafeAreaEdges)(std::to_underlying(a) | std::to_underlying(b));
}

constexpr SafeAreaEdges operator&(SafeAreaEdges a, SafeAreaEdges b)
{
    return (SafeAreaEdges)(std::to_underlying(a) & std::to_underlying(b));
}

constexpr SafeAreaEdges operator~(SafeAreaEdges a)
{
    return (SafeAreaEdges)(~std::to_underlying(a) & std::to_underlying(SafeAreaEdges::ALL));
}

/// True when `edges` has `edge`
constexpr bool hasEdge(SafeAreaEdges edges, SafeAreaEdges edge)
{
    return (std::to_underlying(edges) & std::to_underlying(edge)) != 0;
}

/// The edges `text` names: `top`, `right`, `bottom`, `left`, `vertical`,
/// `horizontal`, `all`, `none` or `auto`, several joined with `|` (or
/// commas, spaces). False, `out` unchanged, for an unknown name or `auto`
/// with another one.
bool parseSafeAreaEdges(const std::string& text, SafeAreaEdges* out);

} // namespace aurore
