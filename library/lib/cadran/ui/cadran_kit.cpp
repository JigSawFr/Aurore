// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/core/time.hpp>

#include <aurore/cadran/ui/hooks.hpp>

#include <fmt/format.h>
#include <yoga/Yoga.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <tuple>

#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/effects.hpp>
#include <aurore/cadran/ui/motion.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/view/cadran_digits.hpp>
#include <aurore/cadran/view/qr_view.hpp>

namespace cadran::kit
{

NVGcolor color(const char* key)
{
    return aurore::Application::getTheme()[key];
}

aurore::Label* label(const std::string& text, float size, const char* color_key)
{
    auto* l = new aurore::Label();
    l->setText(text);
    l->setFontSize(size);
    l->setThemeColor("textColor", color_key);
    return l;
}

namespace
{
    // A label in the display face: Sora, the other scripts from the app's fonts.
    class DisplayLabel : public aurore::Label
    {
      public:
        DisplayLabel()
        {
            const int face = aurore::Application::getFont("cadran/digits");
            if (face != aurore::FONT_INVALID)
                this->font = face;
        }
    };
} // namespace

aurore::Label* display(const std::string& text, float size, const char* color_key)
{
    auto* l = new DisplayLabel();
    l->setText(text);
    l->setFontSize(size);
    l->setThemeColor("textColor", color_key);
    return l;
}

CadranIcon* icon(const char* name, float size, const char* color_key)
{
    auto* view = new CadranIcon();
    view->setIcon(name);
    view->setWidth(aurore::dp(size));
    view->setHeight(aurore::dp(size));
    view->setThemeColor("color", color_key);
    return view;
}

aurore::Box* card(bool focusable, bool grow)
{
    auto* box = new aurore::Box(aurore::Axis::COLUMN);
    box->setAlignItems(aurore::AlignItems::STRETCH);
    box->setThemeColor("backgroundColor", "cadran/surf");
    box->setThemeColor("borderColor", "cadran/line");
    box->setBorderThickness(1);
    box->setCornerRadius(aurore::dp(RADIUS_CARD));
    box->setPadding(aurore::dp(20), aurore::dp(24), aurore::dp(20), aurore::dp(24));
    if (focusable)
    {
        box->setFocusable(true);
        box->setHideHighlightBackground(true);
        box->setHighlightCornerRadius(aurore::dp(RADIUS_CARD));
        box->setAccessibilityRole(aurore::AccessibilityRole::CELL);
    }
    if (grow)
    {
        box->setGrow(1.0f);
        box->setWidth(0);
    }
    return box;
}

aurore::Box* top_card(bool grow)
{
    aurore::Box* box = card(false, grow);
    box->setCornerRadius(aurore::dp(RADIUS_TOP));
    box->setHighlightCornerRadius(aurore::dp(RADIUS_TOP));
    box->setPadding(aurore::dp(22), aurore::dp(26), aurore::dp(22), aurore::dp(26));
    return box;
}

void set_kicker(aurore::Label* label, const std::string& text)
{
    if (!label)
        return;
    label->setFontSize(TEXT_KICKER);
    label->setThemeColor("textColor", "cadran/text2");
    if (!text.empty())
        label->setText(cadran::upper(text));
}

aurore::Box* kicker(const char* icon_name, const std::string& title)
{
    auto* head = new aurore::Box(aurore::Axis::ROW);
    head->setAlignItems(aurore::AlignItems::CENTER);
    head->addView(icon(icon_name, 18, "cadran/text2"));
    auto* text = new aurore::Label();
    set_kicker(text, title);
    text->setSingleLine(true);
    text->setMarginLeft(aurore::dp(8));
    head->addView(text);
    return head;
}

Mark::Mark(const char* color_key)
{
    this->setFocusable(false);
    this->setWidth(12);
    this->setHeight(12);
    this->set(color_key);
}

void Mark::set(const char* color_key)
{
    const std::string key = color_key;
    this->key             = key;
    this->shape           = key.rfind("cadran/teal", 0) == 0                               ? Shape::CHECK
                  : key.rfind("cadran/amber", 0) == 0 || key.rfind("cadran/coral", 0) == 0 ? Shape::WARN
                                                                                           : Shape::DOT;
}

void Mark::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style, aurore::FrameContext* ctx)
{
    const NVGcolor color = ctx->theme[this->key];
    const float cx = x + width / 2, cy = y + height / 2;
    nvgBeginPath(vg);
    switch (this->shape)
    {
        case Shape::DOT:
            nvgCircle(vg, cx, cy, 4);
            nvgFillColor(vg, this->a(color));
            nvgFill(vg);
            break;
        case Shape::CHECK:
            nvgMoveTo(vg, x + 1.5f, cy + 0.5f);
            nvgLineTo(vg, x + 4.8f, cy + 3.6f);
            nvgLineTo(vg, x + width - 1.2f, y + 2.6f);
            nvgStrokeColor(vg, this->a(color));
            nvgStrokeWidth(vg, 2.2f);
            nvgLineCap(vg, NVG_ROUND);
            nvgLineJoin(vg, NVG_ROUND);
            nvgStroke(vg);
            break;
        case Shape::WARN:
        {
            // A rounded triangle, an exclamation mark cut out of it.
            nvgMoveTo(vg, cx, y + 0.5f);
            nvgLineTo(vg, x + width, y + height - 0.5f);
            nvgLineTo(vg, x, y + height - 0.5f);
            nvgClosePath(vg);
            nvgFillColor(vg, this->a(color));
            nvgFill(vg);
            const NVGcolor cut = this->a(ctx->theme["aurore/background"]);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, cx - 0.8f, y + 4.2f, 1.6f, 3.8f, 0.8f);
            nvgCircle(vg, cx, y + height - 2.6f, 0.95f);
            nvgFillColor(vg, cut);
            nvgFill(vg);
            break;
        }
    }
}

aurore::Box* pill(const std::string& text, const char* dot_key)
{
    auto* box = new aurore::Box(aurore::Axis::ROW);
    box->setAlignItems(aurore::AlignItems::CENTER);
    box->setHeight(aurore::dp(30));
    box->setPaddingLeft(aurore::dp(14));
    box->setPaddingRight(aurore::dp(16));
    box->setCornerRadius(aurore::dp(15));
    box->setThemeColor("borderColor", "cadran/line");
    box->setBorderThickness(1);
    box->setMarginRight(aurore::dp(10));
    box->setMarginBottom(aurore::dp(8));
    box->setHighlightCornerRadius(aurore::dp(15));
    auto* mark = new Mark(dot_key);
    mark->setMarginRight(aurore::dp(7));
    box->addView(mark);
    aurore::Label* l = label(text, TEXT_META, "cadran/text2");
    l->setSingleLine(true);
    box->addView(l);
    return box;
}

aurore::Box* key(const char* letter, float size)
{
    auto* bubble = new aurore::Box(aurore::Axis::ROW);
    bubble->setWidth(aurore::dp(size));
    bubble->setHeight(aurore::dp(size));
    bubble->setCornerRadius(aurore::dp(size) / 2);
    bubble->setShrink(0);
    bubble->setThemeColor("backgroundColor", "cadran/key_bg");
    bubble->setAlignItems(aurore::AlignItems::CENTER);
    bubble->setJustifyContent(aurore::JustifyContent::CENTER);
    auto* text = new CadranDigits();
    text->setFontSize(12);
    text->setThemeColor("textColor", "cadran/key_fg");
    text->setText(letter);
    bubble->addView(text);
    return bubble;
}

Chip::Chip()
    : aurore::Box(aurore::Axis::ROW)
{
}

void Chip::frame(aurore::FrameContext* ctx)
{
    // The press's flash (Aurore's click animation, 0 → 1 → 0) as a dip of
    // the whole chip, background and words, about its middle.
    const float press = effects::motion() ? this->getClickAlpha() : 0.0f;
    if (press <= 0.0f)
    {
        aurore::Box::frame(ctx);
        return;
    }
    const aurore::Rect at = this->getFrame();
    const float cx = at.getMidX(), cy = at.getMidY();
    const float scale = 1.0f - 0.05f * press;
    nvgSave(ctx->vg);
    nvgTranslate(ctx->vg, cx, cy);
    nvgScale(ctx->vg, scale, scale);
    nvgTranslate(ctx->vg, -cx, -cy);
    aurore::Box::frame(ctx);
    nvgRestore(ctx->vg);
}

aurore::Box* chip(const std::string& text, float height)
{
    auto* box = new Chip();
    box->setAlignItems(aurore::AlignItems::CENTER);
    box->setHeight(aurore::dp(height));
    box->setPaddingLeft(aurore::dp(22));
    box->setPaddingRight(aurore::dp(22));
    box->setCornerRadius(aurore::dp(height) / 2);
    box->setThemeColor("backgroundColor", "cadran/surf2");
    box->setThemeColor("borderColor", "cadran/line");
    box->setBorderThickness(1);
    box->setFocusable(true);
    box->setHideHighlightBackground(true);
    box->setHighlightCornerRadius(aurore::dp(height) / 2);
    auto* digits = new CadranDigits();
    digits->setText(text);
    digits->setFontSize(17);
    digits->setFollowsTextSize(true);
    box->addView(digits);
    return box;
}

aurore::Box* qr(const std::string& payload, float side, const std::string& title, const std::string& caption, float width)
{
    auto* box = new aurore::Box(aurore::Axis::COLUMN);
    box->setAlignItems(aurore::AlignItems::CENTER);
    if (width > 0)
        box->setWidth(aurore::dp(width));
    box->addView(new QrView(payload, side));
    for (const auto& [text, size, key] : { std::tuple<const std::string&, float, const char*> { title, 18, "cadran/text" },
             { caption, TEXT_META, "cadran/text3" } })
    {
        if (text.empty())
            continue;
        aurore::Label* l = label(text, size, key);
        l->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
        l->setAlignSelf(aurore::AlignSelf::STRETCH); // wraps at the box's width
        l->setMarginTop(aurore::dp(&text == &title ? 10 : 2));
        box->addView(l);
    }
    return box;
}

Bar::Bar()
{
    this->setHeight(aurore::dp(12));
    this->setFocusable(false);
}

void Bar::set(float value)
{
    value = value < 0 ? 0 : value > 1 ? 1
                                      : value;
    if (value == this->fraction)
        return;
    // From where the fill is drawn now, over a third of a second.
    this->from     = this->drawn();
    this->since    = effects::motion() ? motion::now() : 0;
    this->fraction = value;
}

float Bar::drawn() const
{
    const float t = motion::ease_out(motion::progress(this->since, 350000.0f));
    return this->from + (this->fraction - this->from) * t;
}

void Bar::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style, aurore::FrameContext*)
{
    aurore::Theme theme = aurore::Application::getTheme();
    const float radius  = height / 2.0f;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, radius);
    nvgFillColor(vg, this->a(theme["cadran/well"]));
    nvgFill(vg);
    const float fill = this->drawn();
    if (fill <= 0)
        return;
    const float w = std::max(width * fill, height);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, height, radius);
    nvgFillColor(vg, this->a(theme["cadran/teal_fill"]));
    nvgFill(vg);
}

Empty empty(const char* icon_name)
{
    Empty e;
    e.box = card(false, false);
    e.box->setAlignItems(aurore::AlignItems::CENTER);
    e.box->setPadding(aurore::dp(32), aurore::dp(40), aurore::dp(32), aurore::dp(40));
    auto* circle = new aurore::Box(aurore::Axis::ROW);
    circle->setWidth(aurore::dp(64));
    circle->setHeight(aurore::dp(64));
    circle->setCornerRadius(aurore::dp(32));
    circle->setThemeColor("backgroundColor", "cadran/teal_soft");
    circle->setAlignItems(aurore::AlignItems::CENTER);
    circle->setJustifyContent(aurore::JustifyContent::CENTER);
    circle->addView(icon(icon_name, 28, "cadran/teal"));
    e.box->addView(circle);
    e.title = label("", TEXT_LEAD, "cadran/text");
    e.title->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
    e.title->setAlignSelf(aurore::AlignSelf::STRETCH); // wraps at the card's width
    e.title->setMarginTop(aurore::dp(16));
    e.box->addView(e.title);
    e.text = label("", TEXT_META, "cadran/text2");
    e.text->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
    e.text->setAlignSelf(aurore::AlignSelf::STRETCH);
    e.text->setMarginTop(aurore::dp(6));
    e.box->addView(e.text);
    return e;
}

aurore::Box* empty_around(aurore::Label* label, const char* icon_name)
{
    auto* parent = label ? dynamic_cast<aurore::Box*>(label->getParent()) : nullptr;
    if (!parent)
        return nullptr;
    Empty e = empty(icon_name);
    e.box->setId("cadran_empty_card");
    const YGValue top = YGNodeStyleGetMargin(label->getYGNode(), YGEdgeTop);
    e.box->setMargins(top.unit == YGUnitPoint && top.value > aurore::dp(10) ? top.value : aurore::dp(10), 0, aurore::dp(16), 0);
    const auto& children = parent->getChildren();
    const size_t at      = (size_t)(std::find(children.begin(), children.end(), label) - children.begin());
    // The label in the title's place, laid out as it.
    parent->removeView(label, false);
    e.box->removeView(e.title);
    label->setFontSize(TEXT_LEAD);
    label->setThemeColor("textColor", "cadran/text");
    label->setSingleLine(false);
    label->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
    label->setAlignSelf(aurore::AlignSelf::STRETCH);
    label->setMargins(aurore::dp(16), 0, 0, 0);
    e.box->addView(label, 1);
    e.box->removeView(e.text); // the screen's label says it all
    parent->addView(e.box, at);
    e.box->setVisibility(label->getVisibility());
    label->setVisibility(aurore::Visibility::VISIBLE);
    return e.box;
}

void wrap(aurore::View* row)
{
    if (row)
        YGNodeStyleSetFlexWrap(row->getYGNode(), YGWrapWrap);
}

void fit_button(aurore::Button* button)
{
    auto* label = button ? dynamic_cast<aurore::Label*>(button->getView("aurore/button/label")) : nullptr;
    if (!label)
        return;
    label->setSingleLine(true);
    label->setShrinkToFit(true);
    // Its width at the normal size: the text's at the size asked, its
    // padding, and the views beside it (an icon), whose sizes are fixed.
    NVGcontext* vg = aurore::Application::getNVGContext();
    nvgFontSize(vg, label->getFontSize());
    nvgFontFaceId(vg, label->getFont());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    float bounds[4];
    nvgTextBounds(vg, 0, 0, label->getFullText().c_str(), nullptr, bounds);
    YGNodeRef node    = button->getYGNode();
    const auto points = [](YGValue v)
    { return v.unit == YGUnitPoint ? v.value : 0.0f; };
    float width = bounds[2] - bounds[0] + points(YGNodeStyleGetPadding(node, YGEdgeLeft))
        + points(YGNodeStyleGetPadding(node, YGEdgeRight)) + 1.0f;
    for (aurore::View* child : button->getChildren())
    {
        if (child == label || child->getVisibility() == aurore::Visibility::GONE)
            continue;
        YGNodeRef c = child->getYGNode();
        width += points(YGNodeStyleGetWidth(c)) + points(YGNodeStyleGetMargin(c, YGEdgeLeft))
            + points(YGNodeStyleGetMargin(c, YGEdgeRight));
    }
    YGNodeStyleSetFlexBasis(node, width);
    YGNodeStyleSetFlexShrink(node, 0);
    button->invalidate();
}

void fit_head(aurore::Box* row)
{
    if (!row || row->getChildren().size() < 2)
        return;
    auto* title = dynamic_cast<aurore::Label*>(row->getChildren()[0]);
    auto* state = dynamic_cast<aurore::Label*>(row->getChildren()[1]);
    if (!title || !state)
        return;
    wrap(row);
    YGNodeStyleSetGap(row->getYGNode(), YGGutterColumn, aurore::dp(8));
    YGNodeStyleSetGap(row->getYGNode(), YGGutterRow, aurore::dp(2));
    title->setShrink(0.0f);
    state->setSingleLine(true);
    state->setShrink(0.0f);
}

void draw_more_below(NVGcontext* vg, aurore::ScrollingFrame* frame, float x, float y, float width, float height,
    const char* color_key, float corner)
{
    // The content: the frame's tallest child (the other is its scroll bar).
    aurore::View* content = nullptr;
    if (frame)
        for (aurore::View* child : frame->getChildren())
            if (!content || child->getHeight() > content->getHeight())
                content = child;
    if (!content)
        return;
    const float below = content->getHeight() - frame->getContentOffsetY() - height;
    if (below <= 1.0f)
        return;
    const float FADE = aurore::dp(32.0f);
    NVGcolor solid   = aurore::Application::getTheme()[color_key];
    solid.a *= frame->getAlpha();
    NVGcolor clear  = solid;
    clear.a         = 0.0f;
    const float top = y + height - FADE;
    nvgBeginPath(vg);
    corner = aurore::dp(corner);
    if (corner > 0.0f)
        nvgRoundedRectVarying(vg, x + 1, top, width - 2, FADE - 1, 0, 0, corner - 1, corner - 1);
    else
        nvgRect(vg, x, top, width, FADE);
    nvgFillPaint(vg, nvgLinearGradient(vg, x, top, x, y + height, clear, solid));
    nvgFill(vg);
}

std::string without_chevron(const std::string& value)
{
    static const std::string chevron = "›";
    if (value.size() < chevron.size() || value.compare(value.size() - chevron.size(), chevron.size(), chevron) != 0)
        return value;
    std::string out = value.substr(0, value.size() - chevron.size());
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

const char* dot_of(NVGcolor c)
{
    const auto same = [&c](NVGcolor o)
    { return c.r == o.r && c.g == o.g && c.b == o.b; };
    if (same(cadran::kit::color("cadran/teal")))
        return "cadran/teal_fill";
    if (same(cadran::kit::color("cadran/amber")))
        return "cadran/amber_fill";
    if (same(cadran::kit::color("cadran/coral")))
        return "cadran/coral_fill";
    return "cadran/text3";
}

namespace
{
    // {0} the day's name, {1} the same in lower case, {2} the day, {3} the month.
    std::string date_text(int weekday, int day, int month)
    {
        return aurore::getStr("cadran/date", aurore::getStr(fmt::format("cadran/days/{}", weekday)),
            aurore::getStr(fmt::format("cadran/days_lower/{}", weekday)), day,
            aurore::getStr(fmt::format("cadran/months/{}", month)));
    }
} // namespace

std::string long_date()
{
    // The app's clock when it has one of its own (the console's), else the
    // system's.
    if (ui::hooks::get().today)
        return long_date(ui::hooks::get().today());
    const std::time_t now = aurore::getWallTime();
    char ymd[16]          = "";
    const std::tm tm      = aurore::localTime(now);
    std::strftime(ymd, sizeof ymd, "%Y-%m-%d", &tm);
    return long_date(std::string(ymd));
}

std::string long_date(const std::string& ymd)
{
    if (ymd.size() != 10 || ymd[4] != '-' || ymd[7] != '-')
        return ymd;
    std::tm tm = {};
    tm.tm_year = std::atoi(ymd.substr(0, 4).c_str()) - 1900;
    tm.tm_mon  = std::atoi(ymd.substr(5, 2).c_str()) - 1;
    tm.tm_mday = std::atoi(ymd.substr(8, 2).c_str());
    tm.tm_hour = 12; // clear of any change of hour
    if (tm.tm_mon < 0 || tm.tm_mon > 11 || tm.tm_mday < 1 || std::mktime(&tm) == (std::time_t)-1)
        return ymd;
    return date_text(tm.tm_wday, tm.tm_mday, tm.tm_mon + 1);
}

} // namespace cadran::kit
