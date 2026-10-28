// The stand-in for borealis' i18n that ui/tr.hpp asks of a host suite
// (BRLS_CADRAN_HOST_I18N): aurore::internal::getRawStr and "…"_i18n over
// Cadran's real catalog, resources/i18n/<lang>/cadran.json. A key the
// catalog lacks reads as itself, as in borealis, and is noted in
// catalog::missing. Included once, by the suite's only source file.
#pragma once

#include <fstream>
#include <map>
#include <set>
#include <string>

#include <aurore/extern/nlohmann/json.hpp>

namespace catalog
{
inline std::map<std::string, std::string> strings;
inline std::set<std::string> missing; // keys asked for that the catalog lacks

inline void flatten(const nlohmann::json& j, const std::string& prefix)
{
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        if (it->is_object())
            flatten(*it, prefix + it.key() + "/");
        else if (it->is_string())
            strings[prefix + it.key()] = it->get<std::string>();
    }
}

// Cadran's catalog in `lang` ("en-US", "fr"), from the source tree.
inline bool load(const std::string& lang)
{
    strings.clear();
    std::ifstream in(std::string(AURORE_SOURCE_RESOURCES) + "i18n/" + lang + "/cadran.json");
    if (!in)
        return false;
    flatten(nlohmann::json::parse(in), "cadran/");
    return !strings.empty();
}
} // namespace catalog

namespace aurore
{
namespace internal
{
    std::string getRawStr(std::string name)
    {
        const auto it = catalog::strings.find(name);
        if (it != catalog::strings.end())
            return it->second;
        catalog::missing.insert(name);
        return name;
    }
} // namespace internal

inline namespace literals
{
    std::string operator""_i18n(const char* str, size_t len)
    {
        return internal::getRawStr(std::string(str, len));
    }
} // namespace literals
} // namespace aurore
