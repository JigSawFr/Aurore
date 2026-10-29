// SPDX-FileCopyrightText: 2023 zeromake
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/event.hpp>
#include <aurore/core/ime.hpp>
#include <aurore/platforms/sdl/sdl.hpp>

namespace aurore
{
class SDLImeManager : public ImeManager
{
  public:
    SDLImeManager(Event<SDL_Event*>* event);

    bool openForText(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

    bool openForNumber(std::function<void(long)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 18, std::string initialText = "",
        std::string leftButton = "", std::string rightButton = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

    void openInputDialog(std::function<void(std::string)> cb, std::string headerText,
        std::string subText, size_t maxStringLength = 50, std::string initialText = "");

  private:
    Event<SDL_Event*>* event;
    int cursor;
    std::string inputBuffer;
    bool isEditing = false; // 是否正在编辑文字
    std::string editingBuffer;
};
}