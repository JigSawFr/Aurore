// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/bidi.hpp>

#ifdef AURORE_USE_FRIBIDI
#include <fribidi.h>

#include <vector>
#endif

namespace aurore
{

#ifdef AURORE_USE_FRIBIDI

// A lead byte of U+0590…U+07FF (Hebrew, Arabic, Syriac, Thaana, N'Ko) or of
// U+FB1D…U+FEFF (their presentation forms): a cheap test before fribidi.
static bool mayHaveRtl(const std::string& text)
{
    for (size_t i = 0; i < text.size(); i++)
    {
        unsigned char c = (unsigned char)text[i];
        if (c >= 0xD6 && c <= 0xDF)
            return true;
        if (c == 0xEF && i + 1 < text.size() && (unsigned char)text[i + 1] >= 0xAC && (unsigned char)text[i + 1] <= 0xBB)
            return true;
    }
    return false;
}

static std::string visualLine(const std::string& line)
{
    if (line.empty())
        return line;

    std::vector<FriBidiChar> logical(line.size() + 1);
    FriBidiStrIndex length = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, line.c_str(), (FriBidiStrIndex)line.size(), logical.data());
    std::vector<FriBidiChar> visual(length + 1);
    FriBidiParType base = FRIBIDI_PAR_ON; // from the first strong letter
    if (!fribidi_log2vis(logical.data(), length, &base, visual.data(), nullptr, nullptr, nullptr))
        return line;

    // fribidi_log2vis() joins Arabic letters into presentation forms
    std::vector<char> out(length * 4 + 1);
    FriBidiStrIndex size = fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, visual.data(), length, out.data());
    return std::string(out.data(), size);
}

std::string bidiVisual(const std::string& text)
{
    if (!mayHaveRtl(text))
        return text;

    std::string result;
    size_t start = 0;
    while (true)
    {
        size_t end = text.find('\n', start);
        result += visualLine(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos)
            break;
        result += '\n';
        start = end + 1;
    }
    return result;
}

bool bidiAvailable()
{
    return true;
}

#else

std::string bidiVisual(const std::string& text)
{
    return text;
}

bool bidiAvailable()
{
    return false;
}

#endif

} // namespace aurore
