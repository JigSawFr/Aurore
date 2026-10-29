// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Host tests for the boards' palettes (ui/cadran_palette.hpp): every pair
// CONTRAST promises, in both themes, read through KEYS as cadran::install_theme
// puts them in the themes: an ink at 4.5:1 or more on each surface it is drawn
// on, a translucent one blended first over the background (WCAG 1.4.3); the
// focus ring and its glow at 3:1 or more on the background (WCAG 1.4.11).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#include "check.h"

#include <aurore/cadran/ui/cadran_palette.hpp>

using cadran::palette::Palette;
using cadran::palette::Rgba;

namespace
{
double channel(uint8_t c)
{
    const double v = c / 255.0;
    return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
}

double luminance(Rgba c)
{
    return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b);
}

// `top` drawn over the opaque `under`.
Rgba over(Rgba top, Rgba under)
{
    const double a = top.a / 255.0;
    const auto mix = [a](uint8_t t, uint8_t u)
    { return (uint8_t)std::lround(t * a + u * (1 - a)); };
    return { mix(top.r, under.r), mix(top.g, under.g), mix(top.b, under.b), 255 };
}

double ratio(Rgba a, Rgba b)
{
    const double la = luminance(a), lb = luminance(b);
    return (std::fmax(la, lb) + 0.05) / (std::fmin(la, lb) + 0.05);
}

// The colour of theme key `name` in `p`, as install_theme sets it.
const Rgba* color(const Palette& p, const char* name)
{
    for (const auto& key : cadran::palette::KEYS)
        if (std::strcmp(key.name, name) == 0)
            return &(p.*key.color);
    return nullptr;
}

void check(const char* theme, const Palette& p)
{
    for (const auto& pair : cadran::palette::CONTRAST)
    {
        const Rgba* ink     = color(p, pair.ink);
        const Rgba* surface = color(p, pair.surface);
        if (!CHECK(ink && surface))
        {
            std::fprintf(stderr, "%s: no colour for %s or %s\n", theme, pair.ink, pair.surface);
            continue;
        }
        const Rgba under = over(*surface, p.bg);
        const double r   = ratio(over(*ink, under), under);
        if (!CHECK(r >= pair.minimum))
            std::fprintf(stderr, "%s: %s on %s %.2f:1, under %.1f:1\n", theme, pair.ink, pair.surface, r, pair.minimum);
    }
}
} // namespace

int main()
{
    // The arithmetic first: black on white is 21:1, a colour on itself 1:1.
    CHECK(std::fabs(ratio({ 0, 0, 0, 255 }, { 255, 255, 255, 255 }) - 21.0) < 0.01);
    CHECK(std::fabs(ratio({ 0x12, 0x34, 0x56, 255 }, { 0x12, 0x34, 0x56, 255 }) - 1.0) < 1e-9);
    // The pairs: the eight inks on the three surfaces, the inks on fills of
    // their own and the focus ring's two ends, at least.
    CHECK(std::size(cadran::palette::CONTRAST) >= 32);
    check("dark", cadran::palette::DARK);
    check("light", cadran::palette::LIGHT);
    return CHECK_DONE("palette contrast assertions passed");
}
