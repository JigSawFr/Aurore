// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

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
    // 804 MHz and the L2 cache
    osSetSpeedupEnable(true);
    // The system's settings (language), the power controller (battery), the
    // app's resources
    cfguInit();
    ptmuInit();
    if (R_FAILED(romfsInit()))
        Logger::warning("3DS: no romfs, the resources are missing");

    // The three New models have the same CPU, memory and screens; the New
    // 2DS XL has no stereoscopic 3D, which nothing here uses
    u8 model = CFG_MODEL_N3DS;
    CFGU_GetSystemModel(&model);
    switch (model)
    {
        case CFG_MODEL_N3DS:
            this->modelName = "New 3DS";
            break;
        case CFG_MODEL_N3DSXL:
            this->modelName = "New 3DS XL";
            break;
        case CFG_MODEL_N2DSXL:
            this->modelName = "New 2DS XL";
            break;
        case CFG_MODEL_3DSXL:
            this->modelName = "3DS XL";
            break;
        case CFG_MODEL_2DS:
            this->modelName = "2DS";
            break;
        default:
            this->modelName = "3DS";
            break;
    }
    bool isNew = false;
    APT_CheckNew3DS(&isNew);
    this->newModel = isNew;
    if (!this->newModel)
        Logger::warning("3DS: the {} is not supported, only the New 3DS, New 3DS XL and New 2DS XL are (it runs "
                        "at 268 MHz, without the L2 cache)",
            this->modelName);
}

CtrPlatform::~CtrPlatform()
{
    delete this->videoContext;
    romfsExit();
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
    if (!aptMainLoop())
        return false;
    // The buttons and the touch screen, read once per frame (hidKeysDown,
    // hidKeysHeld… then hold this frame's)
    hidScanInput();
    return true;
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
