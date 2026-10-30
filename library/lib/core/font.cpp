// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <unistd.h>

#include <aurore/core/application.hpp>
#include <aurore/core/assets.hpp>
#include <aurore/core/font.hpp>

#define MATERIAL_ICONS "material/MaterialIcons-Regular.ttf"
#define MATERIAL_ICONS_PATH AURORE_ASSET(MATERIAL_ICONS)

namespace aurore
{

bool FontLoader::loadFontFromFile(std::string fontName, std::string filePath)
{
    if (filePath.starts_with("@res/"))
        return Application::loadFontFromFile(fontName, filePath);

    if (access(filePath.c_str(), F_OK) != -1)
    {
        bool loaded = Application::loadFontFromFile(fontName, filePath);

        if (!loaded)
        {
            Logger::error("{} font was located but couldn't be loaded", fontName);
            return false;
        }

        return true;
    }

    Logger::warning("\"{}\" font couldn't be located (searched at \"{}\")", fontName, filePath);
    return false;
}

bool FontLoader::loadMaterialFromResources()
{
#ifdef USE_LIBROMFS
    auto& font = romfs::get(MATERIAL_ICONS);
    if (!font.valid())
    {
        return false;
    }
    return Application::loadFontFromMemory(FONT_MATERIAL_ICONS, (void*)font.data(), font.size(), false);
#else
    return this->loadFontFromFile(FONT_MATERIAL_ICONS, MATERIAL_ICONS_PATH);
#endif
}

} // namespace aurore
