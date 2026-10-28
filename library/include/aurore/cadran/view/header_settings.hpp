// HeaderSetting — the language and the theme in the main screen's header,
// left of the notification bell: the language's code ("FR", "EN", the
// console's own one for "system") in a rounded frame, and the theme as a
// sun (light), a moon (dark) or a half-filled disc (the console's); the
// text size after them, as "AA" (a small A and
// a large one, the large one teal while the text is larger). A code
// rather than a flag: a flag names a country, not a language, and fr-CA,
// es-419 or zh-Hant have none that fits. A tap opens the same list as
// Preferences (ui::pick_language / pick_theme / pick_text_size); not
// focusable, like an app's bell: − opens them from any tab (ui::quick_settings),
// as the footer says.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <aurore.hpp>

/// The language, theme and text size in the main screen's header: a tap opens
/// their picker.
class HeaderSetting : public brls::View
{
  public:
    enum class Kind
    {
        LANGUAGE,
        THEME,
        TEXT_SIZE
    };

    explicit HeaderSetting(Kind kind);

    /// Puts them in the main screen's header, when it first takes the focus
    /// (after NoticeBell::install's bell). Called once, at start-up.
    static void install();

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
        brls::FrameContext* ctx) override;
    /// Touched although it is not focusable.
    brls::View* hitTest(brls::Point point) override;

  protected:
    /// A button: "Language, Français (France)", "Theme, Dark", "Text size, Larger".
    brls::AccessibilityRole getDefaultAccessibilityRole() override { return brls::AccessibilityRole::BUTTON; }
    std::string getDefaultAccessibilityLabel() override;

  private:
    Kind kind;

    void draw_language(NVGcontext* vg, float x, float y, float width, float height);
    void draw_theme(NVGcontext* vg, float x, float y, float width, float height);
    void draw_text_size(NVGcontext* vg, float x, float y, float width, float height);
};
