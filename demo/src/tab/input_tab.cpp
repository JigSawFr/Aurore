// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "tab/input_tab.hpp"

#include <aurore/cadran.hpp>

#include "view/input_views.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// A keyboard key as a keycap: its name in a rounded frame
aurore::Box* keycap(const std::string& name)
{
    auto* cap = new aurore::Box(aurore::Axis::ROW);
    cap->setPadding(4, 10, 4, 10);
    cap->setMinWidth(34);
    cap->setHeight(32);
    cap->setJustifyContent(aurore::JustifyContent::CENTER);
    cap->setAlignItems(aurore::AlignItems::CENTER);
    cap->setCornerRadius(8);
    cap->setBorderThickness(1);
    cap->setThemeColor("borderColor", "cadran/border");
    cap->setThemeColor("backgroundColor", "cadran/surf2");
    aurore::Label* label = kit::label(name, kit::TEXT_META, "cadran/text");
    label->setSingleLine(true);
    cap->addView(label);
    return cap;
}

// A line of the keyboard's table: the keys, then the button they press
aurore::Box* keyLine(const std::vector<std::string>& keys, const char* button, const std::string& what)
{
    aurore::Box* line = show::row(false, 8);
    line->setAlignItems(aurore::AlignItems::CENTER);
    line->setHeight(44);
    aurore::Box* caps = show::row(false, 6);
    caps->setWidth(132);
    caps->setShrink(0);
    caps->setAlignItems(aurore::AlignItems::CENTER);
    for (const std::string& key : keys)
        caps->addView(keycap(key));
    line->addView(caps);
    line->addView(kit::key(button, 28));
    aurore::Label* label = kit::label(what, kit::TEXT_BODY, "cadran/text2");
    label->setMarginLeft(8);
    label->setGrow(1);
    label->setWidth(0);
    line->addView(label);
    return line;
}
} // namespace

InputTab::InputTab()
{
    aurore::Box* page = show::page(this);

    // The controller
    aurore::Box* pad = show::section(page, "gamepad", "demo/input/controller"_i18n, "demo/input/controller_lead"_i18n);
    pad->addView(new ControllerView());

    // Touch and the mouse
    aurore::Box* touch = show::section(page, "phone", "demo/input/touch"_i18n, "demo/input/touch_lead"_i18n);
    touch->addView(new TouchPad());

    // The keyboard
    aurore::Box* keyboard = show::section(page, "code", "demo/input/keyboard"_i18n, "demo/input/keyboard_lead"_i18n);
    aurore::Box* keys     = show::card(false, false);
    keys->setGap(4);
    aurore::Box* columns = show::row(false, 32);
    aurore::Box* left    = show::column(4);
    left->setGrow(1);
    left->setWidth(0);
    left->addView(keyLine({ "Enter" }, "A", "demo/input/key_a"_i18n));
    left->addView(keyLine({ "Esc", "⌫" }, "B", "demo/input/key_b"_i18n));
    left->addView(keyLine({ "X" }, "X", "demo/input/key_x"_i18n));
    left->addView(keyLine({ "Y" }, "Y", "demo/input/key_y"_i18n));
    columns->addView(show::share(left, 1));
    aurore::Box* right = show::column(4);
    right->setGrow(1);
    right->setWidth(0);
    right->addView(keyLine({ "L", "R" }, "L", "demo/input/key_lr"_i18n));
    right->addView(keyLine({ "P", "+" }, "+", "demo/input/key_plus"_i18n));
    right->addView(keyLine({ "M", "−" }, "−", "demo/input/key_minus"_i18n));
    right->addView(keyLine({ "F12" }, "L3", "demo/input/key_inspector"_i18n));
    columns->addView(show::share(right, 1));
    keys->addView(columns);
    keyboard->addView(keys);

    // Hints: what a button does here, in the footer
    aurore::Box* hints = show::section(page, "info", "demo/input/hints"_i18n, "demo/input/hints_lead"_i18n);
    aurore::Box* row   = show::row();
    for (int i = 0; i < 3; i++)
    {
        aurore::Box* card = show::card(true, true);
        card->setHeight(96);
        card->setJustifyContent(aurore::JustifyContent::CENTER);
        static const char* const ICONS[]                = { "heart", "download", "trash" };
        static const char* const NAMES[]                = { "demo/input/hint_favourite", "demo/input/hint_download",
                           "demo/input/hint_remove" };
        static const aurore::ControllerButton BUTTONS[] = { aurore::BUTTON_Y, aurore::BUTTON_X, aurore::BUTTON_RB };
        aurore::Box* head                               = show::row(false, 12);
        head->setAlignItems(aurore::AlignItems::CENTER);
        head->addView(kit::icon(ICONS[i], 24, "cadran/teal"));
        head->addView(kit::label(aurore::getStr(NAMES[i]), kit::TEXT_BODY, "cadran/text"));
        card->addView(head);
        std::string name = aurore::getStr(NAMES[i]);
        card->registerAction(name, BUTTONS[i], [name](aurore::View*)
            {
            ui::notify(name, ui::Toast::Done);
            return true; });
        row->addView(show::share(card, 2));
    }
    hints->addView(row);
}

aurore::View* InputTab::create()
{
    return new InputTab();
}
