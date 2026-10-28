/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <algorithm>
#include <aurore/core/activity.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/touch/long_press_gesture.hpp>
#include <aurore/core/touch/tap_gesture.hpp>
#include <aurore/views/context_menu.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/rectangle.hpp>

using namespace aurore::literals;

namespace aurore
{

namespace
{
    constexpr float ITEM_HEIGHT  = 52;
    constexpr float ICON_SIZE    = 24;
    constexpr float MENU_WIDTH   = 340;
    constexpr float PANEL_MARGIN = 16; // from the screen's edges
    constexpr float ANCHOR_GAP   = 8;

    // A picture for a path, a Material Icons glyph otherwise
    View* makeIcon(const std::string& icon)
    {
        if (icon.empty())
            return nullptr;

        if (icon.rfind("@res/", 0) == 0 || icon.find('/') != std::string::npos || icon.find('.') != std::string::npos)
        {
            auto* image = new Image();
            image->setImageFromFile(icon);
            image->setScalingType(ImageScalingType::FIT);
            return image;
        }

        auto* glyph = new Label();
        glyph->setFont(Application::getFont(FONT_MATERIAL_ICONS));
        glyph->setFontSize(ICON_SIZE);
        glyph->setSingleLine(true);
        glyph->setHorizontalAlign(HorizontalAlign::CENTER);
        glyph->setVerticalAlign(VerticalAlign::CENTER);
        glyph->setText(icon);
        return glyph;
    }
}

// ContextMenuItem

ContextMenuItem::ContextMenuItem(const std::string& text, View* icon)
    : Box(Axis::ROW)
{
    this->setFocusable(true);
    this->setHeight(ITEM_HEIGHT);
    this->setAlignItems(AlignItems::CENTER);
    this->setPaddingLeft(14);
    this->setPaddingRight(14);
    this->setCornerRadius(10);
    this->setHighlightCornerRadius(10);

    if (icon)
    {
        icon->setDimensions(ICON_SIZE, ICON_SIZE);
        icon->setMarginRight(14);
        icon->setShrink(0);
        this->addView(icon);
    }

    this->label = new Label();
    this->label->setText(text);
    this->label->setFontSize(18);
    this->label->setSingleLine(true);
    this->label->setGrow(1);
    this->label->setShrink(1);
    this->addView(this->label);

    this->addGestureRecognizer(new TapGestureRecognizer(this));
}

// ContextMenu

ContextMenu::ContextMenu(View* anchor)
    : Box(Axis::COLUMN)
    , anchor(anchor)
{
    this->dimBackground();

    // Outside the panel: a touch or a click closes the menu
    this->addGestureRecognizer(new TapGestureRecognizer(this, [this]()
        { this->close(); }, TapGestureConfig(false, SOUND_NONE, SOUND_NONE, SOUND_BACK)));

    this->panel = new Box(Axis::COLUMN);
    this->panel->setPositionType(PositionType::ABSOLUTE);
    this->panel->setWidth(MENU_WIDTH);
    this->panel->setPadding(8);
    this->panel->setCornerRadius(16);
    this->panel->setThemeColor("backgroundColor", "brls/background");
    this->panel->setThemeColor("borderColor", "brls/sidebar/separator");
    this->panel->setBorderThickness(1);
    this->panel->setShadowType(ShadowType::GENERIC);
    this->panel->setShadowVisibility(true);
    // A touch on the panel, between its lines, is not a touch outside
    this->panel->addGestureRecognizer(new TapGestureRecognizer([](TapGestureStatus, Sound*) {}));
    this->addView(this->panel);

    this->registerAction("hints/back"_i18n, BUTTON_B, [this](View*)
        {
        this->close();
        return true; }, false, false, SOUND_BACK);
}

// A light dim: the screen it was opened from stays readable
void ContextMenu::dimBackground()
{
    NVGcolor backdrop = Application::getTheme()["brls/backdrop"];
    backdrop.a *= 0.4f;
    this->setBackgroundColor(backdrop);
}

void ContextMenu::onThemeChanged()
{
    this->dimBackground();
    Box::onThemeChanged();
}

void ContextMenu::setTitle(const std::string& text)
{
    if (!this->title)
    {
        this->title = new Label();
        this->title->setFontSize(15);
        this->title->setSingleLine(true);
        this->title->setThemeColor("textColor", "brls/header/subtitle");
        this->title->setMarginLeft(14);
        this->title->setMarginRight(14);
        this->title->setMarginTop(6);
        this->title->setMarginBottom(6);
        this->panel->addView(this->title, 0);
    }
    this->title->setText(text);
}

ContextMenuItem* ContextMenu::addItem(const std::string& label, std::function<void()> action, const std::string& icon)
{
    return this->addItem(label, action, makeIcon(icon));
}

ContextMenuItem* ContextMenu::addItem(const std::string& label, std::function<void()> action, View* icon)
{
    auto* item = new ContextMenuItem(label, icon);
    item->registerClickAction([this, action](View*)
        {
        this->close(action);
        return true; });
    this->panel->addView(item);
    return item;
}

void ContextMenu::addSeparator()
{
    auto* line = new Rectangle();
    line->setThemeColor("color", "brls/sidebar/separator");
    line->setHeight(1);
    line->setMarginTop(6);
    line->setMarginBottom(6);
    line->setMarginLeft(10);
    line->setMarginRight(10);
    this->panel->addView(line);
}

View* ContextMenu::getDefaultFocus()
{
    for (View* child : this->panel->getChildren())
        if (View* focus = child->getDefaultFocus())
            return focus;
    return nullptr;
}

void ContextMenu::open()
{
    // Where the anchor is now: the menu does not follow it afterwards
    if (this->anchor)
        this->anchorFrame = this->anchor->getFrame();
    this->anchor = nullptr;

    Application::pushActivity(new Activity(this), TransitionAnimation::FADE);
}

void ContextMenu::close(std::function<void()> then)
{
    if (this->closing)
        return;
    this->closing = true;

    Application::popActivity(TransitionAnimation::FADE, [then]()
        {
        if (then)
            then(); });
}

// Placed when drawn, once its size is known: not from onLayout(), which yoga
// calls in the middle of its layout
void ContextMenu::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    this->place();
    Box::draw(vg, x, y, width, height, style, ctx);
}

// Below the anchor, else above, else as low as fits; its left edge on the
// anchor's, kept on screen
void ContextMenu::place()
{
    float screenWidth  = this->getWidth();
    float screenHeight = this->getHeight();
    float width        = this->panel->getWidth();
    float height       = this->panel->getHeight();
    if (screenWidth <= 0 || height <= 0)
        return;

    float top = this->anchorFrame.getMaxY() + ANCHOR_GAP;
    if (top + height > screenHeight - PANEL_MARGIN)
    {
        top = this->anchorFrame.getMinY() - ANCHOR_GAP - height;
        if (top < PANEL_MARGIN)
            top = std::max(PANEL_MARGIN, screenHeight - PANEL_MARGIN - height);
    }

    float left = std::clamp(this->anchorFrame.getMinX(), PANEL_MARGIN, std::max(PANEL_MARGIN, screenWidth - PANEL_MARGIN - width));

    // Only when it moves: setting the position lays the screen out again
    if ((int)top != (int)this->panel->getLocalY() || (int)left != (int)this->panel->getLocalX())
    {
        this->panel->setPositionTop(top);
        this->panel->setPositionLeft(left);
    }
}

void ContextMenu::attach(View* view, std::function<void(ContextMenu* menu)> fill, ControllerButton button,
    const std::string& hint, bool longPress)
{
    auto show = [view, fill]()
    {
        auto* menu = new ContextMenu(view);
        fill(menu);
        menu->open();
    };

    view->registerAction(hint.empty() ? "hints/options"_i18n : hint, button, [show](View*)
        {
        show();
        return true; });

    if (longPress)
        view->addGestureRecognizer(new LongPressGestureRecognizer(view, [view, show]()
            {
            Application::giveFocus(view);
            show(); }));
}

} // namespace aurore
