/*
    Copyright 2020-2021 natinusala

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

#include <borealis/core/application.hpp>
#include <borealis/core/assets.hpp>
#include <borealis/core/i18n.hpp>
#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#elif __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Failed to include <filesystem> header!"
#endif
#include <fstream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

#ifndef BRLS_I18N_PREFIX
#define BRLS_I18N_PREFIX ""
#endif

namespace brls
{

// The strings of the default locale and of the current one, never changed
// once published: a new language is loaded into a new table, put in place of
// the old one with an atomic store. getStr may be called from any thread
// while loadTranslations runs on another: a lookup takes the table with an
// atomic load and keeps it alive until it has copied its string, so a table
// replaced meanwhile is freed only after its last reader.
struct Translations
{
    nlohmann::json defaultLocale = nlohmann::json::object();
    nlohmann::json currentLocale = nlohmann::json::object();
};

static std::shared_ptr<const Translations> translations;

// One load at a time (each one builds on the table in place).
static std::mutex loadMutex;

static void loadLocale(std::string locale, nlohmann::json* target)
{
    if (locale.empty())
        return;
#ifdef USE_LIBROMFS
    auto localePath = romfs::list("i18n/" + locale);
    if (localePath.empty())
    {
        Logger::error("Cannot load locale {}: directory i18n/{} doesn't exist", locale, locale);
        return;
    }
    for (auto& entry : localePath)
    {
        std::string path = entry.string();
        std::string name = entry.filename().string();
        if (!endsWith(name, ".json"))
            continue;

        (*target)[name.substr(0, name.length() - 5)] = nlohmann::json::parse(romfs::get(path).string());
    }
#else
    std::string localePath = BRLS_ASSET("i18n/" + locale);

    if (!fs::exists(localePath))
    {
        Logger::error("Cannot load locale {}: directory {} doesn't exist", locale, localePath);
        return;
    }
    else if (!fs::is_directory(localePath))
    {
        Logger::error("Cannot load locale {}: {} isn't a directory", locale, localePath);
        return;
    }

    // Iterate over all JSON files in the directory
    for (const fs::directory_entry& entry : fs::directory_iterator(localePath))
    {
        if (fs::is_directory(entry))
            continue;

        std::string name = entry.path().filename().string();

        if (!endsWith(name, ".json"))
            continue;

        std::string path = entry.path().string();

        nlohmann::json strings;

        std::ifstream jsonStream;
        jsonStream.open(path);

        try
        {
            jsonStream >> strings;
        }
        catch (const std::exception& e)
        {
            Logger::error("Error while loading \"{}\": {}", path, e.what());
        }

        jsonStream.close();

        (*target)[name.substr(0, name.length() - 5)] = strings;
    }
#endif /* USE_LIBROMFS */
}

void loadTranslations()
{
    std::lock_guard<std::mutex> lock(loadMutex);
    auto next = std::make_shared<Translations>();
    loadLocale(LOCALE_DEFAULT, &next->defaultLocale);

    std::string currentLocaleName = Application::getLocale();
    if (currentLocaleName != LOCALE_DEFAULT)
        loadLocale(currentLocaleName, &next->currentLocale);
    std::atomic_store(&translations, std::shared_ptr<const Translations>(std::move(next)));
}

void loadTranslations(const std::string& locale)
{
    std::lock_guard<std::mutex> lock(loadMutex);
    auto next                                    = std::make_shared<Translations>();
    std::shared_ptr<const Translations> previous = std::atomic_load(&translations);
    if (previous && !previous->defaultLocale.empty())
        next->defaultLocale = previous->defaultLocale;
    else
        loadLocale(LOCALE_DEFAULT, &next->defaultLocale);
    if (locale != LOCALE_DEFAULT)
        loadLocale(locale, &next->currentLocale);
    std::atomic_store(&translations, std::shared_ptr<const Translations>(std::move(next)));
}

namespace internal
{
    std::string getRawStr(std::string stringName)
    {
        nlohmann::json::json_pointer pointer;

        try
        {
            pointer = nlohmann::json::json_pointer("/" + std::string(BRLS_I18N_PREFIX) + stringName);
        }
        catch (const std::exception& e)
        {
            Logger::error("Error while getting string \"{}\": {}", stringName, e.what());
            return stringName;
        }

        // The table as it is now, kept for this lookup even if another
        // thread loads a new language meanwhile.
        std::shared_ptr<const Translations> table = std::atomic_load(&translations);
        if (!table)
            return stringName;

        // First look for translated string in current locale, then in the
        // default locale. The current one is empty when it is the default
        // locale: skipped, not looked up (a lookup that misses throws, and
        // operator[] added a null to it each time).
        for (const nlohmann::json* locale : { &table->currentLocale, &table->defaultLocale })
        {
            if (locale->is_null() || locale->empty())
                continue;
            try
            {
                return locale->at(pointer).get<std::string>();
            }
            catch (...)
            {
            }
        }

        // Fallback to returning the string name
        return stringName;
    }
} // namespace internal

inline namespace literals
{
    std::string operator""_i18n(const char* str, size_t len)
    {
        return internal::getRawStr(std::string(str, len));
    }

} // namespace literals

} // namespace brls
