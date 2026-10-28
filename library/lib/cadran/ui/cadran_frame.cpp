// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <borealis/cadran/ui/cadran_frame.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <borealis/cadran/ui/cadran_kit.hpp>
#include <borealis/cadran/ui/cadran_list.hpp>
#include <borealis/cadran/ui/cadran_modal.hpp>
#include <borealis/cadran/ui/cadran_palette.hpp>
#include <borealis/cadran/ui/effects.hpp>
#include <borealis/cadran/ui/hooks.hpp>
#include <borealis/cadran/ui/motion.hpp>
#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/cadran/ui/ui.hpp>
#include <borealis/cadran/view/cadran_icon.hpp>
#include <borealis/cadran/view/scroll_view.hpp>
#include <borealis/cadran/view/tab_badge.hpp>

using namespace brls::literals;
namespace kit = cadran::kit;

namespace
{
// How much the frame's texts grow with the larger text (the content's:
// text_size::LARGE): the rail's labels as much, as their room allows (the
// rail keeps its width, the screens theirs), the header's and the footer's
// a little less, their rows being full already.
constexpr float RAIL_SCALE  = 1.25f;
constexpr float TITLE_SCALE = 28.0f / 24.0f;
constexpr float PILL_SCALE  = 1.2f;
constexpr float DATE_SCALE  = 1.15f;
constexpr float HINTS_SCALE = 1.15f;

// The rail's items (style_rail): their inner padding and their icons' size;
// and the header's mark.
constexpr float RAIL_ITEM_PAD = 12.0f;
constexpr float RAIL_ICON     = 22.0f;
constexpr float MARK_SIZE     = 38.0f;
// The rail open (brls/tab_frame/sidebar_width) and on its icons: the items'
// column inset 16 px on both sides, the icons centred under the header's mark.
constexpr float RAIL_OPEN      = 232.0f;
constexpr float RAIL_SHUT      = 16.0f + RAIL_ITEM_PAD * 2 + RAIL_ICON + 16.0f;
constexpr float RAIL_SHUT_ITEM = RAIL_ITEM_PAD * 2 + RAIL_ICON;
// How open the rail is now, 0 (its icons) to 1 (its names): what the active
// tab's pill and the badges are drawn by (RailFold sets it each frame).
float s_rail_open = 1.0f;
} // namespace

void cadran::install_theme()
{
    namespace pal = cadran::palette;
    // The palettes and why they are what they are: ui/cadran_palette.hpp.
    const std::pair<brls::Theme*, const pal::Palette*> variants[] = { { &brls::Theme::getDarkTheme(), &pal::DARK },
        { &brls::Theme::getLightTheme(), &pal::LIGHT } };
    const auto c                                                  = [](pal::Rgba x)
    { return nvgRGBA(x.r, x.g, x.b, x.a); };
    for (const auto& [t, p] : variants)
    {
        for (const pal::Key& key : pal::KEYS)
            t->addColor(key.name, c(p->*key.color));
        t->addColor("cadran/none", nvgRGBA(0, 0, 0, 0));
#ifndef __SWITCH__
        // The still focus of an app's screenshot tests (STILL_FOCUS): the glow in
        // the stroke's own colour, so a screenshot does not depend on when.
        if (ui::env_flag("STILL_FOCUS"))
            t->addColor("brls/highlight/color2", c(p->hl1));
#endif
    }
    for (const pal::Contrast& pair : pal::CONTRAST)
        brls::Theme::addContrastCheck(pair.ink, pair.surface, pair.minimum);
    // Cards and panels: a theme's background image shows through them (surface_opacity),
    // the tinted ones too (a warning card, a checked row, the status pills)
    for (const char* key : { "cadran/surf", "cadran/surf2", "cadran/amber_surf", "cadran/teal_soft", "cadran/coral_soft",
             "cadran/pill_ok" })
        brls::Theme::addSurfaceKey(key);
    // The icons a theme may draw as pictures (icon.<name>)
    brls::Theme::addIconNames(CadranIcon::names());
#ifndef __SWITCH__
    // And held where it is (brls::setHighlightAnimationStill). The footer's hints too:
    // drawn as once a screen or a dialog has opened, not greyed while its
    // fade blocks the input (0014). A slow frame (the web simulator's
    // software WebGL) could otherwise catch them greyed.
    if (ui::env_flag("STILL_FOCUS"))
    {
        brls::setHighlightAnimationStill(true);
        brls::setHintsSteady(true);
        brls::View::setHighlightGlide(false); // drawn where the focus is, at once
    }
#endif
    // The footer's hints and clock grow less than the rest with the larger
    // text: a footer full of hints still has room for them (brls::Label::setFontScale).
    brls::setHintsFontScaleMax(HINTS_SCALE);
    brls::Style style = brls::getStyle();
    // The corners of the kit, for the XML (@style/cadran/radius/card…).
    style.addMetric("cadran/radius/row", kit::RADIUS_ROW);
    style.addMetric("cadran/radius/icon", kit::RADIUS_ICON);
    style.addMetric("cadran/radius/card", kit::RADIUS_CARD);
    style.addMetric("cadran/radius/top", kit::RADIUS_TOP);
    style.addMetric("cadran/radius/modal", kit::RADIUS_MODAL);
    // The gap between a filled button and the focus ring around it
    style.addMetric("cadran/ring_gap", 4.0f);
    style.addMetric("brls/applet_frame/header_height", 84.0f);
    // 232 px, not the boards' 196: the console's font is wider than the
    // boards' (Nunito Sans), and the longest names must stay whole
    // (PlayGuard's "Tiempo de juego", "Einschränkungen" and their badges).
    style.addMetric("brls/tab_frame/sidebar_width", RAIL_OPEN);
    // A tab's own content (@style/brls/tab_frame/content_padding_*): the
    // margins of Cadran's pages, the cards' edges
    style.addMetric("brls/tab_frame/content_padding_sides", 40.0f);
    style.addMetric("brls/tab_frame/content_padding_top_bottom", 28.0f);
    style.addMetric("brls/sidebar/padding_top", 6.0f);
    style.addMetric("brls/sidebar/padding_right", 8.0f);
    style.addMetric("brls/sidebar/padding_bottom", 10.0f);
    style.addMetric("brls/sidebar/padding_left", 16.0f);
    style.addMetric("brls/sidebar/item_height", 52.0f);
    // The pickers' rows inside their card (cadran_modal), the toasts as
    // cards under the header, and the pills' hairline.
    style.addMetric("brls/dropdown/listPadding", 14.0f);
    style.addMetric("brls/dropdown/listPaddingSides", 20.0f);
    style.addMetric("brls/notification/width", 400.0f);
    style.addMetric("brls/notification/padding", 18.0f);
    style.addMetric("brls/notification/corner_radius", 16.0f);
    style.addMetric("brls/notification/margin_top", 96.0f);
    style.addMetric("brls/notification/margin_right", 32.0f);
    style.addMetric("brls/notification/spacing", 10.0f);
    style.addMetric("cadran/hairline", 1.0f);
    // The boards' display face for the figures (Sora Bold, OFL).
    if (!brls::Application::loadFontFromFile("cadran/digits", BRLS_ASSET("font/sora_bold_latin.ttf")))
    {
        brls::Logger::warning("cadran: display font not loaded, the default one is used");
        return;
    }
    // Latin only: the units in other scripts ("40 мин", "+1 ч", "40分") from
    // the app's own fonts (fontstash looks one level deep: each of them).
    NVGcontext* vg   = brls::Application::getNVGContext();
    const int digits = brls::Application::getFont("cadran/digits");
    for (int font : { brls::Application::getDefaultFont(), brls::Application::getFont(brls::FONT_REGULAR),
             brls::Application::getFont(brls::FONT_CHINESE_SIMPLIFIED),
             brls::Application::getFont(brls::FONT_CHINESE_SIMPLIFIED_EXT),
             brls::Application::getFont(brls::FONT_CHINESE_TRADITIONAL),
             brls::Application::getFont(brls::FONT_JAPANESE),
             brls::Application::getFont(brls::FONT_KOREAN_REGULAR) })
        if (font != brls::FONT_INVALID && font != digits)
            nvgAddFallbackFontId(vg, digits, font);
}

namespace
{
// The frame (header, rail) with Preferences › Text size's larger text
// (ui/text_size): at most `scale` times its size, as the room allows, and
// smaller again, down to its size, where a language's words need it
// (brls::Label::setFontScale). The normal text size leaves it as it is.
void chrome(brls::Label* label, float size, float scale)
{
    label->setFontSize(size);
    label->setFontScaleMax(scale);
    label->setShrinkToFit(true);
}

class Glow : public brls::View
{
  public:
    Glow(float centre_x, float centre_y, float r, const char* color_key)
        : cx(centre_x)
        , cy(centre_y)
        , radius(r)
        , key(color_key)
    {
        this->setFocusable(false);
        this->setPositionType(brls::PositionType::ABSOLUTE);
        this->setPositionTop(0);
        this->setPositionLeft(0);
        this->setWidth(brls::Application::contentWidth);
        this->setHeight(brls::Application::contentHeight);
    }
    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style, brls::FrameContext* ctx) override
    {
        static const bool off = ui::env_flag("CADRAN_NOGLOW");
        if (off || !ui::prefs().glow || effects::light())
            return;
        // The light drifts, slowly (a turn in 24 s, a little wider and
        // narrower on the way): the screen breathes, nothing else moves.
        // One gradient either way: the drift costs nothing.
        float gx = this->cx, gy = this->cy, gr = this->radius;
        if (effects::motion())
        {
            const double t = (double)(brls::getCPUTimeUsec() % 24000000) / 24000000.0 * 2.0 * NVG_PI;
            gx += (float)std::sin(t) * this->radius * 0.035f;
            gy += (float)std::cos(t * 0.7) * this->radius * 0.025f;
            gr *= 1.0f + 0.05f * (float)std::sin(t * 1.3);
        }
        const NVGcolor glow  = ctx->theme[this->key];
        NVGcolor clear       = glow;
        clear.a              = 0.0f;
        const NVGpaint paint = nvgRadialGradient(vg, x + gx, y + gy, 0.0f, gr, glow, clear);
        // Only the square around the light: past its radius it is clear, so
        // the screen's other pixels are left alone (the same picture, about
        // half the fill).
        const float left = std::max(x, x + gx - gr), top = std::max(y, y + gy - gr);
        const float right = std::min(x + width, x + gx + gr), bottom = std::min(y + height, y + gy + gr);
        if (right <= left || bottom <= top)
            return;
        nvgBeginPath(vg);
        nvgRect(vg, left, top, right - left, bottom - top);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }

  private:
    float cx, cy, radius;
    const char* key; // the light's colour
};

// A sidebar item's words and icon in one colour (teal when active, else
// text2), after the library's own colouring of the frame: a theme key at
// rest, a colour on the way from one to the other.
void tint_item(brls::SidebarItem* it, NVGcolor color, const char* key = nullptr)
{
    for (brls::View* c : it->getChildren())
    {
        if (!dynamic_cast<brls::Label*>(c) && !dynamic_cast<CadranIcon*>(c))
            continue;
        const char* property = dynamic_cast<brls::Label*>(c) ? "textColor" : "color";
        if (key)
        {
            c->setThemeColor(property, key);
        }
        else
        {
            c->setThemeColor(property, "");
            c->setStyleProperty(property, brls::StyleValue::of(color));
        }
    }
}

// The active tab's pill, drawn behind the rail's items: it slides from the
// tab left to the tab chosen, the two tabs' colours crossing meanwhile; at
// rest, the same rounded rectangle the item's own background would be.
// Still with the motion off (ui/effects). Held by the first item, as its
// first child (drawn after that item's own, empty, background and before
// everything else of the rail), never by the items' column: the library
// finds a tab by its place among that column's children (Sidebar::getItem).
class RailPill : public brls::View
{
  public:
    explicit RailPill(std::vector<brls::SidebarItem*> rail)
        : items(std::move(rail))
    {
        this->setFocusable(false);
        this->setPositionType(brls::PositionType::ABSOLUTE);
        this->setWidth(0);
        this->setHeight(0);
        this->setCulled(false);
    }

    // `item` is the active tab now: the pill goes there.
    void activate(brls::SidebarItem* item)
    {
        if (item == this->to)
            return;
        this->from  = this->to;
        this->to    = item;
        this->since = this->from && effects::motion() ? motion::now() : 0;
        this->tint(1.0f);
    }

    void draw(NVGcontext* vg, float, float, float, float, brls::Style, brls::FrameContext*) override
    {
        if (!this->to)
            return;
        const float t = motion::ease_out(motion::progress(this->since, SLIDE_US));
        brls::Rect at = this->to->getFrame();
        if (this->since && this->from && t < 1.0f)
        {
            const brls::Rect was = this->from->getFrame();
            at                   = brls::Rect(was.getMinX() + (at.getMinX() - was.getMinX()) * t, was.getMinY() + (at.getMinY() - was.getMinY()) * t,
                                  was.getWidth() + (at.getWidth() - was.getWidth()) * t, was.getHeight() + (at.getHeight() - was.getHeight()) * t);
            this->tint(t);
        }
        else if (this->since)
        {
            this->since = 0; // arrived: the colours settled once
            this->tint(1.0f);
        }
        // On the icon alone while the rail is folded, as wide as the name as it opens.
        const float width = std::min(at.getWidth(), RAIL_SHUT_ITEM + (at.getWidth() - RAIL_SHUT_ITEM) * s_rail_open);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, at.getMinX(), at.getMinY(), width, at.getHeight(), this->to->getCornerRadius());
        NVGcolor fill = cadran::kit::color("cadran/rail_active");
        fill.a *= this->to->getAlpha();
        nvgFillColor(vg, fill);
        nvgFill(vg);
    }

  private:
    // The two tabs' colours `t` of the way across, the others' at rest.
    void tint(float t)
    {
        brls::Theme theme = brls::Application::getTheme();
        const NVGcolor on = theme["cadran/teal"], off = theme["cadran/text2"];
        for (brls::SidebarItem* it : this->items)
        {
            if (t >= 1.0f)
                tint_item(it, on, it == this->to ? "cadran/teal" : "cadran/text2");
            else if (it == this->to)
                tint_item(it, nvgLerpRGBA(off, on, t));
            else if (it == this->from)
                tint_item(it, nvgLerpRGBA(on, off, t));
            else
                tint_item(it, off, "cadran/text2");
        }
    }

    static constexpr float SLIDE_US = 140000.0f;
    std::vector<brls::SidebarItem*> items;
    brls::SidebarItem* from = nullptr;
    brls::SidebarItem* to   = nullptr;
    brls::Time since        = 0;
};
} // namespace

namespace
{
// The header's pill: not focusable, but a tap reaches it (brls::View::hitTest
// skips a view that cannot take the focus).
class HeaderPill : public brls::Box
{
  public:
    HeaderPill()
        : brls::Box(brls::Axis::ROW)
    {
    }

    brls::View* hitTest(brls::Point point) override
    {
        if (this->getAlpha() == 0.0f || this->getVisibility() != brls::Visibility::VISIBLE)
            return nullptr;
        return this->getFrame().pointInside(point) ? this : nullptr;
    }
};

// The main screen's pill (style_frame): its label and icon, and the pill for
// hiding and colouring it.
brls::Box* s_main_pill = nullptr;
// What a tap on it opens now (the app's mode's), nullptr: nothing.
std::function<void()> s_pill_tap;
brls::Label* s_main_pill_label = nullptr;
CadranIcon* s_main_pill_icon   = nullptr;
// What the screen shown last asked the pill to say.
std::string s_pill_text;
std::string s_pill_glyph = "check";
cadran::Tone s_pill_tone = cadran::Tone::OK;
} // namespace

void cadran::set_pill(const std::string& text, const char* glyph, Tone tone)
{
    s_pill_text  = text;
    s_pill_glyph = glyph ? glyph : "check";
    s_pill_tone  = tone;
    refresh_pill();
}

void cadran::refresh_pill()
{
    if (!s_main_pill || !s_main_pill_label || !s_main_pill_icon)
        return;
    // The app's mode, then read-only: that first, on every screen (what a
    // tab says comes back once it is over).
    const auto& h = ui::hooks::get();
    PillMode mode;
    bool moded = h.pill && h.pill(mode);
    if (!moded && h.read_only && h.read_only())
    {
        mode.text = "cadran/pill/read_only"_i18n;
        mode.icon = "pencil-off";
        mode.tone = Tone::WARN;
        moded     = true;
    }
    s_pill_tap             = moded ? mode.on_tap : nullptr;
    const std::string text = moded ? mode.text : s_pill_text;
    const char* glyph      = moded ? mode.icon.c_str() : s_pill_glyph.c_str();
    const Tone tone        = moded ? mode.tone : s_pill_tone;
    const char* ink        = tone == Tone::DANGER ? "cadran/coral" : tone == Tone::WARN ? "cadran/amber"
                                                                                        : "cadran/pill_ok_ink";
    s_main_pill_label->setText(text);
    s_main_pill_label->setThemeColor("textColor", ink);
    s_main_pill_icon->setIcon(glyph);
    s_main_pill_icon->setThemeColor("color", ink);
    s_main_pill->setThemeColor("backgroundColor", tone == Tone::DANGER ? "cadran/coral_soft" : tone == Tone::WARN ? "cadran/amber_surf"
                                                                                                                  : "cadran/pill_ok");
    s_main_pill->setThemeColor("borderColor", tone == Tone::DANGER ? "cadran/coral_line" : "cadran/amber_line");
    s_main_pill->setBorderThickness(tone == Tone::OK ? 0 : 1);
    s_main_pill->setVisibility(text.empty() ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
}

brls::View* cadran::make_glow(float cx, float cy, float radius, const char* color_key)
{
    return new Glow(cx, cy, radius, color_key);
}

void cadran::style_header(brls::AppletFrame* frame, const char* pill_icon, const std::string& pill_text, bool filled, bool with_date)
{
    if (!frame || frame->getView("cadran_mark"))
        return;
    brls::View* app_icon = frame->getView("brls/applet_frame/title_icon");
    brls::Box* left      = app_icon ? app_icon->getParent() : nullptr;
    if (app_icon)
        app_icon->setVisibility(brls::Visibility::GONE);
    if (left)
    {
        left->setAlignItems(brls::AlignItems::CENTER);
        left->setGrow(1.0f);
        CadranIcon* mark = kit::icon("mark", MARK_SIZE, "cadran/text");
        mark->setId("cadran_mark");
        mark->setMarginRight(14);
        if (with_date)
        {
            // The main screen: the mark centred over the rail's icons.
            brls::Style style = brls::getStyle();
            const float icons = style["brls/sidebar/padding_left"] + RAIL_ITEM_PAD + RAIL_ICON / 2;
            const float start = style["brls/applet_frame/padding_sides"] + style["brls/applet_frame/header_padding_sides"];
            mark->setMarginLeft(icons - MARK_SIZE / 2 - start);
            mark->setCulled(false); // left of its box, which borealis would take for off-screen
        }
        left->addView(mark, 0);
    }
    if (auto* title = dynamic_cast<brls::Label*>(frame->getView("brls/applet_frame/title_label")))
    {
        chrome(title, 24, TITLE_SCALE);
        title->setMarginTop(0);
    }
    if (!pill_icon)
        return; // a screen of its own: the mark and the title
    auto* right = new brls::Box(brls::Axis::ROW);
    right->setAlignItems(brls::AlignItems::CENTER);
    const char* ink = filled ? "cadran/pill_ok_ink" : "cadran/text2";
    auto* pill      = new HeaderPill();
    pill->setAlignItems(brls::AlignItems::CENTER);
    pill->setHeight(32);
    pill->setPaddingLeft(14);
    pill->setPaddingRight(14);
    pill->setCornerRadius(16);
    if (filled)
    {
        pill->setThemeColor("backgroundColor", "cadran/pill_ok");
    }
    else
    {
        pill->setThemeColor("borderColor", "cadran/line");
        pill->setBorderThickness(1);
    }
    CadranIcon* glyph = kit::icon(pill_icon, 14, ink);
    glyph->setMarginRight(8);
    pill->addView(glyph);
    auto* pill_label = new brls::Label();
    pill_label->setText(pill_text);
    chrome(pill_label, 15, PILL_SCALE);
    pill_label->setThemeColor("textColor", ink);
    pill->addView(pill_label);
    right->addView(pill);
    if (with_date)
    { // the main screen: its pill is the one set_pill() changes
        pill->addGestureRecognizer(new brls::TapGestureRecognizer(pill, []()
            {
            if (s_pill_tap) s_pill_tap(); }));
        s_main_pill       = pill;
        s_main_pill_label = pill_label;
        s_main_pill_icon  = glyph;
        auto* date        = new brls::Label();
        date->setText(cadran::kit::long_date());
        chrome(date, 16, DATE_SCALE);
        date->setThemeColor("textColor", "cadran/text3");
        date->setMarginLeft(10);
        right->addView(date);
    }
    right->setMarginRight(16); // clear of the header's tools (language, theme, bell) after it
    if (brls::Box* header = frame->getHeader())
        header->addView(right, 1);
}

namespace
{
// The icons of the lines of the screens dressed here, by id (the screens
// with a list of their own pass theirs to cadran::List).
const std::vector<std::pair<const char*, const char*>> SCREEN_ICONS = {
    // An update
    { "up_update", "download" },
    { "up_method", "settings" },
    { "up_later", "calendar" },
    // What's new
    { "wn_full", "list" },
    { "wn_close", "check" },
};

// Where `frame` was opened from: the title of the screen under it, or the
// main screen's tab; "" when it is not known.
std::string crumb_of(brls::AppletFrame* frame)
{
    // A new screen takes the focus before it is on the stack: the one under
    // it is then the top one.
    const std::vector<brls::Activity*> stack = brls::Application::getActivitiesStack();
    brls::View* below                        = nullptr;
    for (size_t i = 0; i < stack.size(); i++)
        if (stack[i]->getContentView() == frame)
            below = i > 0 ? stack[i - 1]->getContentView() : nullptr;
    if (!below && !stack.empty() && stack.back()->getContentView() != frame)
        below = stack.back()->getContentView();
    if (!below)
        return "";
    if (cadran::is_main_frame(below))
    {
        const auto& h         = ui::hooks::get();
        const std::string tab = h.main_screen ? h.main_screen() : ""; // "tab/console"
        return h.tab_title && !tab.empty() ? h.tab_title(tab) : "";
    }
    auto* title = dynamic_cast<brls::Label*>(below->getView("brls/applet_frame/title_label"));
    return title ? title->getFullText() : "";
}

// Before a screen's title, small: where it was opened from ("Console ›").
void add_crumb(brls::AppletFrame* frame)
{
    auto* title = dynamic_cast<brls::Label*>(frame->getView("brls/applet_frame/title_label"));
    auto* row   = title ? dynamic_cast<brls::Box*>(title->getParent()) : nullptr;
    // Not on a screen of its own, which is not a step of the one below.
    const std::string crumb = row && !cadran::is_standalone_screen(frame) ? crumb_of(frame) : "";
    // Nothing said twice (a game's screen is titled as its tab: Activity).
    if (crumb.empty() || title->getFullText().empty() || crumb == title->getFullText())
        return;
    brls::Label* label = cadran::kit::label(crumb + "  ›", cadran::kit::TEXT_LEAD, "cadran/text3");
    label->setSingleLine(true);
    label->setMarginRight(10);
    const auto& views = row->getChildren();
    row->addView(label, std::find(views.begin(), views.end(), title) - views.begin());
}

// Every screen the first time it takes the focus: a main screen (a TabFrame
// in it) as style_frame() does, rail and all, unless the app styled it
// already; the others, the header after the boards, and their list in
// cards; a dialog as a card. Once per screen: the mark it adds says it is
// done.
void skin_screen_of(brls::View* focus)
{
    if (!focus)
        return;
    while (focus->hasParent())
        focus = focus->getParent();
    if (auto* dialog = dynamic_cast<brls::Dialog*>(focus))
    {
        cadran::style_dialog(dialog);
        return;
    }
    auto* frame = dynamic_cast<brls::AppletFrame*>(focus);
    if (!frame || frame->getView("cadran_mark"))
        return;
    if (!frame->getView("brls/applet_frame/header"))
        return;
    if (cadran::is_main_frame(frame))
    {
        // The TabFrame itself: the rail (RailFold) takes its sidebar out of it.
        brls::View* sidebar = frame->getView("brls/tab_frame/sidebar");
        brls::View* tabs    = sidebar && sidebar->hasParent() ? sidebar->getParent() : frame->getView("main_tabs");
        cadran::style_frame(frame, tabs);
        return;
    }
    cadran::style_header(frame, nullptr, "", false, false);
    add_crumb(frame);
    if (!frame->getView("cadran_list"))
    { // a Cadran screen dresses itself
        auto icons = SCREEN_ICONS;
        if (ui::hooks::get().screen_icons)
            for (const auto& line : ui::hooks::get().screen_icons())
                icons.push_back(line);
        cadran::List(frame, true).apply(icons);
    }
    // The screen's content rises into place under its header (the main
    // screen's tabs do the same: TabBase).
    motion::reveal(frame->getContentView(), 8.0f, 180);
}
} // namespace

void cadran::install_screens()
{
    static bool done = false;
    if (done)
        return;
    done = true;
    brls::Application::getGlobalFocusChangeEvent()->subscribe(skin_screen_of);
}

namespace
{
// The sidebar as ui::prefs() says (ui::RAILS): the rail open as it always was, on
// its icons, or (the default) on its icons while a tab has the focus and
// open over the tab while the focus is on the rail, to choose one.
enum class RailMode
{
    AUTO,
    OPEN,
    ICONS
};

RailMode rail_mode()
{
    const std::string& r = ui::prefs().rail;
    return r == "open" ? RailMode::OPEN : r == "icons" ? RailMode::ICONS
                                                       : RailMode::AUTO;
}

// The sidebar, moved into this box (the TabFrame's first child, in its
// place): it folds the rail. Open or on its icons, a narrower layout and
// nothing more. Automatic, the box is out of the row's flow, as wide as
// the open rail, with a spacer of the icons' width after it for the tabs:
// it opens over the tab, which keeps its width, and draws only the part of
// the rail that is out (a scissor), the names fading in. It is drawn last,
// over the tab, by a RailLayer at the end of the frame.
class RailFold : public brls::Box
{
  public:
    RailFold(brls::View* tab_frame, brls::Sidebar* sidebar, brls::Box* list, std::vector<brls::SidebarItem*> rail)
        : brls::Box(brls::Axis::ROW)
        , tabs(tab_frame)
        , bar(sidebar)
        , column(list)
        , items(std::move(rail))
    {
        this->setFocusable(false);
        this->setAlignItems(brls::AlignItems::STRETCH);
        this->setPositionTop(0);
        this->setPositionBottom(0);
        this->setPositionLeft(0);
        for (brls::View* v : list->getChildren())
            if (!dynamic_cast<brls::SidebarItem*>(v))
                this->separators.push_back(v);
        this->spacer = new brls::Box();
        this->spacer->setFocusable(false);
        this->spacer->setWidth(RAIL_SHUT);
        s_fold = this;
    }

    ~RailFold() override
    {
        if (s_fold == this)
            s_fold = nullptr;
        s_rail_open = 1.0f;
    }

    static RailFold* current() { return s_fold; }

    // The TabFrame's first two children: this box (the sidebar inside), then
    // the spacer; the tab, added after them, stays last.
    void install()
    {
        auto* frame = dynamic_cast<brls::Box*>(this->tabs);
        frame->removeView(this->bar, false);
        this->addView(this->bar);
        frame->addView(this, 0);
        frame->addView(this->spacer, 1);
        this->apply();
    }

    // The layout of Preferences › Sidebar's choice, now.
    void apply()
    {
        this->mode      = rail_mode();
        const bool fold = this->mode == RailMode::AUTO, icons = this->mode == RailMode::ICONS;
        this->setPositionType(fold ? brls::PositionType::ABSOLUTE : brls::PositionType::RELATIVE);
        const float width = icons ? RAIL_SHUT : RAIL_OPEN;
        this->setWidth(width);
        this->bar->setWidth(width);
        this->spacer->setVisibility(fold ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
        this->column->setPaddingRight(icons ? 16 : 8);
        for (brls::SidebarItem* item : this->items)
        {
            if (auto* label = item->getView("brls/sidebar/item_label"))
            {
                label->setVisibility(icons ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
                label->setAlpha(1.0f);
            }
            item->setHideHighlight(false);
        }
        // Automatic: drawn by this box, as long as the rail is out.
        for (brls::View* v : this->separators)
            v->setVisibility(fold ? brls::Visibility::INVISIBLE : brls::Visibility::VISIBLE);
        // Open while the focus is on the rail (or nowhere yet: the start).
        this->target = !fold || this->focus_on_rail();
        this->from   = this->target ? 1.0f : 0.0f;
        this->since  = 0;
        this->settle();
    }

    // The focus moved: automatic, the rail opens when it reaches the rail
    // and folds when it leaves for the tab. Elsewhere (a dialog, another
    // screen, the header's tools), it stays as it is.
    void focus_changed(brls::View* focus)
    {
        if (this->mode != RailMode::AUTO || !focus)
            return;
        bool rail     = false;
        brls::View* v = focus;
        while (v && v != this->tabs)
        {
            if (v == this->bar)
                rail = true;
            v = v->hasParent() ? v->getParent() : nullptr;
        }
        if (!v || rail == this->target)
            return;
        this->from   = this->openness();
        this->target = rail;
        this->since  = effects::motion() ? motion::now() : 0;
    }

    // How open the rail is, 0..1 (eased).
    float openness()
    {
        if (this->mode == RailMode::OPEN)
            return 1.0f;
        if (this->mode == RailMode::ICONS)
            return 0.0f;
        const float to = this->target ? 1.0f : 0.0f;
        if (!this->since)
            return to;
        const float p = motion::progress(this->since, FOLD_US);
        if (p >= 1.0f)
            this->since = 0;
        return this->from + (to - this->from) * motion::ease_out(p);
    }

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override
    {
        this->settle();
        if (this->mode != RailMode::AUTO)
        {
            brls::Box::draw(vg, x, y, width, height, style, ctx);
            return;
        }
        if (!this->layer_pass)
            return; // the RailLayer's turn, over the tab
        const float t = s_rail_open, out = this->visible();
        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, out, height);
        // Over a theme's picture, a translucent rail would show the tab under
        // it: the picture first, as behind the screens
        if (brls::Application::hasThemeBackground() && ctx->theme["brls/sidebar/background"].a < 1.0f)
            brls::Application::drawThemeBackground(vg, x, y, out, height);
        brls::Box::draw(vg, x, y, width, height, style, ctx);
        // The separators, from their left end to as far as the rail is out.
        const NVGcolor line = brls::Application::getTheme()["brls/sidebar/separator"];
        for (brls::View* v : this->separators)
        {
            const brls::Rect at = v->getFrame();
            const float shut    = RAIL_SHUT - 2 * 16.0f - 2 * SEPARATOR_INSET;
            nvgBeginPath(vg);
            nvgRect(vg, at.getMinX(), std::round(at.getMidY()), shut + (at.getWidth() - shut) * t, 1);
            nvgFillColor(vg, line);
            nvgFill(vg);
        }
        nvgRestore(vg);
    }

    // Over the tab: a soft shadow along the rail's edge while it is out,
    // then the rail.
    void draw_over(brls::FrameContext* ctx)
    {
        if (this->mode != RailMode::AUTO)
            return;
        const float t = this->openness();
        if (t > 0.0f)
        {
            const brls::Rect at = this->getFrame();
            const float edge    = at.getMinX() + this->visible();
            NVGcolor dark       = ctx->theme["cadran/rail_shadow"];
            dark.a              = (float)(unsigned char)(std::round(dark.a * 255.0f) * t) / 255.0f;
            NVGcolor clear      = dark;
            clear.a             = 0.0f;
            nvgBeginPath(ctx->vg);
            nvgRect(ctx->vg, edge, at.getMinY(), SHADOW, at.getHeight());
            nvgFillPaint(ctx->vg, nvgLinearGradient(ctx->vg, edge, 0, edge + SHADOW, 0, dark, clear));
            nvgFill(ctx->vg);
        }
        this->layer_pass = true;
        this->frame(ctx);
        this->layer_pass = false;
    }

    // A touch reaches only the part of the rail that is out.
    brls::View* hitTest(brls::Point point) override
    {
        if (point.x > this->getX() + this->visible())
            return nullptr;
        return brls::Box::hitTest(point);
    }

    // Automatic, a touch on the tab while the rail is open over it: the
    // focus goes to the tab (as → would), so the rail folds, as a phone's
    // drawer closes when the page beside it is touched.
    void touched_tab(brls::Point point)
    {
        if (this->mode != RailMode::AUTO || !this->target || !this->tabs->getFrame().pointInside(point))
            return;
        // After the touch's own handling, on the frame's tab then.
        brls::sync([]()
            {
            if (RailFold* fold = RailFold::current()) fold->focus_tab(); });
    }

    void focus_tab()
    {
        const auto& kids = static_cast<brls::Box*>(this->tabs)->getChildren();
        brls::View* tab  = kids.empty() ? nullptr : kids.back();
        if (tab && tab != this->spacer && tab != this && this->target)
            brls::Application::giveFocus(tab);
    }

  private:
    // The rail's width on screen now.
    float visible() { return this->mode == RailMode::AUTO ? RAIL_SHUT + (RAIL_OPEN - RAIL_SHUT) * s_rail_open : this->getWidth(); }

    bool focus_on_rail() const
    {
        for (brls::View* v = brls::Application::getCurrentFocus(); v; v = v->hasParent() ? v->getParent() : nullptr)
            if (v == this->bar)
                return true;
        return brls::Application::getCurrentFocus() == nullptr;
    }

    // This frame's state: the openness the pill and the badges are drawn
    // by, the names' alpha, and no focus frame while the rail moves (it
    // would be drawn at the item's full width, out of the rail).
    void settle()
    {
        const float t     = this->openness();
        s_rail_open       = t;
        const bool moving = this->since != 0;
        for (brls::SidebarItem* item : this->items)
        {
            if (this->mode == RailMode::AUTO)
            {
                if (auto* label = item->getView("brls/sidebar/item_label"))
                    label->setAlpha(t * t);
                item->setHideHighlight(moving || t < 1.0f);
            }
            if (TabBadge* badge = TabBadge::find(item))
                badge->dock(t, this->mode == RailMode::ICONS);
        }
    }

    static constexpr float FOLD_US         = 180000.0f;
    static constexpr float SEPARATOR_INSET = 14.0f; // style_rail's margins
    static constexpr float SHADOW          = 28.0f;
    static RailFold* s_fold;
    brls::View* tabs;
    brls::Sidebar* bar;
    brls::Box* column;
    brls::Box* spacer = nullptr;
    std::vector<brls::SidebarItem*> items;
    std::vector<brls::View*> separators;
    RailMode mode    = RailMode::OPEN;
    bool target      = true;
    float from       = 1.0f;
    brls::Time since = 0;
    bool layer_pass  = false;
};
RailFold* RailFold::s_fold = nullptr;

// The last child of the main screen's frame: draws the rail over the tab
// (RailFold::draw_over) and gives it the touches there first.
class RailLayer : public brls::View
{
  public:
    explicit RailLayer(RailFold* rail)
        : fold(rail)
    {
        this->setFocusable(false);
        this->setPositionType(brls::PositionType::ABSOLUTE);
        this->setWidth(0);
        this->setHeight(0);
        this->setCulled(false);
    }

    void draw(NVGcontext*, float, float, float, float, brls::Style, brls::FrameContext* ctx) override
    {
        if (this->fold)
            this->fold->draw_over(ctx);
    }

    brls::View* hitTest(brls::Point point) override
    {
        if (!this->fold || rail_mode() != RailMode::AUTO)
            return nullptr;
        if (brls::View* rail = this->fold->hitTest(point))
            return rail;
        this->fold->touched_tab(point);
        return nullptr;
    }

  private:
    RailFold* fold; // in the same frame: they go together
};
} // namespace

void cadran::apply_rail()
{
    if (RailFold* fold = RailFold::current())
        fold->apply();
    // The tab wider or narrower: its lines move, the focused one shown again
    // (now, and once the focus is back from a picker that just closed).
    ScrollView::reveal_focused();
    brls::sync([]()
        { ScrollView::reveal_focused(); });
}

void cadran::style_rail(brls::View* tab_frame)
{
    auto* sidebar            = tab_frame ? dynamic_cast<brls::Sidebar*>(tab_frame->getView("brls/tab_frame/sidebar")) : nullptr;
    brls::SidebarItem* first = sidebar ? sidebar->getItem(0) : nullptr;
    brls::Box* list          = first ? first->getParent() : nullptr;
    if (!list)
        return;
    list->setPadding(6, 8, 10, 16);
    // The app's, in main.xml's order ("" for a separator).
    const auto& rail_icons               = ui::hooks::get().rail_icons;
    const std::vector<std::string> icons = rail_icons ? rail_icons() : std::vector<std::string> {};
    std::vector<brls::SidebarItem*> items;
    size_t position = 0;
    for (brls::View* v : list->getChildren())
    {
        const std::string name = position < icons.size() ? icons[position] : "";
        position++;
        auto* item = dynamic_cast<brls::SidebarItem*>(v);
        if (!item)
        { // a separator: its line is drawn at mid-height
            // 13 px: up to ten items and three separators fit between the
            // header and the footer (559 of 563 px), so the rail never
            // scrolls, nor moves from one tab to the next.
            v->setHeight(13);
            v->setMarginLeft(14);
            v->setMarginRight(14);
            continue;
        }
        item->setHeight(52);
        item->setCornerRadius(14);
        item->setPaddingLeft(RAIL_ITEM_PAD);
        item->setPaddingRight(RAIL_ITEM_PAD);
        item->setMarginBottom(4);
        item->setAlignItems(brls::AlignItems::CENTER);
        // The library shows its accent again on each change of tab: without
        // a size or a colour, that moves nothing (the pill says it instead).
        if (auto* accent = dynamic_cast<brls::Rectangle*>(item->getView("brls/sidebar/item_accent")))
        {
            accent->setWidth(0);
            accent->setMargins(0, 0, 0, 0);
            accent->setThemeColor("color", "cadran/none");
            accent->setVisibility(brls::Visibility::GONE);
        }
        if (auto* label = dynamic_cast<brls::Label*>(item->getView("brls/sidebar/item_label")))
        {
            chrome(label, 17, RAIL_SCALE);
            label->setMarginTop(0);
            label->setMarginBottom(0);
            label->setMarginRight(0);
            label->setMarginLeft(name.empty() ? 4 : 10);
            label->setSingleLine(true);
            // A long tab name down to 80 % of its size rather than cut and
            // scrolling
            label->setShrinkToFitMin(17 * 0.8f);
        }
        // No icon from the app (ui::hooks' rail_icons): the label alone.
        if (!name.empty())
            item->addView(kit::icon(name.c_str(), RAIL_ICON, "cadran/text2"), 0);
        items.push_back(item);
    }
    // The active tab's pill, behind the items (their background stays none).
    if (items.empty())
        return;
    auto* pill = new RailPill(items);
    items[0]->addView(pill, 0);
    for (brls::SidebarItem* item : items)
    {
        item->setThemeColor("backgroundColor", "cadran/none");
        item->getActiveEvent()->subscribe([items, item, pill](brls::View*)
            {
            // In this frame, not the next (the frame would be drawn in the
            // library's colours first, the text flashing at each move): the
            // tab left is made inactive here, before the library recolours it
            // after this event, which then leaves it as it is.
            for (brls::SidebarItem* it : items)
                if (it != item) it->setActive(false);
            pill->activate(item); });
    }
    pill->activate(items[0]); // the first tab is already active

    // Folded on its icons, or over the tab (Preferences › Sidebar).
    auto* fold = new RailFold(tab_frame, sidebar, list, items);
    fold->install();
    static bool listening = false;
    if (!listening)
    {
        listening = true;
        brls::Application::getGlobalFocusChangeEvent()->subscribe([](brls::View* focus)
            {
            if (RailFold* f = RailFold::current()) f->focus_changed(focus); });
    }
}

bool cadran::is_main_frame(brls::View* frame)
{
    return frame && (frame->getView("main_tabs") || frame->getView("brls/tab_frame/sidebar"));
}

bool cadran::is_standalone_screen(brls::View* frame)
{
    if (!frame)
        return false;
    const auto& standalone = ui::hooks::get().standalone_screen;
    if (standalone)
        return standalone(frame);
    // "child_root": the id PlayGuard gave its child screen before the hook,
    // kept until it sets the hook.
    return frame->getView("cadran/standalone") || frame->getView("child_root");
}

void cadran::style_frame(brls::AppletFrame* frame, brls::View* tab_frame)
{
    if (frame)
        frame->addView(make_glow(300.0f, -80.0f, 640.0f, "cadran/glow"), 0);
    style_header(frame, "check", "", true, true);
    style_rail(tab_frame);
    refresh_pill(); // hidden until a screen says something
    // Last: over everything of the frame.
    if (frame && RailFold::current())
        frame->addView(new RailLayer(RailFold::current()));
}
