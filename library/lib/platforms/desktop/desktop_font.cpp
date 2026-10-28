/*
    Copyright 2021 natinusala
    Copyright 2023 xfangfang

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

#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <borealis/core/application.hpp>
#include <borealis/core/assets.hpp>
#include <borealis/platforms/desktop/desktop_font.hpp>

#if defined(__linux__) && !defined(ANDROID)
#include <dlfcn.h>
#endif
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#endif

// Inter and PromptFont's buttons, made by tools/make_fonts.py
#define INTER_FONT_PATH BRLS_ASSET("font/aurore_text.ttf")
#define INTER_ICON_PATH BRLS_ASSET("font/aurore_buttons.ttf")
// An app's own font for Chinese, Japanese and Korean (.ttc, .ttf or .otf)
#define APP_CJK_FONT_PATH BRLS_ASSET("font/cjk")

namespace brls
{

namespace
{

    const std::vector<std::string> fontExts = {
        ".ttc",
        ".ttf",
        ".otf",
    };

    struct CjkLanguage
    {
        const char* language;
        const std::string& fontName;
        uint32_t sample; // a character only a font of this language is sure to have
    };

    const std::vector<CjkLanguage>& cjkLanguages()
    {
        static const std::vector<CjkLanguage> languages = {
            { "ja", FONT_JAPANESE, 0x3042 }, // あ
            { "zh-Hans", FONT_CHINESE_SIMPLIFIED, 0x7B80 }, // 简
            { "zh-Hant", FONT_CHINESE_TRADITIONAL, 0x7E41 }, // 繁
            { "ko", FONT_KOREAN_REGULAR, 0xD55C }, // 한
        };
        return languages;
    }

    // The language of a locale among cjkLanguages(), or an empty string
    std::string cjkLanguageOf(const std::string& locale)
    {
        auto starts = [&](const char* prefix)
        { return locale.rfind(prefix, 0) == 0; };
        if (starts("ja"))
            return "ja";
        if (starts("ko"))
            return "ko";
        if (starts("zh-Hant") || starts("zh-TW") || starts("zh-HK") || starts("zh_TW") || starts("zh_HK"))
            return "zh-Hant";
        if (starts("zh"))
            return "zh-Hans";
        return "";
    }

    uint32_t be32(const unsigned char* p)
    {
        return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
    }

    uint16_t be16(const unsigned char* p)
    {
        return (uint16_t)(p[0] << 8 | p[1]);
    }

    bool readAt(FILE* file, long offset, void* buffer, size_t size)
    {
        return fseek(file, offset, SEEK_SET) == 0 && fread(buffer, 1, size, file) == size;
    }

    // The PostScript name of the font whose table directory is at `offset`
    std::string postScriptName(FILE* file, uint32_t offset)
    {
        unsigned char header[12];
        if (!readAt(file, offset, header, sizeof(header)))
            return "";
        uint16_t tables = be16(header + 4);
        for (uint16_t i = 0; i < tables; i++)
        {
            unsigned char record[16];
            if (!readAt(file, offset + 12 + 16 * i, record, sizeof(record)))
                return "";
            if (memcmp(record, "name", 4) != 0)
                continue;
            uint32_t length = be32(record + 12);
            if (length < 6 || length > (1 << 20))
                return "";
            std::vector<unsigned char> name(length);
            if (!readAt(file, be32(record + 8), name.data(), length))
                return "";
            uint16_t count = be16(&name[2]), strings = be16(&name[4]);
            for (uint16_t j = 0; j < count && 6 + 12 * (j + 1) <= length; j++)
            {
                const unsigned char* entry = &name[6 + 12 * j];
                uint16_t platform = be16(entry), id = be16(entry + 6), size = be16(entry + 8), at = be16(entry + 10);
                if (id != 6 || (size_t)strings + at + size > length)
                    continue;
                const unsigned char* text = &name[strings + at];
                std::string result;
                if (platform == 1)
                    result.assign((const char*)text, size);
                else // UTF-16BE, ASCII in a PostScript name
                    for (uint16_t k = 1; k < size; k += 2)
                        result += (char)text[k];
                return result;
            }
            return "";
        }
        return "";
    }

    // The face of a font file whose PostScript name is `name`, or -1
    [[maybe_unused]] int faceIndex(const std::string& path, const std::string& name)
    {
        FILE* file = fopen(path.c_str(), "rb");
        if (!file)
            return -1;
        int found = -1;
        unsigned char header[12];
        if (readAt(file, 0, header, sizeof(header)))
        {
            if (memcmp(header, "ttcf", 4) == 0)
            {
                uint32_t faces = be32(header + 8);
                for (uint32_t i = 0; i < faces && i < 64 && found < 0; i++)
                {
                    unsigned char offset[4];
                    if (readAt(file, 12 + 4 * i, offset, sizeof(offset)) && postScriptName(file, be32(offset)) == name)
                        found = (int)i;
                }
            }
            else if (postScriptName(file, 0) == name)
            {
                found = 0;
            }
        }
        fclose(file);
        return found;
    }

    // A file to look for: its face named `face`, or the first when empty
    struct Candidate
    {
        std::string path;
        std::string face;
    };

    [[maybe_unused]] bool findCandidate(const std::vector<Candidate>& candidates, DesktopFontLoader::FontFile* out)
    {
        for (auto& candidate : candidates)
        {
            if (access(candidate.path.c_str(), F_OK) == -1)
                continue;
            int index = candidate.face.empty() ? 0 : faceIndex(candidate.path, candidate.face);
            if (index < 0)
                continue;
            out->path  = candidate.path;
            out->index = index;
            return true;
        }
        return false;
    }

#if defined(__linux__) && !defined(ANDROID)
    // fontconfig, opened when there: a program that runs without it
    bool fontconfigMatch(const std::string& language, uint32_t sample, DesktopFontLoader::FontFile* out)
    {
        static void* library = dlopen("libfontconfig.so.1", RTLD_LAZY | RTLD_LOCAL);
        if (!library)
            return false;

        using InitLoadConfigAndFonts = void* (*)();
        using NameParse              = void* (*)(const unsigned char*);
        using ConfigSubstitute       = int (*)(void*, void*, int);
        using DefaultSubstitute      = void (*)(void*);
        using FontMatch              = void* (*)(void*, void*, int*);
        using PatternGetString       = int (*)(void*, const char*, int, unsigned char**);
        using PatternGetInteger      = int (*)(void*, const char*, int, int*);
        using PatternGetCharSet      = int (*)(void*, const char*, int, void**);
        using CharSetHasChar         = int (*)(void*, uint32_t);
        using PatternDestroy         = void (*)(void*);

        auto init       = (InitLoadConfigAndFonts)dlsym(library, "FcInitLoadConfigAndFonts");
        auto parse      = (NameParse)dlsym(library, "FcNameParse");
        auto substitute = (ConfigSubstitute)dlsym(library, "FcConfigSubstitute");
        auto defaults   = (DefaultSubstitute)dlsym(library, "FcDefaultSubstitute");
        auto match      = (FontMatch)dlsym(library, "FcFontMatch");
        auto getString  = (PatternGetString)dlsym(library, "FcPatternGetString");
        auto getInteger = (PatternGetInteger)dlsym(library, "FcPatternGetInteger");
        auto getCharSet = (PatternGetCharSet)dlsym(library, "FcPatternGetCharSet");
        auto hasChar    = (CharSetHasChar)dlsym(library, "FcCharSetHasChar");
        auto destroy    = (PatternDestroy)dlsym(library, "FcPatternDestroy");
        if (!init || !parse || !substitute || !defaults || !match || !getString || !getInteger || !getCharSet
            || !hasChar || !destroy)
            return false;

        static void* config = init();
        if (!config)
            return false;

        // fontconfig's names of the languages
        std::string query = "sans-serif:lang=";
        query += language == "zh-Hans" ? "zh-cn" : language == "zh-Hant" ? "zh-tw"
                                                                         : language;
        void* pattern = parse((const unsigned char*)query.c_str());
        if (!pattern)
            return false;
        substitute(config, pattern, 0); // FcMatchPattern
        defaults(pattern);
        int result = 0;
        void* font = match(config, pattern, &result);
        bool found = false;
        destroy(pattern);
        if (!font)
            return false;

        // The best match may not have the language at all (no such font
        // installed): only one with its characters is taken
        unsigned char* file = nullptr;
        void* charset       = nullptr;
        int index           = 0;
        if (getString(font, "file", 0, &file) == 0 && getCharSet(font, "charset", 0, &charset) == 0
            && hasChar(charset, sample))
        {
            getInteger(font, "index", 0, &index);
            out->path  = (const char*)file;
            out->index = index;
            found      = true;
        }
        destroy(font);
        return found;
    }
#endif

#ifdef __APPLE__
    bool coreTextFont(const char* name, DesktopFontLoader::FontFile* out)
    {
        CFStringRef wanted = CFStringCreateWithCString(nullptr, name, kCFStringEncodingUTF8);
        if (!wanted)
            return false;
        CTFontRef font = CTFontCreateWithName(wanted, 12.0, nullptr);
        CFRelease(wanted);
        if (!font)
            return false;

        // CoreText gives another font when it has not this one
        bool found        = false;
        char buffer[4096] = {};
        CFStringRef given = CTFontCopyPostScriptName(font);
        if (given && CFStringGetCString(given, buffer, sizeof(buffer), kCFStringEncodingUTF8) && strcmp(buffer, name) == 0)
        {
            CFURLRef url = (CFURLRef)CTFontCopyAttribute(font, kCTFontURLAttribute);
            if (url)
            {
                if (CFURLGetFileSystemRepresentation(url, true, (UInt8*)buffer, sizeof(buffer)))
                {
                    out->path  = buffer;
                    out->index = std::max(0, faceIndex(out->path, name));
                    found      = true;
                }
                CFRelease(url);
            }
        }
        if (given)
            CFRelease(given);
        CFRelease(font);
        return found;
    }
#endif

    // A file, or a resource built in ("@res/...")
    bool fontExists(const std::string& path)
    {
#ifdef USE_LIBROMFS
        if (path.rfind("@res/", 0) == 0)
        {
            try
            {
                return romfs::get(path.substr(5)).valid();
            }
            catch (...)
            {
                return false;
            }
        }
#endif
        return access(path.c_str(), F_OK) != -1;
    }

    // A file read once for every name it is loaded as: fontstash keeps
    // pointing into it
    std::vector<unsigned char>* readFontFile(const std::string& path)
    {
        static std::map<std::string, std::unique_ptr<std::vector<unsigned char>>> files;
        auto& data = files[path];
        if (data)
            return data.get();

        FILE* file = fopen(path.c_str(), "rb");
        if (!file)
            return nullptr;
        auto content = std::make_unique<std::vector<unsigned char>>();
        unsigned char buffer[65536];
        size_t read;
        while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0)
            content->insert(content->end(), buffer, buffer + read);
        fclose(file);
        if (content->empty())
            return nullptr;
        data = std::move(content);
        return data.get();
    }

} // namespace

DesktopFontLoader::FontFile DesktopFontLoader::findSystemFont(const std::string& language)
{
    FontFile file;
    uint32_t sample = 0;
    for (auto& cjk : cjkLanguages())
        if (language == cjk.language)
            sample = cjk.sample;
    if (sample == 0)
        return file;

#if defined(__APPLE__)
    // macOS and iOS
    const char* name = language == "ja" ? "HiraginoSans-W3"
        : language == "ko"              ? "AppleSDGothicNeo-Regular"
        : language == "zh-Hans"         ? "PingFangSC-Regular"
                                        : "PingFangTC-Regular";
    if (coreTextFont(name, &file))
        return file;
    std::vector<Candidate> candidates;
    if (language == "ko")
        candidates = { { "/System/Library/Fonts/AppleSDGothicNeo.ttc", "" } };
    else if (language == "ja")
        candidates = { { "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc", "" } };
    else
        candidates = { { "/System/Library/Fonts/PingFang.ttc", name },
            { "/System/Library/Fonts/STHeiti Light.ttc", language == "zh-Hans" ? "STHeitiSC-Light" : "STHeitiTC-Light" } };
    findCandidate(candidates, &file);
#elif defined(_WIN32)
    std::string fonts = "C:\\Windows\\Fonts\\";
    if (char* windows = getenv("systemroot"))
        fonts = std::string { windows } + "\\Fonts\\";
    std::vector<Candidate> candidates;
    if (language == "ja")
        candidates = { { fonts + "YuGothM.ttc", "" }, { fonts + "meiryo.ttc", "" }, { fonts + "msgothic.ttc", "" } };
    else if (language == "ko")
        candidates = { { fonts + "malgun.ttf", "" }, { fonts + "gulim.ttc", "" } };
    else if (language == "zh-Hans")
        candidates = { { fonts + "msyh.ttc", "" }, { fonts + "simsun.ttc", "" } };
    else
        candidates = { { fonts + "msjh.ttc", "" }, { fonts + "mingliu.ttc", "" } };
    findCandidate(candidates, &file);
#elif defined(ANDROID)
    const std::string noto = language == "ja" ? "NotoSansCJKjp-Regular"
        : language == "ko"                    ? "NotoSansCJKkr-Regular"
        : language == "zh-Hans"               ? "NotoSansCJKsc-Regular"
                                              : "NotoSansCJKtc-Regular";
    findCandidate({ { "/system/fonts/NotoSansCJK-Regular.ttc", noto }, { "/system/fonts/NotoSansCJK-Regular.ttc", "" },
                      { "/system/fonts/DroidSansFallback.ttf", "" }, { "/system/fonts/DroidSansFallbackFull.ttf", "" } },
        &file);
#elif defined(__linux__)
    if (fontconfigMatch(language, sample, &file))
        return file;
    // Without fontconfig: where distributions put Noto CJK
    const std::string noto = language == "ja" ? "NotoSansCJKjp-Regular"
        : language == "ko"                    ? "NotoSansCJKkr-Regular"
        : language == "zh-Hans"               ? "NotoSansCJKsc-Regular"
                                              : "NotoSansCJKtc-Regular";
    std::vector<Candidate> candidates;
    for (const char* folder : { "/usr/share/fonts/opentype/noto/", "/usr/share/fonts/noto-cjk/",
             "/usr/share/fonts/google-noto-sans-cjk-fonts/", "/usr/share/fonts/google-noto-cjk/",
             "/usr/share/fonts/OTF/" })
        candidates.push_back({ std::string(folder) + "NotoSansCJK-Regular.ttc", noto });
    findCandidate(candidates, &file);
#else
    (void)sample;
#endif
    return file;
}

bool DesktopFontLoader::loadFont(const std::string& name, const std::string& path)
{
    return !path.empty() && fontExists(path) && Application::loadFontFromFile(name, path);
}

bool DesktopFontLoader::loadFontFile(const std::string& name, const FontFile& file)
{
    std::vector<unsigned char>* data = readFontFile(file.path);
    return data && Application::loadFontFromMemory(name, data->data(), data->size(), false, file.index);
}

bool DesktopFontLoader::loadAppCjkFont(NVGcontext* vg)
{
    for (auto& ext : fontExts)
    {
        std::string path = std::string(APP_CJK_FONT_PATH) + ext;
        void* data       = nullptr;
        size_t size      = 0;
#ifdef USE_LIBROMFS
        if (path.rfind("@res/", 0) == 0)
        {
            try
            {
                auto& font = romfs::get(path.substr(5));
                if (font.valid())
                {
                    data = (void*)font.data();
                    size = font.size();
                }
            }
            catch (...)
            {
            }
        }
        else
#endif
            if (access(path.c_str(), F_OK) != -1)
        {
            if (std::vector<unsigned char>* file = readFontFile(path))
            {
                data = file->data();
                size = file->size();
            }
        }
        if (!data)
            continue;

        // One font for the four languages, read once
        bool loaded = false;
        for (auto& cjk : cjkLanguages())
        {
            if (!Application::loadFontFromMemory(cjk.fontName, data, size, false))
                continue;
            if (!loaded)
                nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(cjk.fontName));
            loaded = true;
        }
        if (loaded)
            brls::Logger::info("Using the app's font for Chinese, Japanese and Korean: {}", path);
        return loaded;
    }
    return false;
}

void DesktopFontLoader::loadSystemCjkFonts(NVGcontext* vg)
{
    // The fallbacks are looked up in order: the locale's language first, so
    // that the characters the four share are drawn as it writes them
    std::vector<const CjkLanguage*> order;
    std::string first = cjkLanguageOf(Application::getLocale());
    for (auto& cjk : cjkLanguages())
        if (first == cjk.language)
            order.push_back(&cjk);
    for (auto& cjk : cjkLanguages())
        if (first != cjk.language)
            order.push_back(&cjk);

    std::vector<std::pair<std::string, int>> added;
    for (auto* cjk : order)
    {
        FontFile file = findSystemFont(cjk->language);
        if (file.path.empty())
        {
            brls::Logger::info("No system font for {}", cjk->language);
            continue;
        }
        if (!loadFontFile(cjk->fontName, file))
        {
            brls::Logger::warning("Could not load the {} font {}", cjk->language, file.path);
            continue;
        }
        brls::Logger::info("Using {} font: {} ({})", cjk->language, file.path, file.index);
        // The same face for two languages: looked up once
        std::pair<std::string, int> face { file.path, file.index };
        if (std::find(added.begin(), added.end(), face) != added.end())
            continue;
        added.push_back(face);
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(cjk->fontName));
    }
}

void DesktopFontLoader::loadFonts()
{
    NVGcontext* vg = brls::Application::getNVGContext();

    // Text font
    if (loadFont(FONT_REGULAR, USER_FONT_PATH))
    {
        // Using internal font as fallback
        if (loadFont("default", INTER_FONT_PATH))
        {
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("default"));
        }
    }
    else
    {
        brls::Logger::info("No custom font (searched at: {}), using the internal one: {}", USER_FONT_PATH, INTER_FONT_PATH);
        if (!loadFont(FONT_REGULAR, INTER_FONT_PATH))
        {
            Logger::warning("Failed to load internal font, text may not be displayed");
        }
    }

    // The buttons, before the other fallbacks: their code points are in the
    // private use area, where other fonts may draw something else
    if (loadFont(FONT_SWITCH_ICONS, USER_ICON_PATH))
    {
        // User-provided icons
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
    }
    else
    {
        brls::Logger::info("No custom icons (searched at: {}), using the internal ones: {}", USER_ICON_PATH, INTER_ICON_PATH);
        if (loadFont(FONT_SWITCH_ICONS, INTER_ICON_PATH))
        {
            // Internal icons
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
        }
        else
        {
            Logger::warning("Failed to load internal icons, bottom bar icons may not be displayed");
        }
    }

    // Chinese, Japanese and Korean: the app's font, else the system's
    if (!loadAppCjkFont(vg) && useSystemFonts)
        loadSystemCjkFonts(vg);

    // Load Emoji
    if (loadFont(FONT_EMOJI, USER_EMOJI_PATH))
    {
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_EMOJI));
    }
#ifdef BRLS_USE_FREETYPE
    // FreeType draws colour emoji: the system's font, when the app gives none
    else if (USER_EMOJI_PATH.empty() && useSystemFonts)
    {
#if defined(__APPLE__) && !defined(IOS)
        std::vector<std::string> emojiFonts = { "/System/Library/Fonts/Apple Color Emoji.ttc" };
#elif defined(_WIN32)
        std::string prefix = "C:\\Windows\\Fonts\\";
        if (char* winDir = getenv("systemroot"))
            prefix = std::string { winDir } + "\\Fonts\\";
        std::vector<std::string> emojiFonts = { prefix + "seguiemj.ttf" };
#elif defined(ANDROID)
        std::vector<std::string> emojiFonts = { "/system/fonts/NotoColorEmoji.ttf" };
#elif defined(__linux__)
        std::vector<std::string> emojiFonts = {
            "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf",
            "/usr/share/fonts/noto/NotoColorEmoji.ttf",
            "/usr/share/fonts/google-noto-emoji/NotoColorEmoji.ttf",
            "/usr/share/fonts/TTF/NotoColorEmoji.ttf",
        };
#else
        std::vector<std::string> emojiFonts;
#endif
        for (auto& path : emojiFonts)
        {
            if (loadFont(FONT_EMOJI, path))
            {
                nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_EMOJI));
                brls::Logger::info("Using emoji font: {}", path);
                break;
            }
        }
    }
#endif

    // Material icons
    if (this->loadMaterialFromResources())
    {
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_MATERIAL_ICONS));
    }
    else
    {
        Logger::error("desktop: could not load Material icons font from resources");
    }
}

} // namespace brls
