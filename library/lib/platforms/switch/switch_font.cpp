// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <switch.h>

#include <aurore/core/application.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/switch/switch_font.hpp>

namespace aurore
{

void SwitchFontLoader::loadFonts()
{
    PlFontData font;
    Result rc;
    NVGcontext* vg = aurore::Application::getNVGContext();

    aurore::Logger::info("switch system locale: {}", aurore::Application::getPlatform()->getLocale());

    // Standard
    rc = plGetSharedFontByType(&font, PlSharedFontType_Standard);
    if (R_FAILED(rc))
        Logger::error("switch: could not load Standard shared font: {:#x}", rc);
    else if (!Application::loadFontFromMemory(FONT_REGULAR, font.address, font.size, false))
        Logger::error("switch: the Standard shared font could not be read");

    // Simplified Chinese
    // custom Font
    if (access(USER_FONT_PATH.c_str(), F_OK) != -1)
    {
        aurore::Logger::info("Load custom font: {}", USER_FONT_PATH);
        if (this->loadFontFromFile(FONT_CHINESE_SIMPLIFIED, USER_FONT_PATH))
            nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_REGULAR));
        else
        {
            // A custom font that cannot be read: the system's one instead
            Logger::error("switch: could not load the custom font {}", USER_FONT_PATH);
            rc = plGetSharedFontByType(&font, PlSharedFontType_ChineseSimplified);
            if (R_SUCCEEDED(rc) && Application::loadFontFromMemory(FONT_CHINESE_SIMPLIFIED, font.address, font.size, false))
                nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_REGULAR));
        }
    }
    else
    {
        aurore::Logger::warning("Cannot find custom font, (Searched at: {})", USER_FONT_PATH);
        rc = plGetSharedFontByType(&font, PlSharedFontType_ChineseSimplified);
        if (R_SUCCEEDED(rc) && Application::loadFontFromMemory(FONT_CHINESE_SIMPLIFIED, font.address, font.size, false))
            nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_REGULAR));
        else
        {
            Logger::error("switch: could not load Chinese Simplified shared font: {:#x}", rc);
            // Every label is drawn with this font's name: without it no text
            // at all would show. The Standard font takes its place.
            rc = plGetSharedFontByType(&font, PlSharedFontType_Standard);
            if (R_SUCCEEDED(rc) && !Application::loadFontFromMemory(FONT_CHINESE_SIMPLIFIED, font.address, font.size, false))
                Logger::error("switch: the Standard shared font could not stand in for Chinese Simplified");
        }
    }

    // Simplified Chinese ext
    rc = plGetSharedFontByType(&font, PlSharedFontType_ExtChineseSimplified);
    if (R_SUCCEEDED(rc) && Application::loadFontFromMemory(FONT_CHINESE_SIMPLIFIED_EXT, font.address, font.size, false))
        nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_CHINESE_SIMPLIFIED_EXT));
    else
        Logger::error("switch: could not load Chinese Simplified Extended shared font: {:#x}", rc);

    // Traditional Chinese
    rc = plGetSharedFontByType(&font, PlSharedFontType_ChineseTraditional);
    if (R_SUCCEEDED(rc) && Application::loadFontFromMemory(FONT_CHINESE_TRADITIONAL, font.address, font.size, false))
        nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_CHINESE_TRADITIONAL));
    else
        Logger::error("switch: could not load Chinese Traditional shared font: {:#x}", rc);

    // Korean
    rc = plGetSharedFontByType(&font, PlSharedFontType_KO);
    if (R_SUCCEEDED(rc) && Application::loadFontFromMemory(FONT_KOREAN_REGULAR, font.address, font.size, false))
        nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_KOREAN_REGULAR));
    else
        Logger::error("switch: could not load Korean shared font: {:#x}", rc);

    // Extended (symbols)
    rc = plGetSharedFontByType(&font, PlSharedFontType_NintendoExt);
    if (R_SUCCEEDED(rc) && Application::loadFontFromMemory(FONT_SWITCH_ICONS, font.address, font.size, false))
        nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_SWITCH_ICONS));
    else
        Logger::error("switch: could not load Extented shared font: {:#x}", rc);

    // Material icons
    if (this->loadMaterialFromResources())
        nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont(FONT_MATERIAL_ICONS));
    else
        Logger::error("switch: could not load Material icons font from resources");

    // Load Emoji
    if (!USER_EMOJI_PATH.empty())
    {
        if (access(USER_EMOJI_PATH.c_str(), F_OK) != -1)
        {
            aurore::Logger::info("Load emoji font: {}", USER_EMOJI_PATH);
            if (this->loadFontFromFile("emoji", USER_EMOJI_PATH))
                nvgAddFallbackFontId(vg, Application::getFont(FONT_CHINESE_SIMPLIFIED), Application::getFont("emoji"));
            else
                aurore::Logger::warning("Emoji font not loaded: {}", USER_EMOJI_PATH);
        }
        else
        {
            aurore::Logger::warning("Cannot find custom emoji, (Searched at: {})", USER_EMOJI_PATH);
        }
    }
}

} // namespace aurore
