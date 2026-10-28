// ui — the palette's colours by role, and the console's system font (see ui.hpp).
// Copyright (C) 2026 JigSawFr, (C) 2026 Taylor.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/ui/ui.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <aurore/cadran/util/text.hpp>

namespace ui
{

#ifdef __SWITCH__
namespace
{
    // The default font's name registered again with the system's `type` of
    // shared font (its data is the system's, not copied), then the fallbacks the
    // Chinese font had, and `also` (fontstash looks one level deep). False when
    // the system does not give it.
    bool register_default(PlSharedFontType type, int also)
    {
        PlFontData data;
        if (R_FAILED(plGetSharedFontByType(&data, type)))
            return false;
        if (!aurore::Application::loadFontFromMemory(aurore::FONT_CHINESE_SIMPLIFIED, data.address, data.size, false))
            return false;
        const int font = aurore::Application::getFont(aurore::FONT_CHINESE_SIMPLIFIED);
        NVGcontext* vg = aurore::Application::getNVGContext();
        for (const std::string& other : { aurore::FONT_REGULAR, aurore::FONT_CHINESE_SIMPLIFIED_EXT, aurore::FONT_CHINESE_TRADITIONAL,
                 aurore::FONT_KOREAN_REGULAR, aurore::FONT_SWITCH_ICONS, aurore::FONT_MATERIAL_ICONS,
                 aurore::FONT_EMOJI })
        {
            const int id = aurore::Application::getFont(other);
            if (id != aurore::FONT_INVALID && id != font)
                nvgAddFallbackFontId(vg, font, id);
        }
        if (also != aurore::FONT_INVALID)
            nvgAddFallbackFontId(vg, font, also);
        return true;
    }
} // namespace
#endif

void use_font_for(const std::string& locale)
{
#ifdef __SWITCH__
    // borealis (wiliwili's branch) draws every label with the Simplified
    // Chinese system font, the Latin one only filling its gaps. That font has
    // the pinyin letters é, è, ê… as full-width glyphs ("prot é g é e"), and a
    // full-width "·" and "…". Chinese keeps it: the han in its own style. The
    // default font is looked up for each new label.
    // Traditional Chinese takes its own font: the Simplified one draws the
    // characters both share in their mainland forms.
    static const int chinese        = aurore::Application::getFont(aurore::FONT_CHINESE_SIMPLIFIED); // as loaded
    static PlSharedFontType current = PlSharedFontType_ChineseSimplified; // what the name holds now
    const bool traditional          = text::starts_with(locale, "zh-Hant") || text::starts_with(locale, "zh-TW")
        || text::starts_with(locale, "zh-HK");
    const PlSharedFontType want = !text::starts_with(locale, "zh") ? PlSharedFontType_Standard
        : traditional                                              ? PlSharedFontType_ChineseTraditional
                                                                   : PlSharedFontType_ChineseSimplified;
    if (want == current)
        return;
    if (register_default(want, want == PlSharedFontType_ChineseSimplified ? aurore::FONT_INVALID : chinese))
        current = want;
#else
    (void)locale;
#endif
}

} // namespace ui
