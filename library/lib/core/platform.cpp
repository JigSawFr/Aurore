/*
    Copyright 2021 natinusala

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

#if defined(AURORE_USE_DEKO3D)
#elif defined(__GLFW__)
#include <aurore/platforms/glfw/glfw_platform.hpp>
#elif defined(__SDL2__)
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
#elif defined(__SDL2__)
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
