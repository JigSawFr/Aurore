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

// Every catalog of resources/i18n: valid JSON, strings only, keys en-US
// has, the same placeholders, and loaded by loadTranslations() (a catalog
// that cannot be parsed is left out with an error in the log, the app's
// text falling back to en-US without a word)

#include <aurore/core/i18n.hpp>
#include <aurore/extern/nlohmann/json.hpp>
#include <aurore/test/harness.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

using aurore::test::Harness;

namespace
{

const std::filesystem::path I18N = std::filesystem::path(AURORE_SOURCE_RESOURCES) / "i18n";

// The strings of a catalog by their getStr() name ("file/key/subkey");
// false when it is not JSON or holds something else than strings
bool readCatalog(const std::filesystem::path& file, std::map<std::string, std::string>* strings, std::string* error)
{
    std::ifstream in(file);
    std::stringstream text;
    text << in.rdbuf();
    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(text.str());
    }
    catch (const std::exception& e)
    {
        *error = e.what();
        return false;
    }
    std::string prefix = file.stem().string();
    bool ok            = true;
    auto walk          = [&](auto& self, const nlohmann::json& node, const std::string& name) -> void
    {
        if (node.is_object())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
                self(self, it.value(), name + "/" + it.key());
        }
        else if (node.is_string())
            (*strings)[name] = node.get<std::string>();
        else
        {
            *error = name + " is not a string";
            ok     = false;
        }
    };
    walk(walk, json, prefix);
    return ok;
}

size_t placeholders(const std::string& text)
{
    return (size_t)std::count(text.begin(), text.end(), '{');
}

} // namespace

AURORE_TEST(every_catalog_is_valid_and_loaded)
{
    Harness harness;
    std::map<std::string, std::string> english;
    std::string error;
    for (const auto& file : std::filesystem::directory_iterator(I18N / aurore::LOCALE_DEFAULT))
        AURORE_CHECK(readCatalog(file.path(), &english, &error));
    AURORE_CHECK(!english.empty());

    int locales = 0;
    for (const auto& folder : std::filesystem::directory_iterator(I18N))
    {
        std::string locale = folder.path().filename().string();
        std::map<std::string, std::string> strings;
        for (const auto& file : std::filesystem::directory_iterator(folder.path()))
        {
            error.clear();
            if (!readCatalog(file.path(), &strings, &error))
                aurore::test::reportFailure(__FILE__, __LINE__, locale + "/" + file.path().filename().string() + ": " + error);
        }

        aurore::loadTranslations(locale);
        for (const auto& [name, text] : strings)
        {
            auto en = english.find(name);
            if (en == english.end())
                aurore::test::reportFailure(__FILE__, __LINE__, locale + ": " + name + " is not in " + aurore::LOCALE_DEFAULT);
            else if (placeholders(text) != placeholders(en->second))
                aurore::test::reportFailure(__FILE__, __LINE__, locale + ": " + name + " has other placeholders than in " + aurore::LOCALE_DEFAULT);
            if (aurore::internal::getRawStr(name) != text)
                aurore::test::reportFailure(__FILE__, __LINE__, locale + ": " + name + " is not what loadTranslations() read");
        }
        locales++;
    }
    AURORE_CHECK_EQ(locales, 15);
    aurore::loadTranslations(aurore::LOCALE_DEFAULT);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
