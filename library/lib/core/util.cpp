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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <set>
#include <aurore/core/util.hpp>
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

#ifdef __PSV__
#include <psp2/kernel/processmgr.h>
#endif

namespace aurore
{

bool endsWith(const std::string& data, const std::string& suffix)
{
    return data.find(suffix, data.size() - suffix.size()) != std::string::npos;
}

bool startsWith(const std::string& data, const std::string& prefix)
{
    return data.rfind(prefix, 0) == 0;
}

[[noreturn]] void fatal(std::string message)
{
    aurore::Logger::error("Fatal error: {}", message);
#ifdef __PSV__
    sceKernelExitProcess(1);
#else
    throw std::logic_error(message);
#endif
}

std::string loadFileContents(const std::string& path)
{
    std::ifstream fin;
    fin.open(path, std::ios::in);
    if (!fin.is_open())
    {
        aurore::Logger::error("cannot open file: {}", path);
        return "";
    }

    std::string res, buff;
    while (getline(fin, buff))
    {
        res += buff + "\n";
    }
    fin.close();

    return res;
}

bool readSmallFile(const std::string& path, size_t maxBytes, std::string* text)
{
    uintmax_t size = 0;
    try
    {
        if (!fs::is_regular_file(path))
            return false;
        size = fs::file_size(path);
    }
    catch (...)
    {
        return false;
    }
    if (size > maxBytes)
        return false;
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    // A byte more than said: a file that has grown since is being written
    std::string read((size_t)size + 1, '\0');
    stream.read(&read[0], (std::streamsize)read.size());
    size_t count = (size_t)stream.gcount();
    if (count > size)
        return false;
    read.resize(count);
    *text = std::move(read);
    return true;
}

bool isResourceNameSafe(const std::string& name)
{
    size_t start = 0;
    while (start <= name.size())
    {
        size_t end = name.find_first_of("/\\", start);
        if (end == std::string::npos)
            end = name.size();
        if (name.compare(start, end - start, "..") == 0)
            return false;
        start = end + 1;
    }
    return true;
}

std::string escapeControlChars(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); i++)
    {
        unsigned char c = (unsigned char)text[i];
        // C1 controls in UTF-8: 0xC2 0x80 to 0xC2 0x9F
        bool c1 = c == 0xC2 && i + 1 < text.size() && (unsigned char)text[i + 1] >= 0x80 && (unsigned char)text[i + 1] <= 0x9F;
        if (c >= 0x20 && c != 0x7F && !c1)
        {
            out += (char)c;
            continue;
        }
        if (c1)
            c = (unsigned char)text[++i];
        char escaped[5];
        std::snprintf(escaped, sizeof(escaped), "\\x%02X", c);
        out += escaped;
    }
    return out;
}

// legacy-names: begin
const char* environmentVariable(const std::string& name)
{
    if (const char* value = std::getenv(("AURORE_" + name).c_str()))
        return value;
    for (const char* prefix : { "BRLS_", "BOREALIS_" })
    {
        std::string old = prefix + name;
        if (const char* value = std::getenv(old.c_str()))
        {
            static std::mutex mutex;
            static std::set<std::string> told;
            std::lock_guard<std::mutex> lock(mutex);
            if (told.insert(old).second)
                Logger::warning("{} is deprecated: use AURORE_{}", old, name);
            return value;
        }
    }
    return nullptr;
}

bool isLegacyKey(const std::string& key)
{
    return key.compare(0, 5, "brls/") == 0;
}

std::string canonicalKey(const std::string& key)
{
    return isLegacyKey(key) ? "aurore/" + key.substr(5) : key;
}

std::string canonicalXMLName(const std::string& name)
{
    return name.compare(0, 5, "brls:") == 0 ? "aurore:" + name.substr(5) : name;
}
// legacy-names: end

} // namespace aurore
