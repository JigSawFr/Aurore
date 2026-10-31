// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <3ds.h>
#include <arpa/inet.h>
#include <malloc.h>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <vector>

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

// The keyboard

// What swkbd writes back: maxStringLength characters in UTF-8 (4 bytes at
// most each) and the NUL
static size_t textBufferSize(int maxStringLength)
{
    size_t characters = maxStringLength > 0 ? (size_t)maxStringLength : 256;
    return characters * 4 + 1;
}

// Aurore's keys not to type, as swkbd filters them (it has no filter for a
// space or a slash)
static u32 filterFlags(int disabled)
{
    u32 flags = 0;
    if (disabled & (KEYBOARD_DISABLE_AT | KEYBOARD_DISABLE_USERNAME))
        flags |= SWKBD_FILTER_AT;
    if (disabled & (KEYBOARD_DISABLE_PERCENT | KEYBOARD_DISABLE_USERNAME))
        flags |= SWKBD_FILTER_PERCENT;
    if (disabled & (KEYBOARD_DISABLE_BACKSLASH | KEYBOARD_DISABLE_USERNAME))
        flags |= SWKBD_FILTER_BACKSLASH;
    if (disabled & KEYBOARD_DISABLE_NUMBERS)
        flags |= SWKBD_FILTER_DIGITS;
    return flags;
}

// The keyboard shown, what was typed in `text`: false when it was cancelled
static bool showKeyboard(SwkbdType type, const std::string& headerText, const std::string& subText,
    int maxStringLength, const std::string& initialText, int disabled, bool password, std::string* text,
    int leftKey = 0, int rightKey = 0)
{
    SwkbdState swkbd;
    swkbdInit(&swkbd, type, 2, maxStringLength > 0 ? maxStringLength : -1);
    // The hint shows in the empty field: what to type
    std::string hint = headerText.empty() ? subText : headerText;
    if (!hint.empty())
        swkbdSetHintText(&swkbd, hint.c_str());
    if (!initialText.empty())
        swkbdSetInitialText(&swkbd, initialText.c_str());
    swkbdSetValidation(&swkbd, SWKBD_ANYTHING, filterFlags(disabled), 0);
    if (password)
        swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
    if (type == SWKBD_TYPE_NUMPAD)
        swkbdSetNumpadKeys(&swkbd, leftKey, rightKey);

    std::vector<char> buffer(textBufferSize(maxStringLength), 0);
    SwkbdButton button = swkbdInputText(&swkbd, buffer.data(), buffer.size());
    if (button != SWKBD_BUTTON_CONFIRM)
        return false;
    *text = buffer.data();
    return true;
}

bool CtrImeManager::openForText(std::function<void(std::string)> f, std::string headerText, std::string subText,
    int maxStringLength, std::string initialText, int kbdDisableBitmask)
{
    std::string text;
    if (!showKeyboard(SWKBD_TYPE_NORMAL, headerText, subText, maxStringLength, initialText, kbdDisableBitmask, false, &text))
        return false;
    f(text);
    return true;
}

bool CtrImeManager::openForPassword(std::function<void(std::string)> f, std::string headerText, std::string subText,
    int maxStringLength, std::string initialText, int kbdDisableBitmask)
{
    std::string text;
    if (!showKeyboard(SWKBD_TYPE_NORMAL, headerText, subText, maxStringLength, initialText, kbdDisableBitmask, true, &text))
        return false;
    f(text);
    return true;
}

bool CtrImeManager::openForNumber(std::function<void(long)> f, std::string headerText, std::string subText,
    int maxStringLength, std::string initialText, std::string leftButton, std::string rightButton,
    int kbdDisableBitmask)
{
    // The number pad's two keys left and right of 0: one character each
    // (ASCII: swkbd takes a UTF-16 unit)
    auto key = [](const std::string& label)
    { return label.size() == 1 ? (int)(unsigned char)label[0] : 0; };
    std::string text;
    if (!showKeyboard(SWKBD_TYPE_NUMPAD, headerText, subText, maxStringLength, initialText, kbdDisableBitmask, false,
            &text, key(leftButton), key(rightButton))
        || text.empty())
        return false;
    // The optional keys can leave a text that is not a number ("-", "."),
    // or one too large for a long
    long value;
    try
    {
        value = std::stol(text);
    }
    catch (const std::exception& e)
    {
        Logger::warning("3DS keyboard: \"{}\" is not a number: {}", text, e.what());
        return false;
    }
    f(value);
    return true;
}

void CtrFontLoader::loadFonts()
{
    NVGcontext* vg = Application::getNVGContext();
    if (!this->loadFontFromFile(FONT_REGULAR, TEXT_FONT_PATH))
        Logger::warning("3DS: no text font at {}", TEXT_FONT_PATH);
    if (this->loadFontFromFile(FONT_SWITCH_ICONS, BUTTONS_FONT_PATH))
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
    if (!this->loadMaterialFromResources())
        Logger::warning("3DS: no Material icons font in the resources");
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

    // The Wi-Fi (ac) and the sockets (soc: the IP address, and 3dslink),
    // which take a megabyte of their own
    acInit();
    this->socBuffer = static_cast<u32*>(memalign(0x1000, 0x100000));
    this->socReady  = this->socBuffer && R_SUCCEEDED(socInit(this->socBuffer, 0x100000));
    // The log: on the terminal of 3dslink when it started the app, else in
    // a file on the SD card (the top screen is the app's)
    if (this->socReady && __3dslink_host.s_addr != 0)
        link3dsStdio();
    else if (std::FILE* log = std::fopen("sdmc:/3ds/aurore.log", "w"))
        Logger::setLogOutput(log);

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
    if (this->socReady)
        socExit();
    std::free(this->socBuffer);
    acExit();
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

bool CtrPlatform::hasWirelessConnection()
{
    u32 status = 0;
    return R_SUCCEEDED(ACU_GetWifiStatus(&status)) && status != 0;
}

int CtrPlatform::getWirelessLevel()
{
    return this->hasWirelessConnection() ? osGetWifiStrength() : 0;
}

std::string CtrPlatform::getIpAddress()
{
    if (!this->socReady)
        return "-";
    // In the network's order: the first byte is the first number
    long ip = gethostid();
    if (ip == 0 || ip == -1)
        return "-";
    u32 a = (u32)ip;
    return std::format("{}.{}.{}.{}", a & 0xff, (a >> 8) & 0xff, (a >> 16) & 0xff, (a >> 24) & 0xff);
}

bool CtrPlatform::isBatteryCharging()
{
    u8 charging = 0;
    return R_SUCCEEDED(PTMU_GetBatteryChargeState(&charging)) && charging;
}

} // namespace aurore
