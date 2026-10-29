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

#include <3ds.h>

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/ctr/ctr_platform.hpp>

#define TEXT_FONT_PATH AURORE_ASSET("font/aurore_text.ttf")
#define BUTTONS_FONT_PATH AURORE_ASSET("font/aurore_buttons.ttf")

namespace aurore
{

void CtrInputManager::updateUnifiedControllerState(ControllerState* state)
{
    *state = {};
}

void CtrInputManager::updateControllerState(ControllerState* state, int controller)
{
    *state = {};
}

void CtrFontLoader::loadFonts()
{
    NVGcontext* vg = Application::getNVGContext();
    if (!this->loadFontFromFile(FONT_REGULAR, TEXT_FONT_PATH))
        Logger::warning("3DS: no text font at {}", TEXT_FONT_PATH);
    if (this->loadFontFromFile(FONT_SWITCH_ICONS, BUTTONS_FONT_PATH))
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
    this->loadMaterialFromResources();
}

CtrPlatform::CtrPlatform()
{
    // The system's settings (language) and the power controller (battery)
    cfguInit();
    ptmuInit();
}

CtrPlatform::~CtrPlatform()
{
    delete this->videoContext;
    ptmuExit();
    cfguExit();
}

void CtrPlatform::createWindow(std::string title, uint32_t width, uint32_t height, float windowXPos, float windowYPos)
{
    this->videoContext = new CtrVideoContext();
}

const LayoutPreset* CtrPlatform::getLayoutPreset()
{
    return &PRESET_SMALL_SCREEN;
}

bool CtrPlatform::mainLoopIteration()
{
    // HOME or the power button ask the app to close
    return aptMainLoop();
}

std::string CtrPlatform::getLocale()
{
    u8 language = CFG_LANGUAGE_EN;
    if (R_FAILED(CFGU_GetSystemLanguage(&language)))
        return LOCALE_DEFAULT;
    switch (language)
    {
        case CFG_LANGUAGE_JP:
            return LOCALE_JA;
        case CFG_LANGUAGE_FR:
            return LOCALE_FR;
        case CFG_LANGUAGE_DE:
            return LOCALE_DE;
        case CFG_LANGUAGE_IT:
            return LOCALE_IT;
        case CFG_LANGUAGE_ES:
            return LOCALE_ES;
        case CFG_LANGUAGE_ZH:
            return LOCALE_ZH_HANS;
        case CFG_LANGUAGE_KO:
            return LOCALE_Ko;
        case CFG_LANGUAGE_NL:
            return LOCALE_NL;
        case CFG_LANGUAGE_PT:
            return LOCALE_PT;
        case CFG_LANGUAGE_RU:
            return LOCALE_RU;
        case CFG_LANGUAGE_TW:
            return LOCALE_ZH_HANT;
        default:
            return LOCALE_EN_US;
    }
}

int CtrPlatform::getBatteryLevel()
{
    // 0 to 5 bars
    u8 level = 5;
    if (R_FAILED(PTMU_GetBatteryLevel(&level)))
        return 100;
    return level * 20;
}

bool CtrPlatform::isBatteryCharging()
{
    u8 charging = 0;
    return R_SUCCEEDED(PTMU_GetBatteryChargeState(&charging)) && charging;
}

} // namespace aurore
