// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/ime.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace aurore
{

/**
 * The text GLFW's preedit callback describes (a word being composed with an
 * input method), its focused block between [ ]; sizes that do not add up to
 * the text leave the rest in no block.
 */
std::string glfwPreeditText(int preeditCount, const unsigned int* preeditString, int blockCount,
    const int* blockSizes, int focusedBlock);

class GLFWImeManager : public ImeManager
{
  public:
    GLFWImeManager(GLFWwindow* window);

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
    GLFWwindow* window;
    inline static bool showIME;
    inline static std::u32string textBuffer;
    inline static std::string preeditTextBuffer;
    inline static int cursor {};
    inline static bool isEditing {};
    static void ime_callback(GLFWwindow* window);
    static void preedit_callback(GLFWwindow* window, int preeditCount, unsigned int* preeditString,
        int blockCount, int* blockSizes, int focusedBlock, int caret);
    static void char_callback(GLFWwindow* window, unsigned int codepoint);
    static std::string getInputText();
};

};