// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/touch/tap_gesture.hpp>
#include <aurore/views/sidebar.hpp>

using namespace aurore::literals;

namespace aurore
{

const std::string sidebarItemXML = R"xml(
    <aurore:Box
        width="auto"
        height="@style/aurore/sidebar/item_height"
        focusable="true" >

        <aurore:Rectangle
            id="aurore/sidebar/item_accent"
            width="@style/aurore/sidebar/item_accent_rect_width"
            height="auto"
            visibility="invisible"
            color="@theme/aurore/sidebar/active_item"
            marginTop="@style/aurore/sidebar/item_accent_margin_top_bottom"
            marginBottom="@style/aurore/sidebar/item_accent_margin_top_bottom"
            marginLeft="@style/aurore/sidebar/item_accent_margin_sides"
            marginRight="@style/aurore/sidebar/item_accent_margin_sides" />

        <aurore:Label
            id="aurore/sidebar/item_label"
            width="auto"
            height="auto"
            grow="1.0"
            fontSize="@style/aurore/sidebar/item_font_size"
            marginTop="@style/aurore/sidebar/item_accent_margin_top_bottom"
            marginBottom="@style/aurore/sidebar/item_accent_margin_top_bottom"
            marginRight="@style/aurore/sidebar/item_accent_margin_sides" />

    </aurore:Box>
)xml";

SidebarItem::SidebarItem()
    : Box(Axis::ROW)
{
    this->inflateFromXMLString(sidebarItemXML);

    this->registerStringXMLAttribute("label", [this](std::string value)
        { this->setLabel(value); });

    this->setFocusSound(SOUND_FOCUS_SIDEBAR);

    this->registerAction(
        "hints/ok"_i18n, BUTTON_A, [](View* view)
        {
            Application::onControllerButtonPressed(BUTTON_NAV_RIGHT, false);
            return true; },
        false, false, SOUND_CLICK_SIDEBAR);

    this->addGestureRecognizer(new TapGestureRecognizer([this](TapGestureStatus status, Sound* soundToPlay)
        {
        if (this->active)
            return;

        this->playClickAnimation(status.state != GestureState::UNSURE);

        switch (status.state)
        {
            case GestureState::UNSURE:
                *soundToPlay = SOUND_FOCUS_SIDEBAR;
                break;
            case GestureState::FAILED:
            case GestureState::INTERRUPTED:
                *soundToPlay = SOUND_TOUCH_UNFOCUS;
                break;
            case GestureState::END:
                *soundToPlay = SOUND_CLICK_SIDEBAR;
                Application::giveFocus(this);
                break;
            default:
                break;
        } }));
}

void SidebarItem::setActive(bool active)
{
    if (active == this->active)
        return;

    if (active)
    {
        this->activeEvent.fire(this);

        this->accent->setVisibility(Visibility::VISIBLE);
        this->label->setThemeColor("textColor", "aurore/sidebar/active_item");
    }
    else
    {
        this->accent->setVisibility(Visibility::INVISIBLE);
        this->label->setThemeColor("textColor", "aurore/text");
    }

    this->active = active;
}

void SidebarItem::onFocusGained()
{
    Box::onFocusGained();

    if (this->group)
        this->group->setActive(this);
}

void SidebarItem::onFocusLost()
{
    Box::onFocusLost();
}

void SidebarItem::setGroup(SidebarItemGroup* group)
{
    this->group = group;

    if (group)
        group->add(this);
}

GenericEvent* SidebarItem::getActiveEvent()
{
    return &this->activeEvent;
}

void SidebarItem::setLabel(std::string label)
{
    this->label->setText(label);
}

Sidebar::Sidebar()
{
    Style style = Application::getStyle();

    this->setScrollingBehavior(ScrollingBehavior::CENTERED);
    this->setBackground(ViewBackground::SIDEBAR);
    this->setThemeArea(ThemeArea::SIDEBAR);

    // Create content box
    this->contentBox = new Box(Axis::COLUMN);

    this->contentBox->setPadding(
        style["aurore/sidebar/padding_top"],
        style["aurore/sidebar/padding_right"],
        style["aurore/sidebar/padding_bottom"],
        style["aurore/sidebar/padding_left"]);

    this->setContentView(this->contentBox);
    this->setScrollingIndicatorVisible(false);
}

void Sidebar::addItem(std::string label, GenericEvent::Callback focusCallback)
{
    SidebarItem* item = new SidebarItem();
    item->setGroup(&this->group);
    item->setLabel(label);
    item->getActiveEvent()->subscribe(focusCallback);

    this->contentBox->addView(item);
}

SidebarItem* Sidebar::getItem(int position)
{
    auto& children = this->contentBox->getChildren();
    if (position < 0 || (size_t)position >= children.size())
        return nullptr;
    // nullptr for a separator
    return dynamic_cast<SidebarItem*>(children[position]);
}

void Sidebar::addSeparator()
{
    this->contentBox->addView(new SidebarSeparator());
}

void Sidebar::clearItems()
{
    this->contentBox->clearViews();
    group.clear();
}

View* Sidebar::create()
{
    return new Sidebar();
}

void SidebarItemGroup::add(SidebarItem* item)
{
    this->items.push_back(item);
}

void SidebarItemGroup::setActive(SidebarItem* active)
{
    for (SidebarItem* item : this->items)
    {
        if (item == active)
            item->setActive(true);
        else
            item->setActive(false);
    }
}

void SidebarItemGroup::clear()
{
    this->items.clear();
}

SidebarSeparator::SidebarSeparator()
{
    Style style = Application::getStyle();

    this->setHeight(style["aurore/sidebar/separator_height"]);
}

void SidebarSeparator::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    float midY = y + height / 2;

    nvgBeginPath(vg);
    nvgFillColor(vg, a(ctx->theme["aurore/sidebar/separator"]));
    nvgRect(vg, x, midY, width, 1);
    nvgFill(vg);
}

} // namespace aurore
