// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// libFuzzer target (-DAURORE_BUILD_FUZZERS=ON, clang): a theme file on any
// bytes, as an app reads one from a folder its user fills (docs/themes.md).
// Read from an empty folder of its own, so that what it includes or inherits
// is refused or not found, never a file of the machine's. Beyond the
// sanitizers: the variant it gives is one of the two, and a colour it reads
// has its four components between 0 and 1.
//
//   ./aurore_fuzz_theme -max_total_time=60 corpus resources/themes

#include <aurore/core/logger.hpp>
#include <aurore/core/theme.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#define FUZZ_ASSERT(expr)                                                                    \
    do                                                                                       \
    {                                                                                        \
        if (!(expr))                                                                         \
        {                                                                                    \
            std::fprintf(stderr, "%s:%d: property failed: %s\n", __FILE__, __LINE__, #expr); \
            std::abort();                                                                    \
        }                                                                                    \
    } while (0)

static bool unit(float v)
{
    return v >= 0.0f && v <= 1.0f;
}

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    // A line it cannot read is logged as a warning: millions of them
    aurore::Logger::setLogLevel(aurore::LogLevel::LOG_ERROR);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    static const std::string folder = []
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() / "aurore_fuzz_theme";
        std::filesystem::create_directories(path);
        return path.string();
    }();
    const std::string in(reinterpret_cast<const char*>(data), size);

    aurore::ThemeVariant variant = aurore::ThemeVariant::LIGHT;
    (void)aurore::Theme::load(in, &variant, folder); // any input: refused or taken, never a crash
    FUZZ_ASSERT(variant == aurore::ThemeVariant::LIGHT || variant == aurore::ThemeVariant::DARK);
    aurore::Theme::reset();

    NVGcolor color;
    if (aurore::Theme::parseColor(in.substr(0, 64), &color))
        FUZZ_ASSERT(unit(color.r) && unit(color.g) && unit(color.b) && unit(color.a));
    return 0;
}
