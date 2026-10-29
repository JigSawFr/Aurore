// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "view/input_views.hpp"

#include <aurore/cadran/ui/cadran_kit.hpp>

#include <algorithm>
#include <cmath>

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
const aurore::ControllerButton TESTED[] = { aurore::BUTTON_A, aurore::BUTTON_B, aurore::BUTTON_X, aurore::BUTTON_Y,
    aurore::BUTTON_LB, aurore::BUTTON_RB, aurore::BUTTON_LT, aurore::BUTTON_RT, aurore::BUTTON_START,
    aurore::BUTTON_BACK, aurore::BUTTON_LSB, aurore::BUTTON_RSB };

// A button of the drawing: a shape lit while pressed, its name on it
void button(NVGcontext* vg, aurore::FrameContext* ctx, bool pressed, const char* name, float x, float y, float w,
    float h, float radius)
{
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, radius);
    nvgFillColor(vg, ctx->theme[pressed ? "cadran/teal_fill" : "cadran/surf"]);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, ctx->theme[pressed ? "cadran/teal_fill" : "cadran/border"]);
    nvgStroke(vg);
    if (name && *name)
    {
        nvgFontFaceId(vg, aurore::Application::getFont(aurore::FONT_REGULAR));
        nvgFontSize(vg, std::min(h * 0.55f, 16.0f));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, ctx->theme[pressed ? "cadran/navy" : "cadran/text2"]);
        nvgText(vg, x + w / 2, y + h / 2 + 1, name, nullptr);
    }
}

// A stick: its well, and its cap leaning as it is pushed
void stick(NVGcontext* vg, aurore::FrameContext* ctx, float cx, float cy, float radius, float ax, float ay, bool pressed)
{
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, radius);
    nvgFillColor(vg, ctx->theme["cadran/well"]);
    nvgFill(vg);
    float lean = radius * 0.42f;
    nvgBeginPath(vg);
    nvgCircle(vg, cx + ax * lean, cy + ay * lean, radius * 0.66f);
    bool moved = std::fabs(ax) > 0.15f || std::fabs(ay) > 0.15f;
    nvgFillColor(vg, ctx->theme[pressed ? "cadran/teal_fill" : moved ? "cadran/teal"
                                                                     : "cadran/surf"]);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, ctx->theme["cadran/border"]);
    nvgStroke(vg);
}
} // namespace

ControllerView::ControllerView()
    : aurore::Box(aurore::Axis::COLUMN)
{
    this->setFocusable(true);
    this->setHideHighlightBackground(true);
    this->setHighlightCornerRadius(kit::RADIUS_CARD);
    this->setHeight(340);
    this->setJustifyContent(aurore::JustifyContent::FLEX_END);
    this->setAlignItems(aurore::AlignItems::CENTER);
    this->setPadding(0, 20, 18, 20);
    this->setAccessibilityLabel("demo/input/controller"_i18n);
    this->status = kit::label("demo/input/test_start"_i18n, kit::TEXT_NOTE, "cadran/text2");
    this->status->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
    this->addView(this->status);

    // A starts the test; while it runs, every button is only shown here
    for (aurore::ControllerButton b : TESTED)
        this->registerAction("", b, [this, b](aurore::View*)
            {
            if (b == aurore::BUTTON_A && !this->testing)
            {
                this->setTesting(true);
                return true;
            }
            return this->testing; }, b != aurore::BUTTON_A);
    this->updateActionHint(aurore::BUTTON_A, "demo/input/test"_i18n);
}

void ControllerView::setTesting(bool testing)
{
    this->testing    = testing;
    this->bHeldSince = 0;
    this->setHideHighlight(testing);
    this->status->setText(testing ? "demo/input/test_stop"_i18n : "demo/input/test_start"_i18n);
    aurore::Application::requestRedraw();
}

aurore::View* ControllerView::getNextFocus(aurore::FocusDirection direction, aurore::View* currentView)
{
    // Testing, the D-pad is a button like the others: the focus stays
    if (this->testing)
        return this;
    return aurore::Box::getNextFocus(direction, currentView);
}

void ControllerView::onFocusLost()
{
    aurore::Box::onFocusLost();
    if (this->testing)
        this->setTesting(false);
}

void ControllerView::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    const aurore::ControllerState& state = aurore::Application::getControllerState();
    auto on                              = [&state](aurore::ControllerButton b)
    { return state.buttons[b]; };

    // The card
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, kit::RADIUS_CARD);
    nvgFillColor(vg, ctx->theme["cadran/surf"]);
    nvgFill(vg);
    nvgStrokeWidth(vg, this->testing ? 2.0f : 1.0f);
    nvgStrokeColor(vg, ctx->theme[this->testing ? "cadran/teal" : "cadran/line"]);
    nvgStroke(vg);

    // The controller: shoulders, a body, its buttons
    float bw = std::min(width - 80, 540.0f), bh = 190;
    float bx = x + (width - bw) / 2, by = y + 72;
    float s = bw / 540.0f;

    button(vg, ctx, on(aurore::BUTTON_LT), "ZL", bx + 40 * s, by - 52, 96 * s, 22, 11);
    button(vg, ctx, on(aurore::BUTTON_RT), "ZR", bx + bw - 136 * s, by - 52, 96 * s, 22, 11);
    button(vg, ctx, on(aurore::BUTTON_LB), "L", bx + 30 * s, by - 24, 120 * s, 20, 10);
    button(vg, ctx, on(aurore::BUTTON_RB), "R", bx + bw - 150 * s, by - 24, 120 * s, 20, 10);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, bx, by, bw, bh, bh / 2);
    nvgFillColor(vg, ctx->theme["cadran/surf2"]);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, ctx->theme["cadran/border"]);
    nvgStroke(vg);

    stick(vg, ctx, bx + bw * 0.2f, by + bh * 0.36f, 32, state.axes[aurore::LEFT_X], state.axes[aurore::LEFT_Y],
        on(aurore::BUTTON_LSB));
    stick(vg, ctx, bx + bw * 0.66f, by + bh * 0.70f, 32, state.axes[aurore::RIGHT_X], state.axes[aurore::RIGHT_Y],
        on(aurore::BUTTON_RSB));

    // The D-pad, and A, B, X, Y as a console lays them out
    float dx = bx + bw * 0.34f, dy = by + bh * 0.70f, d = 26;
    button(vg, ctx, on(aurore::BUTTON_UP), "", dx - d / 2, dy - d * 1.5f, d, d, 6);
    button(vg, ctx, on(aurore::BUTTON_DOWN), "", dx - d / 2, dy + d * 0.5f, d, d, 6);
    button(vg, ctx, on(aurore::BUTTON_LEFT), "", dx - d * 1.5f, dy - d / 2, d, d, 6);
    button(vg, ctx, on(aurore::BUTTON_RIGHT), "", dx + d * 0.5f, dy - d / 2, d, d, 6);
    float fx = bx + bw * 0.8f, fy = by + bh * 0.36f, f = 34, gap = 30;
    button(vg, ctx, on(aurore::BUTTON_X), "X", fx - f / 2, fy - gap - f / 2, f, f, f / 2);
    button(vg, ctx, on(aurore::BUTTON_B), "B", fx - f / 2, fy + gap - f / 2, f, f, f / 2);
    button(vg, ctx, on(aurore::BUTTON_Y), "Y", fx - gap - f / 2, fy - f / 2, f, f, f / 2);
    button(vg, ctx, on(aurore::BUTTON_A), "A", fx + gap - f / 2, fy - f / 2, f, f, f / 2);
    button(vg, ctx, on(aurore::BUTTON_BACK), "−", bx + bw * 0.40f - 16, by + bh * 0.24f - 11, 32, 22, 11);
    button(vg, ctx, on(aurore::BUTTON_START), "+", bx + bw * 0.60f - 16, by + bh * 0.24f - 11, 32, 22, 11);

    // B held a second ends the test
    if (this->testing)
    {
        if (!on(aurore::BUTTON_B))
            this->bHeldSince = 0;
        else if (this->bHeldSince == 0)
            this->bHeldSince = aurore::getCPUTimeUsec();
        else if (aurore::getCPUTimeUsec() - this->bHeldSince > 1000000)
            aurore::sync([this]
                { this->setTesting(false); });
    }
    // Every frame while it has the focus: a stick moved asks for none
    if (aurore::Application::getCurrentFocus() == this)
        aurore::Application::requestRedraw();

    aurore::Box::draw(vg, x, y, width, height, style, ctx);
}

// Touch

TouchPad::TouchPad()
    : aurore::Box(aurore::Axis::COLUMN)
{
    this->setHeight(220);
    this->setJustifyContent(aurore::JustifyContent::CENTER);
    this->setAlignItems(aurore::AlignItems::CENTER);
    aurore::Label* hint = kit::label("demo/input/touch_here"_i18n, kit::TEXT_NOTE, "cadran/text3");
    hint->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
    this->addView(hint);
    this->addGestureRecognizer(new aurore::TapGestureRecognizer([this](aurore::TapGestureStatus status, aurore::Sound*)
        {
        if (status.state == aurore::GestureState::END)
            this->add(status.position, true); }));
    this->addGestureRecognizer(new aurore::PanGestureRecognizer([this](aurore::PanGestureStatus status, aurore::Sound*)
        {
        if (status.state == aurore::GestureState::STAY || status.state == aurore::GestureState::START)
            this->add(status.position, false); },
        aurore::PanAxis::ANY));
}

void TouchPad::add(aurore::Point point, bool ripple)
{
    float w = this->getWidth(), h = this->getHeight();
    if (w <= 0 || h <= 0)
        return;
    if (this->marks.size() > 160)
        this->marks.pop_front();
    this->marks.push_back({ (point.x - this->getX()) / w, (point.y - this->getY()) / h, aurore::getCPUTimeUsec(), ripple });
    aurore::Application::requestRedraw();
}

void TouchPad::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, kit::RADIUS_CARD);
    nvgFillColor(vg, ctx->theme["cadran/surf"]);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1);
    nvgStrokeColor(vg, ctx->theme["cadran/line"]);
    nvgStroke(vg);

    // A grid of dots, as on a pad
    nvgBeginPath(vg);
    for (float gx = x + 24; gx < x + width - 12; gx += 32)
        for (float gy = y + 22; gy < y + height - 12; gy += 32)
            nvgCircle(vg, gx, gy, 1.2f);
    nvgFillColor(vg, ctx->theme["cadran/line"]);
    nvgFill(vg);

    aurore::Time now = aurore::getCPUTimeUsec();
    while (!this->marks.empty() && now - this->marks.front().at > 900000)
        this->marks.pop_front();
    NVGcolor teal = ctx->theme["cadran/teal"];
    for (const Mark& mark : this->marks)
    {
        float age = (float)(now - mark.at) / 900000.0f;
        float mx = x + mark.x * width, my = y + mark.y * height;
        nvgBeginPath(vg);
        if (mark.ripple)
        {
            nvgCircle(vg, mx, my, 10 + 40 * age);
            nvgStrokeWidth(vg, 3);
            nvgStrokeColor(vg, nvgRGBAf(teal.r, teal.g, teal.b, 1 - age));
            nvgStroke(vg);
        }
        else
        {
            nvgCircle(vg, mx, my, 7 * (1 - age) + 2);
            nvgFillColor(vg, nvgRGBAf(teal.r, teal.g, teal.b, (1 - age) * 0.8f));
            nvgFill(vg);
        }
    }
    if (!this->marks.empty())
        aurore::Application::requestRedraw();

    aurore::Box::draw(vg, x, y, width, height, style, ctx);
}
