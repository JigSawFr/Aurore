// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include <libretro-common/encodings/utf.h>

#include <cstring>
#include <iostream>
#include <aurore/core/box.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/thread.hpp>
#include <aurore/platforms/desktop/steam_deck.hpp>
#include <aurore/platforms/glfw/glfw_ime.hpp>
#include <aurore/views/dialog.hpp>
#include <aurore/views/edit_text_dialog.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{
static int currentIMEStatus = GLFW_FALSE;

static size_t encode_utf8(char* s, unsigned int ch)
{
    size_t count = 0;

    if (ch < 0x80)
        s[count++] = (char)ch;
    else if (ch < 0x800)
    {
        s[count++] = (ch >> 6) | 0xc0;
        s[count++] = (ch & 0x3f) | 0x80;
    }
    else if (ch < 0x10000)
    {
        s[count++] = (ch >> 12) | 0xe0;
        s[count++] = ((ch >> 6) & 0x3f) | 0x80;
        s[count++] = (ch & 0x3f) | 0x80;
    }
    else if (ch < 0x110000)
    {
        s[count++] = (ch >> 18) | 0xf0;
        s[count++] = ((ch >> 12) & 0x3f) | 0x80;
        s[count++] = ((ch >> 6) & 0x3f) | 0x80;
        s[count++] = (ch & 0x3f) | 0x80;
    }

    return count;
}

// The dialog's text as UTF-8, one code point per character of the buffer
static std::string toUtf8(const std::u32string& text)
{
    std::string out;
    char encoded[4];
    for (char32_t c : text)
        out.append(encoded, encode_utf8(encoded, c));
    return out;
}

// UTF-8 as code points, up to the first byte that starts none
static std::u32string fromUtf8(const std::string& text)
{
    std::u32string out(text.size(), U'\0');
    out.resize(utf8_conv_utf32((uint32_t*)out.data(), out.size(), text.data(), text.size()));
    return out;
}

void GLFWImeManager::ime_callback(GLFWwindow* window)
{
    currentIMEStatus = glfwGetInputMode(window, GLFW_IME);
    aurore::Logger::info("IME switched: {}", currentIMEStatus ? "ON" : "OFF");
}

std::string glfwPreeditText(int preeditCount, const unsigned int* preeditString, int blockCount,
    const int* blockSizes, int focusedBlock)
{
    std::string text;
    if (preeditCount <= 0 || blockCount <= 0 || !preeditString || !blockSizes)
        return text;
    // Past the last block (sizes adding up to less than the text): in none
    int blockIndex = -1, remainingBlockSize = 0;
    for (int i = 0; i < preeditCount; i++)
    {
        while (remainingBlockSize <= 0 && blockIndex < blockCount)
        {
            if (blockIndex == focusedBlock)
                text += "]";
            blockIndex++;
            remainingBlockSize = blockIndex < blockCount ? blockSizes[blockIndex] : preeditCount;
            if (blockIndex < blockCount && blockIndex == focusedBlock)
                text += "[";
        }
        char encoded[4];
        text.append(encoded, encode_utf8(encoded, preeditString[i]));
        remainingBlockSize--;
    }
    if (blockIndex < blockCount && blockIndex == focusedBlock)
        text += "]";
    return text;
}

void GLFWImeManager::preedit_callback(GLFWwindow* window, int preeditCount,
    unsigned int* preeditString, int blockCount,
    int* blockSizes, int focusedBlock, int caret)
{
    preeditTextBuffer = glfwPreeditText(preeditCount, preeditString, blockCount, blockSizes, focusedBlock);
}

void GLFWImeManager::char_callback(GLFWwindow* window, unsigned int codepoint)
{
    if (!showIME)
        return;
    if (cursor < 0 || cursor > (int)textBuffer.size())
        cursor = textBuffer.size();
    textBuffer.insert(textBuffer.begin() + cursor, (char32_t)codepoint);
    cursor++;
}

GLFWImeManager::GLFWImeManager(GLFWwindow* window)
    : window(window)
{
    showIME          = false;
    currentIMEStatus = glfwGetInputMode(window, GLFW_IME);
    glfwSetPreeditCursorRectangle(window, 0, 0, 1, 1);
    glfwSetIMEStatusCallback(window, ime_callback);
    glfwSetPreeditCallback(window, preedit_callback);
    glfwSetCharCallback(window, char_callback);
}

void GLFWImeManager::openInputDialog(std::function<void(std::string)> cb, std::string headerText,
    std::string subText, size_t maxStringLength, std::string initialText)
{
#ifdef __linux__
    if (isSteamDeck())
    {
        aurore::delay(200, []()
            { runSteamDeckCommand("steam://open/keyboard?Mode=0\n"); });
    }
#endif
    preeditTextBuffer.clear();
    glfwSetInputMode(window, GLFW_IME, GLFW_TRUE);
    showIME     = true;
    textBuffer  = fromUtf8(initialText);
    auto dialog = new EditTextDialog();
    dialog->setText(initialText);
    cursor = -1;
    dialog->setCursor(cursor);
    dialog->setHintText(subText);
    dialog->setHeaderText(headerText);
    dialog->setCountText(std::to_string(utf8len(initialText.data())) + "/" + std::to_string(maxStringLength));
    float scale = Application::windowScale / Application::getPlatform()->getVideoContext()->getScaleFactor();
    dialog->getLayoutEvent()->subscribe([this, scale](Point p)
        { glfwSetPreeditCursorRectangle(window, p.x * scale, p.y * scale, 1, 1); });

    dialog->getClipboardEvent()->subscribe([dialog, maxStringLength](const std::string& str)
        {
            if(isEditing || str.empty()) return;
            if (cursor < 0 || cursor > (int)textBuffer.size()) cursor = textBuffer.size();
            auto left = toUtf8(textBuffer.substr(0, cursor));
            auto right = toUtf8(textBuffer.substr(cursor, textBuffer.size()));
            auto clipboard = fromUtf8(str);
            textBuffer.insert(textBuffer.begin() + cursor, clipboard.begin(), clipboard.end());
            cursor += clipboard.size();
            if (textBuffer.size() > maxStringLength) {
                textBuffer = textBuffer.substr(0, maxStringLength);
            }
            if (cursor < 0 || cursor > (int)textBuffer.size()) cursor = textBuffer.size();
            dialog->setText(toUtf8(textBuffer));
            dialog->setCursor(cursor); });

    // update
    auto eventID = Application::getRunLoopEvent()->subscribe([dialog, maxStringLength]()
        {
            static std::u32string lastText = textBuffer;
            static std::string lastPreeditText = preeditTextBuffer;
            if(lastText != textBuffer){
                if(textBuffer.size() > maxStringLength)
                    textBuffer.erase(maxStringLength, textBuffer.size() - maxStringLength);
                lastText = textBuffer;
                if(textBuffer.empty()){
                    dialog->setText("");
                } else{
                    dialog->setText(getInputText());
                    dialog->setCursor(cursor);
                }
                dialog->setCountText(std::to_string(textBuffer.size()) + "/" + std::to_string(maxStringLength));
                lastPreeditText.clear();
                preeditTextBuffer.clear();
                isEditing = false;
            } else if(lastPreeditText != preeditTextBuffer){
                lastPreeditText = preeditTextBuffer;
                if (preeditTextBuffer.empty()) {
                    isEditing = false;
                    dialog->setText(getInputText());
                    dialog->setCursor(cursor);
                    return ;
                }
                isEditing = true;
                if (cursor < 0 || cursor > (int)textBuffer.size()) cursor = textBuffer.size();
                auto left = toUtf8(textBuffer.substr(0, cursor));
                auto right = toUtf8(textBuffer.substr(cursor, textBuffer.size()));
                dialog->setText(left + preeditTextBuffer + right);
                dialog->setCursor(cursor + preeditTextBuffer.size());
            } });

    dialog->getClearEvent()->subscribe([dialog]()
        {
            if (isEditing) return;
            textBuffer.clear();
            cursor = -1;
            dialog->setCursor(cursor); });

    // delete text
    dialog->getBackspaceEvent()->subscribe([dialog](...)
        {
            if(textBuffer.empty()) return true;
            if (cursor < 0 || cursor > (int)textBuffer.size()) cursor = textBuffer.size();
            if (cursor > 0 && cursor <= (int)textBuffer.size()) {
                textBuffer.erase(cursor - 1, 1);
                cursor--;
                dialog->setCursor(cursor);
            }
            return true; });

    dialog->registerAction(
        "hints/left"_i18n, BUTTON_LEFT, [dialog](...)
        {
            if (isEditing) return true;
            if (cursor == (int)CursorPosition::END) {
                cursor = textBuffer.empty() ? 0 : (int)textBuffer.size() - 1;
            } else if (cursor > (int)CursorPosition::START) {
                cursor--;
            }
            dialog->setCursor(cursor);
            return true; }, true, true);
    dialog->registerAction(
        "hints/right"_i18n, BUTTON_RIGHT, [dialog](...)
        {
            if (isEditing) return true;
            if (cursor >= (int)CursorPosition::START) {
                if (cursor < (int)textBuffer.size()) {
                    cursor++;
                    dialog->setCursor(cursor);
                }
            }
            return true; }, true, true);

    // cancel
    dialog->getCancelEvent()->subscribe([this, eventID]()
        {
            glfwSetInputMode(window, GLFW_IME, GLFW_FALSE);
            Application::getRunLoopEvent()->unsubscribe(eventID);
            showIME = false; });

    // submit
    dialog->getSubmitEvent()->subscribe([this, eventID, cb]()
        {
            glfwSetInputMode(window, GLFW_IME, GLFW_FALSE);
            Application::getRunLoopEvent()->unsubscribe(eventID);
            showIME = false;
            cb(getInputText());
            return true; });

    dialog->open();
}

std::string GLFWImeManager::getInputText()
{
    return toUtf8(textBuffer);
}

bool GLFWImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    int kbdDisableBitmask)
{
    this->openInputDialog([f](const std::string& text)
        { f(text); },
        headerText, subText, maxStringLength, initialText);
    return true;
}

bool GLFWImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    std::string leftButton, std::string rightButton,
    int kbdDisableBitmask)
{
    this->openInputDialog([f](const std::string& text)
        {
            if(text.empty()) return ;
            try
            {
                f(stoll(text));
            }
            catch (const std::invalid_argument& e)
            {
                Logger::error("Could not parse input, did you enter a valid integer? {}", e.what());
            }
            catch (const std::out_of_range& e) {
                Logger::error("Out of range: {}", e.what());
            }
            catch (const std::exception& e)
            {
                Logger::error("Unexpected error occurred: {}", e.what());
            } },
        headerText, subText, maxStringLength, initialText);
    return true;
}

};