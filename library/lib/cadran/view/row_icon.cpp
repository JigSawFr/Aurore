// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/view/row_icon.hpp>

#include <aurore/cadran/ui/cadran_kit.hpp>

static constexpr float SIZE          = 26;
static constexpr const char* ICON_ID = "row_icon";

RowIcon::RowIcon(Kind shown)
    : kind(shown)
{
    this->setId(ICON_ID);
    this->setFocusable(false);
    this->setWidth(SIZE);
    this->setHeight(SIZE);
    this->setAlignSelf(aurore::AlignSelf::CENTER);
    this->setMarginLeft(12);
    this->setVisibility(aurore::Visibility::GONE);
}

void RowIcon::set_shown(bool shown)
{
    this->setVisibility(shown ? aurore::Visibility::VISIBLE : aurore::Visibility::GONE);
}

void RowIcon::set_kind(Kind shown)
{
    this->kind = shown;
}

void RowIcon::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    (void)style;
    (void)ctx;
    const float cx = x + width * 0.5f;
    float text_y;
    nvgBeginPath(vg);
    if (this->kind == Kind::Warning)
    {
        // Rounded corners: a stroke of the same colour along the filled triangle.
        const float inset = 2.5f;
        nvgMoveTo(vg, cx, y + inset);
        nvgLineTo(vg, x + width - inset, y + height - inset);
        nvgLineTo(vg, x + inset, y + height - inset);
        nvgClosePath(vg);
        nvgLineJoin(vg, NVG_ROUND);
        nvgStrokeWidth(vg, inset * 2);
        nvgStrokeColor(vg, cadran::kit::color("cadran/amber"));
        nvgStroke(vg);
        nvgFillColor(vg, cadran::kit::color("cadran/amber"));
        nvgFontSize(vg, height * 0.72f);
        text_y = y + height * 0.62f;
    }
    else
    {
        // In the value's colour: something to read, not something wrong.
        nvgCircle(vg, cx, y + height * 0.5f, height * 0.5f);
        nvgFillColor(vg, cadran::kit::color("cadran/text2"));
        nvgFontSize(vg, height * 0.7f);
        text_y = y + height * 0.53f;
    }
    nvgFill(vg);
    nvgFontFaceId(vg, aurore::Application::getDefaultFont());
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, aurore::Application::getTheme()["aurore/background"]);
    nvgText(vg, cx, text_y, this->kind == Kind::Warning ? "!" : "i", nullptr);
}

RowIcon* RowIcon::on_row(aurore::Box* row, Kind kind)
{
    if (!row)
        return nullptr;
    if (auto* icon = dynamic_cast<RowIcon*>(row->getView(ICON_ID)))
        return icon;
    auto* icon = new RowIcon(kind);
    // Right after the value (before a badge): a long value overflows its
    // box to the left, over anything put before it.
    row->addView(icon, 2);
    return icon;
}
