// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/view/markdown_view.hpp>

#include <sys/stat.h>
#include <string>

#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#endif

#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/util/text.hpp>

using namespace aurore::literals;

namespace markdown_view
{
namespace kit = cadran::kit;

namespace
{
    // A label that takes the rest of its row and wraps there.
    aurore::Label* growing(const std::string& text, float size, const char* color_key)
    {
        aurore::Label* l = kit::label(text, size, color_key);
        l->setGrow(1.0f);
        l->setShrink(1.0f);
        return l;
    }

    // Whether the resources hold `path` ("img/…").
    bool asset_exists(const std::string& path)
    {
#ifdef USE_LIBROMFS
        try
        {
            return romfs::get(path).valid();
        }
        catch (const std::exception&)
        {
            return false;
        }
#else
        struct stat st;
        return stat((std::string(AURORE_RESOURCES) + path).c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
    }

    // The emoji `hex` from the Twemoji set in img/emoji/, `size` wide; null
    // when it is not one of them. Only hex digits reach the path.
    aurore::Image* icon(const std::string& hex, float size)
    {
        if (hex.empty() || hex.size() > 6 || hex.find_first_not_of("0123456789abcdef") != std::string::npos)
            return nullptr;
        if (!asset_exists("img/emoji/" + hex + ".png"))
            return nullptr;
        auto* image = new aurore::Image();
        image->setScalingType(aurore::ImageScalingType::FIT);
        image->setImageFromRes("img/emoji/" + hex + ".png");
        image->setWidth(size);
        image->setHeight(size);
        image->setShrink(0.0f);
        return image;
    }

    // A heading as the boards': the first two levels in the display face, the
    // third as a kicker (capitals, second ink), the others as a line's text.
    aurore::View* heading(const markdown::Block& b, bool first)
    {
        const int level = b.level >= 1 && b.level <= 6 ? b.level : 6;
        auto* row       = new aurore::Box(aurore::Axis::ROW);
        row->setAlignItems(aurore::AlignItems::CENTER);
        row->setMarginTop(first ? 0 : aurore::dp(level <= 2 ? 26 : 18));
        row->setMarginBottom(aurore::dp(level <= 2 ? 10 : 6));
        const float size = level == 1 ? 22 : level == 2 ? 20
            : level == 3                                ? kit::TEXT_KICKER
                                                        : 18;
        if (aurore::Image* image = icon(b.icon, size + 4))
        {
            image->setMarginRight(aurore::dp(10));
            row->addView(image);
        }
        aurore::Label* text = level <= 2 ? kit::display(b.text, size)
            : level == 3                 ? kit::label(cadran::upper(b.text), size, "cadran/text2")
                                         : kit::label(b.text, size, "cadran/text");
        text->setGrow(1.0f);
        text->setShrink(1.0f);
        row->addView(text);
        return row;
    }

    // A task's box: ticked (filled, a white mark inside) or not (an outline).
    aurore::View* checkbox(bool done)
    {
        auto* box = new aurore::Box(aurore::Axis::ROW);
        box->setWidth(aurore::dp(20));
        box->setHeight(aurore::dp(20));
        box->setShrink(0.0f);
        box->setCornerRadius(aurore::dp(4));
        box->setBorderThickness(2);
        box->setThemeColor("borderColor", done ? "cadran/teal_fill" : "cadran/text3");
        box->setJustifyContent(aurore::JustifyContent::CENTER);
        box->setAlignItems(aurore::AlignItems::CENTER);
        box->setMarginTop(aurore::dp(4));
        if (done)
        {
            box->setThemeColor("backgroundColor", "cadran/teal_fill");
            auto* mark = new aurore::Rectangle();
            mark->setThemeColor("color", "cadran/navy");
            mark->setWidth(aurore::dp(8));
            mark->setHeight(aurore::dp(8));
            mark->setCornerRadius(aurore::dp(2));
            box->addView(mark);
        }
        return box;
    }

    aurore::View* item(const markdown::Block& b)
    {
        auto* row = new aurore::Box(aurore::Axis::ROW);
        row->setAlignItems(aurore::AlignItems::FLEX_START);
        row->setMarginLeft(aurore::dp(8 + 26 * b.level));
        row->setMarginTop(aurore::dp(6));
        aurore::View* marker = nullptr;
        if (b.task >= 0)
            marker = checkbox(b.task == 1);
        else if (b.number > 0)
            marker = kit::label(std::to_string(b.number) + ".", 18, "cadran/teal");
        else if (!(marker = icon(b.icon, 20)))
            marker = kit::label(b.level == 0 ? "•" : "–", 18, "cadran/teal");
        marker->setMinWidth(aurore::dp(22));
        marker->setMarginRight(aurore::dp(10));
        marker->setShrink(0.0f);
        row->addView(marker);
        // A task done reads quieter, as GitHub greys it.
        row->addView(growing(b.text, 18, b.task == 1 ? "cadran/text3" : "cadran/text"));
        return row;
    }

    aurore::View* code(const markdown::Block& b)
    {
        auto* box = new aurore::Box(aurore::Axis::COLUMN);
        box->setThemeColor("backgroundColor", "cadran/surf2");
        box->setThemeColor("borderColor", "cadran/line");
        box->setBorderThickness(1);
        box->setCornerRadius(aurore::dp(kit::RADIUS_ROW));
        box->setPadding(aurore::dp(12), aurore::dp(18), aurore::dp(12), aurore::dp(18));
        box->setMarginTop(aurore::dp(8));
        box->setMarginBottom(aurore::dp(4));
        box->addView(kit::label(text::join(b.lines, "\n"), kit::TEXT_BODY, "cadran/text"));
        return box;
    }

    aurore::View* quote(const markdown::Block& b)
    {
        // GitHub's alerts: their colour and their title, on the second surface
        // as the boards' notes.
        const char* color = "cadran/text3";
        std::string title;
        if (b.alert == "note")
            color = "cadran/blue", title = "cadran/markdown/note"_i18n;
        else if (b.alert == "tip")
            color = "cadran/teal", title = "cadran/markdown/tip"_i18n;
        else if (b.alert == "important")
            color = "cadran/violet", title = "cadran/markdown/important"_i18n;
        else if (b.alert == "warning")
            color = "cadran/amber", title = "cadran/markdown/warning"_i18n;
        else if (b.alert == "caution")
            color = "cadran/coral", title = "cadran/markdown/caution"_i18n;
        auto* row = new aurore::Box(aurore::Axis::ROW);
        row->setAlignItems(aurore::AlignItems::STRETCH);
        row->setMarginTop(aurore::dp(8));
        if (!title.empty())
        {
            row->setThemeColor("backgroundColor", "cadran/surf2");
            row->setCornerRadius(aurore::dp(kit::RADIUS_ROW));
            row->setPaddingLeft(aurore::dp(10));
        }
        auto* bar = new aurore::Rectangle();
        bar->setThemeColor("color", color);
        bar->setWidth(aurore::dp(4));
        bar->setCornerRadius(aurore::dp(2));
        bar->setMarginRight(aurore::dp(14));
        row->addView(bar);
        auto* body = new aurore::Box(aurore::Axis::COLUMN);
        body->setGrow(1.0f);
        body->setShrink(1.0f);
        body->setPadding(aurore::dp(6), aurore::dp(8), aurore::dp(6), 0);
        if (!title.empty())
        {
            aurore::Label* t = kit::label(cadran::upper(title), kit::TEXT_META, color);
            t->setMarginBottom(aurore::dp(4));
            body->addView(t);
        }
        if (!b.text.empty())
            body->addView(kit::label(b.text, 18, title.empty() ? "cadran/text2" : "cadran/text"));
        row->addView(body);
        return row;
    }

    aurore::View* table(const markdown::Block& b)
    {
        size_t columns = 0;
        for (const auto& r : b.rows)
            columns = std::max(columns, r.size());
        auto* grid = new aurore::Box(aurore::Axis::COLUMN);
        grid->setMarginTop(aurore::dp(8));
        grid->setBorderThickness(1);
        grid->setThemeColor("borderColor", "cadran/line");
        grid->setCornerRadius(aurore::dp(kit::RADIUS_ROW));
        grid->setThemeColor("backgroundColor", "cadran/surf");
        for (size_t r = 0; r < b.rows.size(); r++)
        {
            auto* row = new aurore::Box(aurore::Axis::ROW);
            row->setAlignItems(aurore::AlignItems::STRETCH);
            if (r + 1 < b.rows.size())
            {
                row->setLineBottom(1);
                row->setThemeColor("lineColor", "cadran/line");
            }
            for (size_t c = 0; c < columns; c++)
            {
                const std::string text = c < b.rows[r].size() ? b.rows[r][c] : "";
                auto* cell             = new aurore::Box(aurore::Axis::ROW);
                cell->setWidthPercentage(100.0f / (float)columns);
                cell->setPadding(aurore::dp(8), aurore::dp(14), aurore::dp(8), aurore::dp(14));
                cell->addView(growing(text, kit::TEXT_BODY, r == 0 ? "cadran/text2" : "cadran/text"));
                row->addView(cell);
            }
            grid->addView(row);
        }
        return grid;
    }
} // namespace

void fill(aurore::Box* box, const std::vector<markdown::Block>& blocks)
{
    bool first = true;
    for (const markdown::Block& b : blocks)
    {
        aurore::View* view = nullptr;
        switch (b.kind)
        {
            case markdown::Block::Heading:
                view = heading(b, first);
                break;
            case markdown::Block::Paragraph:
                view = kit::label(b.text, 18, "cadran/text");
                view->setMarginTop(aurore::dp(8));
                break;
            case markdown::Block::Item:
                view = item(b);
                break;
            case markdown::Block::Code:
                view = code(b);
                break;
            case markdown::Block::Quote:
                view = quote(b);
                break;
            case markdown::Block::Rule:
            {
                auto* rule = new aurore::Rectangle();
                rule->setThemeColor("color", "cadran/line");
                rule->setHeight(1);
                rule->setMarginTop(aurore::dp(14));
                rule->setMarginBottom(aurore::dp(6));
                view = rule;
                break;
            }
            case markdown::Block::Table:
                view = table(b);
                break;
        }
        box->addView(view);
        first = false;
    }
}

} // namespace markdown_view
