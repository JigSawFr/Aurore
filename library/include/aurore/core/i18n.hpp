// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fmt/format.h>

#include <string>
#include <aurore/core/logger.hpp>

namespace aurore
{

const std::string LOCALE_JA      = "ja";
const std::string LOCALE_EN_US   = "en-US";
const std::string LOCALE_EN_GB   = "en-GB";
const std::string LOCALE_FR      = "fr";
const std::string LOCALE_FR_CA   = "fr-CA";
const std::string LOCALE_DE      = "de";
const std::string LOCALE_IT      = "it";
const std::string LOCALE_ES      = "es";
const std::string LOCALE_ZH_CN   = "zh-CN";
const std::string LOCALE_ZH_HANS = "zh-Hans";
const std::string LOCALE_ZH_HANT = "zh-Hant";
const std::string LOCALE_ZH_TW   = "zh-TW";
const std::string LOCALE_Ko      = "ko";
const std::string LOCALE_NL      = "nl";
const std::string LOCALE_PT      = "pt";
const std::string LOCALE_PT_BR   = "pt-BR";
const std::string LOCALE_RU      = "ru";
const std::string LOCALE_ES_419  = "es-419";

/// custom locale
// Rename ja-RYU to RYU, as there may be a bug in PkgTool that not allow dirs with same prefix exist.
#ifdef PS4
const std::string LOCALE_RYU = "RYU";
#else
const std::string LOCALE_RYU = "ja-RYU";
#endif
const std::string LOCALE_AUTO = "auto";

const std::string LOCALE_DEFAULT = LOCALE_EN_US;

/**
 * The locale Aurore uses (one of the LOCALE_ above) for a system one:
 * POSIX ("fr_FR.UTF-8", "pt_BR@euro"), BCP 47 ("fr-CA", "zh-Hant-TW") or a
 * language alone ("de"). en-US when no locale of Aurore matches.
 */
std::string localeFromSystem(std::string system);

namespace internal
{
    std::string getRawStr(std::string stringName);
} // namespace internal

/**
 * Returns the translation for the given string,
 * after injecting format parameters (if any)
 */
template <typename... Args>
std::string getStr(std::string stringName, Args&&... args)
{
    std::string rawStr = internal::getRawStr(stringName);

    try
    {
        return fmt::format(fmt::runtime(rawStr), args...);
    }
    catch (const std::exception& e)
    {
        Logger::error("Invalid format \"{}\" from string \"{}\": {}", rawStr, stringName, e.what());
        return stringName;
    }
}

/**
 * Loads all translations of the current system locale + default locale
 * Must be called before trying to get a translation!
 */
void loadTranslations();

/**
 * Loads the translations of `locale` in place of the current ones, while the
 * app runs (the platform's locale is left as it is): the strings read after
 * it are in that language
 */
void loadTranslations(const std::string& locale);

inline namespace literals
{
    /**
     * Returns the translation for the given string, without
     * injecting any parameters
     * Shortcut to aurore::getStr(stringName)
     */
    std::string operator""_i18n(const char* str, size_t len);
} // namespace literals
} // namespace aurore
