// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "view/showcase.hpp"

#include <aurore/cadran/ui/cadran_modal.hpp>

namespace kit = cadran::kit;

namespace show
{

bool compact()
{
    return aurore::Application::getWidthClass() == aurore::WidthClass::COMPACT;
}

namespace
{
    // The tab frame's margins (@style/aurore/tab_frame/content_padding_*)
    float tabMargin(const char* key)
    {
        aurore::Style style = aurore::getStyle();
        return style[key];
    }
} // namespace

float pageTop()
{
    return compact() ? tabMargin("aurore/tab_frame/content_padding_top_bottom") : aurore::dp(28);
}

float pageSides()
{
    return compact() ? tabMargin("aurore/tab_frame/content_padding_sides") : aurore::dp(40);
}

float pageBottom()
{
    return aurore::dp(compact() ? 32 : 48);
}

float screenSides()
{
    return compact() ? pageSides() : aurore::dp(72);
}

float sectionGap()
{
    return aurore::dp(compact() ? 28 : 36);
}

float headGap()
{
    return aurore::dp(14);
}

float gap()
{
    return aurore::dp(16);
}

float padV()
{
    return aurore::dp(compact() ? 16 : 20);
}

float padH()
{
    return aurore::dp(compact() ? 16 : 22);
}

aurore::Box* page(ScrollView* scroll)
{
    auto* column = new aurore::Box(aurore::Axis::COLUMN);
    column->setPadding(pageTop(), pageSides(), pageBottom(), pageSides());
    scroll->setContentView(column);
    return column;
}

aurore::Box* section(aurore::Box* page, const char* icon, const std::string& title, const std::string& lead)
{
    aurore::Box* head = kit::kicker(icon, title);
    head->setMarginTop(page->getChildren().empty() ? 0 : sectionGap());
    head->setMarginBottom(lead.empty() ? headGap() : aurore::dp(6));
    page->addView(head);
    if (!lead.empty())
    {
        aurore::Label* line = kit::label(lead, kit::TEXT_NOTE, "cadran/text2");
        line->setMarginBottom(headGap());
        page->addView(line);
    }
    aurore::Box* content = column();
    page->addView(content);
    return content;
}

aurore::Box* row(bool wrap, float gap)
{
    auto* box = new aurore::Box(aurore::Axis::ROW);
    box->setGap(gap < 0 ? show::gap() : aurore::dp(gap));
    if (wrap || compact())
        box->setWrap(aurore::Wrap::WRAP);
    return box;
}

aurore::Box* column(float gap)
{
    auto* box = new aurore::Box(aurore::Axis::COLUMN);
    box->setGap(gap < 0 ? show::gap() : aurore::dp(gap));
    return box;
}

aurore::Box* card(bool focusable, bool grow)
{
    aurore::Box* box = kit::card(focusable, grow);
    box->setAxis(aurore::Axis::COLUMN);
    box->setPadding(padV(), padH(), padV(), padH());
    if (grow && compact())
    {
        // A line of its own: the rows wrap
        box->setGrow(0);
        box->setWidthPercentage(100);
    }
    else if (grow)
        box->setWidth(0); // the row shares its width, whatever the texts
    return box;
}

aurore::View* share(aurore::View* view, int perLine)
{
    if (!compact())
        return view;
    // A share of the line less the gaps, the rest grown into
    view->setWidthPercentage(100.0f / perLine - 6.0f);
    view->setGrow(1);
    return view;
}

aurore::Label* title(const std::string& text)
{
    return kit::label(text, kit::TEXT_LEAD, "cadran/text");
}

aurore::Label* note(const std::string& text)
{
    aurore::Label* label = kit::label(text, kit::TEXT_NOTE, "cadran/text2");
    label->setMarginTop(4);
    return label;
}

aurore::Button* button(const std::string& text, std::function<void()> run, bool primary)
{
    auto* button = new aurore::Button();
    button->setText(text);
    button->registerClickAction([run](aurore::View*)
        {
        run();
        return true; });
    cadran::style_button(button, primary ? cadran::Emphasis::PRIMARY : cadran::Emphasis::NONE);
    kit::fit_button(button);
    return button;
}

aurore::IconButton* iconButton(const char* icon, const std::string& text, std::function<void()> run)
{
    auto* button = new aurore::IconButton();
    button->setIconView(kit::icon(icon, 22, "cadran/text"));
    button->setIconSize(22);
    button->setText(text);
    button->registerClickAction([run](aurore::View*)
        {
        run();
        return true; });
    cadran::style_button(button, cadran::Emphasis::NONE);
    kit::fit_button(button);
    return button;
}

aurore::Box* choice(const char* icon, const std::string& name, std::function<void()> run)
{
    aurore::Box* tile = kit::card(true, false);
    tile->setAxis(aurore::Axis::ROW);
    tile->setAlignItems(aurore::AlignItems::CENTER);
    tile->setPadding(12, 18, 12, 16);
    tile->setGap(12);
    tile->setHeight(60);
    if (icon)
        tile->addView(kit::icon(icon, 24, "cadran/teal"));
    aurore::Label* label = kit::label(name, kit::TEXT_BODY, "cadran/text");
    label->setSingleLine(true);
    tile->addView(label);
    tile->registerClickAction([run](aurore::View*)
        {
        run();
        return true; });
    tile->addGestureRecognizer(new aurore::TapGestureRecognizer(tile));
    return tile;
}

void markChoice(aurore::Box* tile, bool chosen)
{
    tile->setThemeColor("borderColor", chosen ? "cadran/teal" : "cadran/line");
    tile->setBorderThickness(chosen ? 2 : 1);
    tile->setAccessibilitySelected(chosen);
}

aurore::Box* code(const std::string& text)
{
    auto* box = new aurore::Box(aurore::Axis::COLUMN);
    box->setPadding(14, 18, 14, 18);
    box->setCornerRadius(kit::RADIUS_ROW);
    box->setThemeColor("backgroundColor", "cadran/surf2");
    box->setBorderThickness(1);
    box->setThemeColor("borderColor", "cadran/line");
    aurore::Label* label = kit::label(text, kit::TEXT_META, "cadran/text2");
    label->setLineHeight(1.45f);
    box->addView(label);
    return box;
}

aurore::Label* centred(aurore::Label* label)
{
    label->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
    label->setWidthPercentage(100);
    return label;
}

} // namespace show
