// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <3ds.h>

#include <algorithm>

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/ctr/ctr_platform.hpp>

#define TEXT_FONT_PATH AURORE_ASSET("font/aurore_text.ttf")
#define BUTTONS_FONT_PATH AURORE_ASSET("font/aurore_buttons.ttf")

namespace aurore
{

// libctru's keys for each of Aurore's buttons, the Switch's layout (A on the
// right, B at the bottom); ZL, ZR and the C-stick are the New models'
static const u32 CTR_BUTTONS_MAPPING[_BUTTON_MAX] = {
    KEY_ZL, // BUTTON_LT
    KEY_L, // BUTTON_LB

    0, // BUTTON_LSB

    KEY_DUP, // BUTTON_UP
    KEY_DRIGHT, // BUTTON_RIGHT
    KEY_DDOWN, // BUTTON_DOWN
    KEY_DLEFT, // BUTTON_LEFT

    KEY_SELECT, // BUTTON_BACK
    0, // BUTTON_GUIDE
    KEY_START, // BUTTON_START

    0, // BUTTON_RSB

    KEY_Y, // BUTTON_Y
    KEY_B, // BUTTON_B
    KEY_A, // BUTTON_A
    KEY_X, // BUTTON_X

    KEY_R, // BUTTON_RB
    KEY_ZR, // BUTTON_RT

    // The D-pad, the Circle Pad (libctru's KEY_UP…) or the C-stick
    KEY_UP | KEY_CSTICK_UP, // BUTTON_NAV_UP
    KEY_RIGHT | KEY_CSTICK_RIGHT, // BUTTON_NAV_RIGHT
    KEY_DOWN | KEY_CSTICK_DOWN, // BUTTON_NAV_DOWN
    KEY_LEFT | KEY_CSTICK_LEFT, // BUTTON_NAV_LEFT
};

// The Circle Pad's and the C-stick's reach, about ±156 at the edge (clamped:
// it varies from a console to another); a stick's y grows upwards, Aurore's
// downwards
static constexpr float CTR_STICK_RANGE = 156.0f;

static float stickAxis(s16 value)
{
    return std::clamp(value / CTR_STICK_RANGE, -1.0f, 1.0f);
}

void CtrInputManager::updateUnifiedControllerState(ControllerState* state)
{
    this->updateControllerState(state, 0);
}

void CtrInputManager::updateControllerState(ControllerState* state, int controller)
{
    // hidScanInput() ran in mainLoopIteration()
    u32 keys = hidKeysHeld();
    for (size_t i = 0; i < _BUTTON_MAX; i++)
        state->buttons[i] = keys & CTR_BUTTONS_MAPPING[i];

    circlePosition pad, cstick;
    hidCircleRead(&pad);
    hidCstickRead(&cstick);
    state->axes[LEFT_X]  = stickAxis(pad.dx);
    state->axes[LEFT_Y]  = -stickAxis(pad.dy);
    state->axes[RIGHT_X] = stickAxis(cstick.dx);
    state->axes[RIGHT_Y] = -stickAxis(cstick.dy);
    // ZL and ZR are buttons, not triggers
    state->axes[LEFT_Z]  = state->buttons[BUTTON_LT] ? 1.0f : 0.0f;
    state->axes[RIGHT_Z] = state->buttons[BUTTON_RT] ? 1.0f : 0.0f;
}

void CtrInputManager::updateTouchStates(std::vector<RawTouchState>* states)
{
    // The bottom screen, one finger, in its pixels (320x240): the window's
    if (!(hidKeysHeld() & KEY_TOUCH))
        return;
    touchPosition touch;
    hidTouchRead(&touch);
    RawTouchState state;
    state.pressed    = true;
    state.fingerId   = 0;
    state.position.x = touch.px / Application::windowScale;
    state.position.y = touch.py / Application::windowScale;
    states->push_back(state);
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
