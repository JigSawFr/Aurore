// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cctype>
#include <aurore/core/platform.hpp>

#ifdef __SWITCH__
#include <aurore/platforms/switch/switch_platform.hpp>
#endif

#ifdef ANDROID
#include <aurore/platforms/android/android_platform.hpp>
#endif

#ifdef __PSV__
#include <aurore/platforms/psv/psv_platform.hpp>
#endif

#ifdef PS4
#include <aurore/platforms/ps4/ps4_platform.hpp>
#endif

#ifdef __3DS__
#include <aurore/platforms/ctr/ctr_platform.hpp>
#endif

#if defined(AURORE_USE_DEKO3D)
#elif defined(__GLFW__)
#include <aurore/platforms/glfw/glfw_platform.hpp>
#elif defined(__SDL__)
#include <aurore/platforms/sdl/sdl_platform.hpp>
#endif

namespace aurore
{

Platform* Platform::createPlatform()
{
    if (Platform::factory)
        return Platform::factory();

#if defined(__SWITCH__)
    return new SwitchPlatform();
#elif defined(ANDROID)
    return new AndroidPlatform();
#elif defined(__PSV__)
    return new PsvPlatform();
#elif defined(PS4)
    return new Ps4Platform();
#elif defined(__3DS__)
    return new CtrPlatform();
#elif defined(__SDL__)
    return new SDLPlatform();
#elif defined(__GLFW__)
    return new GLFWPlatform();
#endif

    return nullptr;
}

bool Platform::isBrowserUrl(const std::string& url)
{
    size_t sep = url.find("://");
    if (sep == std::string::npos || sep == 0 || sep + 3 == url.size())
        return false;

    std::string scheme;
    for (size_t i = 0; i < sep; i++)
        scheme += (char)std::tolower((unsigned char)url[i]);
    if (std::find(BROWSER_URL_SCHEMES.begin(), BROWSER_URL_SCHEMES.end(), scheme) == BROWSER_URL_SCHEMES.end())
        return false;

    for (unsigned char c : url)
        if (c < 0x20 || c == 0x7f)
            return false;
    return true;
}

} // namespace aurore
