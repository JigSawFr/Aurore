// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include <cstdlib>
#include <vector>
#include <aurore/core/safe_area.hpp>

namespace aurore
{

// The words of `text` between separators (`|`, commas, spaces)
static std::vector<std::string> words(const std::string& text)
{
    std::vector<std::string> result;
    std::string word;
    for (char c : text)
    {
        if (c == '|' || c == ',' || c == ' ' || c == '\t')
        {
            if (!word.empty())
                result.push_back(word);
            word.clear();
        }
        else
            word += c;
    }
    if (!word.empty())
        result.push_back(word);
    return result;
}

bool Insets::parse(const std::string& text, Insets* out)
{
    std::vector<float> values;
    for (const std::string& word : words(text))
    {
        char* end     = nullptr;
        const float v = std::strtof(word.c_str(), &end);
        if (end == word.c_str() || *end != '\0' || !std::isfinite(v) || v < 0)
            return false;
        values.push_back(v);
    }
    Insets insets;
    switch (values.size())
    {
        case 1:
            insets = { values[0], values[0], values[0], values[0] };
            break;
        case 2:
            insets = { values[0], values[1], values[0], values[1] };
            break;
        case 3:
            insets = { values[0], values[1], values[2], values[1] };
            break;
        case 4:
            insets = { values[0], values[1], values[2], values[3] };
            break;
        default:
            return false;
    }
    *out = insets;
    return true;
}

bool parseSafeAreaEdges(const std::string& text, SafeAreaEdges* out)
{
    std::vector<std::string> names = words(text);
    if (names.empty())
        return false;
    SafeAreaEdges edges = SafeAreaEdges::NONE;
    for (const std::string& name : names)
    {
        if (name == "auto")
        {
            if (names.size() != 1)
                return false;
            edges = SafeAreaEdges::AUTO;
        }
        else if (name == "none")
            continue;
        else if (name == "top")
            edges = edges | SafeAreaEdges::TOP;
        else if (name == "right")
            edges = edges | SafeAreaEdges::RIGHT;
        else if (name == "bottom")
            edges = edges | SafeAreaEdges::BOTTOM;
        else if (name == "left")
            edges = edges | SafeAreaEdges::LEFT;
        else if (name == "vertical")
            edges = edges | SafeAreaEdges::VERTICAL;
        else if (name == "horizontal")
            edges = edges | SafeAreaEdges::HORIZONTAL;
        else if (name == "all")
            edges = edges | SafeAreaEdges::ALL;
        else
            return false;
    }
    *out = edges;
    return true;
}

} // namespace aurore
