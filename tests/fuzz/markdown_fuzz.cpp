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

// libFuzzer target (-DBRLS_BUILD_FUZZERS=ON, clang): Cadran's Markdown and
// safe_text on any bytes, as GitHub sends them. Beyond the sanitizers: the
// bounds markdown.hpp promises, every string valid UTF-8, and clean() within
// its size and stable when run again.
//
//   ./aurore_fuzz_markdown -max_total_time=60

#include <aurore/cadran/util/markdown.hpp>
#include <aurore/cadran/util/safe_text.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

// No stray continuation byte, overlong form, surrogate, nothing past
// U+10FFFF, no sequence cut at the end
static bool utf8(const std::string& s)
{
    for (size_t i = 0; i < s.size();)
    {
        unsigned char c = (unsigned char)s[i];
        size_t len;
        uint32_t cp, min;
        if (c < 0x80)
        {
            i++;
            continue;
        }
        else if ((c & 0xE0) == 0xC0)
            len = 2, cp = c & 0x1F, min = 0x80;
        else if ((c & 0xF0) == 0xE0)
            len = 3, cp = c & 0x0F, min = 0x800;
        else if ((c & 0xF8) == 0xF0)
            len = 4, cp = c & 0x07, min = 0x10000;
        else
            return false;
        if (i + len > s.size())
            return false;
        for (size_t k = 1; k < len; k++)
        {
            unsigned char d = (unsigned char)s[i + k];
            if ((d & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (d & 0x3F);
        }
        if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        i += len;
    }
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const std::string in(reinterpret_cast<const char*>(data), size);

    for (bool multiline : { true, false })
    {
        const std::string c = safe_text::clean(in, 4096, multiline);
        FUZZ_ASSERT(c.size() <= 4096);
        FUZZ_ASSERT(utf8(c));
        FUZZ_ASSERT(multiline || c.find('\n') == std::string::npos);
        FUZZ_ASSERT(safe_text::clean(c, 4096, multiline) == c);
    }

    using markdown::Block;
    const std::vector<Block> blocks = markdown::parse(in);
    FUZZ_ASSERT(blocks.size() <= markdown::MAX_BLOCKS);
    for (const Block& b : blocks)
    {
        FUZZ_ASSERT(utf8(b.text) && utf8(b.alert) && utf8(b.icon));
        FUZZ_ASSERT(b.icon.size() <= 8);
        if (b.kind == Block::Heading)
            FUZZ_ASSERT(b.level >= 1 && b.level <= 6);
        if (b.kind == Block::Item)
            FUZZ_ASSERT(b.level >= 0 && b.level <= markdown::MAX_DEPTH);
        FUZZ_ASSERT(b.task >= -1 && b.task <= 1);
        // MAX_CODE_LINES lines, then one "…" saying the rest was cut
        FUZZ_ASSERT(b.lines.size() <= markdown::MAX_CODE_LINES + 1);
        for (const std::string& l : b.lines)
            FUZZ_ASSERT(utf8(l));
        // The header row, then MAX_ROWS rows
        FUZZ_ASSERT(b.rows.size() <= markdown::MAX_ROWS + 1);
        for (const auto& row : b.rows)
        {
            FUZZ_ASSERT(row.size() <= markdown::MAX_COLUMNS);
            for (const std::string& cell : row)
                FUZZ_ASSERT(utf8(cell));
        }
    }
    FUZZ_ASSERT(utf8(markdown::inline_text(safe_text::clean(in.substr(0, 1024), 1024, false))));
    return 0;
}
