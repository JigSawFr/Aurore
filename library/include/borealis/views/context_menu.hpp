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

#pragma once

#include <borealis/core/box.hpp>
#include <borealis/views/label.hpp>
#include <functional>
#include <string>

namespace brls
{

// A line of a ContextMenu: an icon (optional), a label, what it does.
class ContextMenuItem : public Box
{
  public:
    ContextMenuItem(const std::string& label, View* icon);

    Label* getLabel() const { return this->label; }

  private:
    Label* label;
};

// A short list of actions in a popup next to a view: below it, or above
// when there is no room below, kept on screen. It opens over the screen
// (the screen stays visible around it); B, a touch or a click outside it,
// or choosing a line closes it.
//
//     auto* menu = new brls::ContextMenu(cell);
//     menu->addItem("Rename", [] { ... }, "\uE3C9");   // a Material Icons glyph
//     menu->addItem("Delete", [] { ... }, "@res/img/delete.png");
//     menu->open();
//
// ContextMenu::attach() opens one with X (and a long press) on a view.
class ContextMenu : public Box
{
  public:
    explicit ContextMenu(View* anchor);

    // A line. `icon`: a picture ("@res/…" or a file path), or one Material
    // Icons glyph; empty: none. `action` runs once the menu is closed.
    ContextMenuItem* addItem(const std::string& label, std::function<void()> action, const std::string& icon = "");

    // The same with a view of the app's as the icon (a vector icon…).
    ContextMenuItem* addItem(const std::string& label, std::function<void()> action, View* icon);

    // A line between two groups of lines.
    void addSeparator();

    // A title above the lines.
    void setTitle(const std::string& title);

    // Pushes the menu over the current screen.
    void open();

    // Closes it, then runs `then` (if any).
    void close(std::function<void()> then = nullptr);

    // Opens a menu, which `fill` fills, with `button` on `view` (its hint
    // reads `hint`, "Options" when empty), and with a long press on it
    // when `longPress`.
    static void attach(View* view, std::function<void(ContextMenu* menu)> fill, ControllerButton button = BUTTON_X,
        const std::string& hint = "", bool longPress = true);

    bool isTranslucent() override { return true; }
    View* getDefaultFocus() override;
    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

  private:
    View* anchor;
    Box* panel;
    Label* title = nullptr;
    bool closing = false;
    Rect anchorFrame;

    void place();
};

} // namespace brls
