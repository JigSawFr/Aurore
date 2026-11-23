// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// The sheets of ui.hpp: pick_actions(), pick() and pick_marked() in the
// Cadran look. A header (an image when there is one, the title, what it is
// about now), then the lines in cards, a section's kicker above each group;
// each line its icon in a soft square (or a swatch of colours), its label
// and, at its end, what is set now, a switch, or the check of the value
// chosen; removals in coral.
// It sits where Aurore's Dropdown did (880 px wide, above the hints), over
// the screen it was opened from, as tall as its lines (the rest scrolls);
// B, or a touch outside it, closes it.
#include <yoga/Yoga.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <type_traits>

#include <aurore/cadran/ui/action_sheet.hpp>
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/cadran_modal.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/view/cadran_icon.hpp>

using namespace aurore::literals;

namespace ui
{

void close_sheet(std::function<void()> then)
{
    aurore::Application::popActivity(aurore::TransitionAnimation::FADE, [then]()
        {
        if (then) aurore::sync(then); });
}

aurore::Box* sheet_outside(float width, bool grow)
{
    auto* box = new aurore::Box();
    if (grow)
        box->setGrow(1);
    else
        box->setWidth(aurore::dp(width));
    box->setShrink(0);
    box->addGestureRecognizer(new aurore::TapGestureRecognizer(box, []()
        { close_sheet(nullptr); }));
    return box;
}

namespace
{
    namespace kit = cadran::kit;

    constexpr float SIDE        = 200; // the panel's margins: 880 px wide
    constexpr float CARD_GAP    = 14;
    constexpr float TILE_HEIGHT = 116; // a grid's tiles (pick_grid())
    constexpr float TILE_GAP    = 12;

    struct Line
    {
        std::string label, detail, group;
        std::string icon; // a Cadran icon's name; empty: none
        std::string image; // a picture in the icon's place (pick()'s `icons`)
        std::optional<Swatch> swatch; // colours in the icon's place (pick()'s `swatches`)
        std::shared_ptr<const std::vector<unsigned char>> picture; // a picture held in memory (PickOptions)
        Choice::Tone tone = Choice::Tone::PLAIN;
        int toggle        = -1; // a switch: 0 off, 1 on; -1 none
        bool checked      = false; // the value chosen (pick())
        std::function<void()> run;
    };

    // The lines' column: once laid out, its height sizes the panel.
    class Column : public aurore::Box
    {
      public:
        Column()
            : aurore::Box(aurore::Axis::COLUMN)
        {
        }
        std::function<void(float)> measured;
        void onLayout() override
        {
            aurore::Box::onLayout();
            if (this->measured && this->getHeight() > 0)
                this->measured(this->getHeight());
        }
    };

    // The panel: its list's foot fades into it while more lines follow.
    class Panel : public aurore::Box
    {
      public:
        Panel()
            : aurore::Box(aurore::Axis::COLUMN)
        {
        }
        aurore::ScrollingFrame* frame = nullptr;

        void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
            aurore::FrameContext* ctx) override
        {
            aurore::Box::draw(vg, x, y, width, height, style, ctx);
            if (this->frame)
                kit::draw_more_below(vg, this->frame, this->frame->getX(), this->frame->getY(), this->frame->getWidth(),
                    this->frame->getHeight(), "cadran/surf", cadran::PICKER_CORNER);
        }
    };

    class Sheet : public aurore::Box
    {
      public:
        Sheet(const std::string& title, const std::string& subtitle, int image, std::vector<Line> choices, size_t first,
            int columns = 0)
            : aurore::Box(aurore::Axis::COLUMN)
            , lines(std::move(choices))
            , focus(first)
        {
            this->setId("cadran_sheet");
            // Said when the focus enters it: its title, then its row.
            this->setAccessibilityRole(aurore::AccessibilityRole::LIST);
            this->setAccessibilityLabel(subtitle.empty() ? title : title + ", " + subtitle);
            this->setThemeColor("backgroundColor", "aurore/backdrop");
            this->setAlignItems(aurore::AlignItems::STRETCH);
            this->addView(sheet_outside(0, true));

            auto* panel = new Panel();
            panel->setGrow(1);
            panel->setCornerRadius(aurore::dp(cadran::PICKER_CORNER));
            panel->setThemeColor("backgroundColor", "cadran/surf");
            panel->setThemeColor("borderColor", "cadran/line");
            panel->setBorderThickness(1);
            panel->setAlignItems(aurore::AlignItems::STRETCH);
            panel->setThemeArea(aurore::ThemeArea::DIALOG); // backdrop.dialog, and over a theme's picture
            aurore::Box* head = this->header(title, subtitle, image);
            panel->addView(head);

            auto* frame = new aurore::ScrollingFrame();
            frame->setGrow(1);
            frame->setScrollingBehavior(aurore::ScrollingBehavior::CENTERED);
            auto* column = new Column();
            column->setAlignItems(aurore::AlignItems::STRETCH);
            column->setPadding(aurore::dp(18), aurore::dp(28), aurore::dp(22), aurore::dp(28));
            if (columns > 0)
                this->grid(column, (size_t)columns);
            else
            {
                cadran::Card* card = nullptr;
                for (size_t i = 0; i < this->lines.size(); i++)
                {
                    const Line& l = this->lines[i];
                    // A new group: its kicker (when named) and its card.
                    if (!card || l.group != this->lines[i - 1].group)
                    {
                        if (!l.group.empty())
                        {
                            auto* kicker = new aurore::Label();
                            kit::set_kicker(kicker, l.group);
                            kicker->setSingleLine(true);
                            kicker->setMarginLeft(aurore::dp(6));
                            kicker->setMarginTop(card ? aurore::dp(CARD_GAP + 4) : 0);
                            kicker->setMarginBottom(aurore::dp(10));
                            column->addView(kicker);
                        }
                        card = new cadran::Card();
                        if (l.group.empty() && !column->getChildren().empty())
                            card->setMarginTop(aurore::dp(CARD_GAP));
                        column->addView(card);
                    }
                    card->addView(this->line(l, i));
                }
            }
            frame->setContentView(column);
            panel->frame = frame;
            panel->addView(frame);

            // As tall as its lines, up to most of the screen; the rest scrolls.
            // A first guess, then the lines' own height once laid out.
            const float most = aurore::Application::contentHeight * 0.8f;
            const float body = columns > 0
                ? aurore::dp(TILE_HEIGHT + TILE_GAP) * (float)((this->lines.size() + columns - 1) / columns)
                : aurore::dp(60.0f) * (float)this->lines.size();
            panel->setHeight(std::min(aurore::dp(subtitle.empty() ? 70.0f : 92.0f) + body + aurore::dp(40), most));
            column->measured = [panel, head, most](float lines_height)
            {
                const float want = std::min(head->getHeight() + lines_height + 2, most);
                if (std::fabs(want - panel->getHeight()) > 1)
                    aurore::sync([panel, want]()
                        { panel->setHeight(want); });
            };

            auto* row = new aurore::Box(aurore::Axis::ROW);
            row->setAlignItems(aurore::AlignItems::STRETCH);
            row->setShrink(0);
            // On a compact screen the panel takes the width, its lines whole
            const float side = aurore::Application::getWidthClass() == aurore::WidthClass::COMPACT ? 12 : SIDE;
            row->addView(sheet_outside(side, false));
            row->addView(panel);
            row->addView(sheet_outside(side, false));
            this->addView(row);
            this->addView(sheet_outside(20, false)); // the gap above the hints
            this->addView(new aurore::BottomBar());

            this->registerAction("hints/back"_i18n, aurore::BUTTON_B, [](aurore::View*)
                {
            close_sheet(nullptr);
            return true; }, false, false, aurore::SOUND_BACK);
        }

        bool isTranslucent() override { return true; }

        aurore::View* getDefaultFocus() override
        {
            if (this->focus < this->cells.size())
                return this->cells[this->focus];
            return aurore::Box::getDefaultFocus();
        }

      private:
        std::vector<Line> lines;
        size_t focus;
        std::vector<aurore::View*> cells;

        aurore::Box* header(const std::string& title, const std::string& subtitle, int image)
        {
            auto* head = new aurore::Box(aurore::Axis::ROW);
            head->setAlignItems(aurore::AlignItems::CENTER);
            head->setPadding(aurore::dp(subtitle.empty() ? 24 : 20), aurore::dp(36), aurore::dp(subtitle.empty() ? 20 : 18), aurore::dp(image ? 28 : 36));
            head->setThemeColor("lineColor", "cadran/line");
            head->setLineBottom(1);
            head->setShrink(0);
            if (image)
            {
                auto* icon = new aurore::Image();
                icon->setWidth(aurore::dp(48));
                icon->setHeight(aurore::dp(48));
                icon->setCornerRadius(aurore::dp(10));
                icon->setScalingType(aurore::ImageScalingType::FIT);
                icon->innerSetImage(image);
                icon->setFreeTexture(false); // the list's, or the icon cache's
                icon->setMarginRight(aurore::dp(16));
                icon->setShrink(0);
                head->addView(icon);
            }
            auto* words = new aurore::Box(aurore::Axis::COLUMN);
            words->setGrow(1);
            words->setShrink(1);
            aurore::Label* name = kit::label(title, 22, "cadran/text");
            name->setSingleLine(true);
            words->addView(name);
            if (!subtitle.empty())
            {
                aurore::Label* line = kit::label(subtitle, kit::TEXT_META, "cadran/text2");
                line->setMarginTop(aurore::dp(5));
                words->addView(line);
            }
            head->addView(words);
            return head;
        }

        // The lines as tiles, `columns` to a row: the icon large above the
        // label, the value chosen in teal. Up and down keep the column.
        void grid(aurore::Box* column, size_t columns)
        {
            aurore::Box* row = nullptr;
            for (size_t i = 0; i < this->lines.size(); i++)
            {
                if (i % columns == 0)
                {
                    row = new aurore::Box(aurore::Axis::ROW);
                    row->setAlignItems(aurore::AlignItems::STRETCH);
                    if (i > 0)
                        row->setMarginTop(aurore::dp(TILE_GAP));
                    YGNodeStyleSetGap(row->getYGNode(), YGGutterColumn, aurore::dp(TILE_GAP));
                    column->addView(row);
                }
                row->addView(this->tile(this->lines[i], i));
            }
            // The last row's empty places, so its tiles keep their width.
            for (size_t i = this->lines.size(); row && i % columns != 0; i++)
            {
                auto* gap = new aurore::Box();
                gap->setGrow(1);
                gap->setWidth(0);
                row->addView(gap);
            }
            for (size_t i = 0; i < this->cells.size(); i++)
            {
                if (i >= columns)
                    this->cells[i]->setCustomNavigationRoute(aurore::FocusDirection::UP, this->cells[i - columns]);
                if (i + columns < this->cells.size())
                    this->cells[i]->setCustomNavigationRoute(aurore::FocusDirection::DOWN, this->cells[i + columns]);
            }
        }

        aurore::DetailCell* tile(const Line& l, size_t index)
        {
            auto* cell = new aurore::DetailCell();
            cell->setText(l.label);
            cell->setDetailText("");
            cell->detail->setVisibility(aurore::Visibility::GONE);
            cell->setAxis(aurore::Axis::COLUMN);
            cell->setAlignItems(aurore::AlignItems::CENTER);
            cell->setJustifyContent(aurore::JustifyContent::CENTER);
            cell->setGrow(1);
            cell->setWidth(0);
            cell->setHeight(aurore::dp(TILE_HEIGHT));
            cell->setPadding(aurore::dp(14), aurore::dp(6), aurore::dp(14), aurore::dp(6));
            cell->setMarginRight(0);
            cell->setLineBottom(0);
            cell->setLineTop(0);
            cell->setCornerRadius(aurore::dp(kit::RADIUS_CARD));
            cell->setHighlightCornerRadius(aurore::dp(kit::RADIUS_CARD));
            cell->setThemeColor("backgroundColor", l.checked ? "cadran/teal_soft" : "cadran/surf2");
            cell->setAccessibilitySelected(l.checked);
            cell->title->setGrow(0);
            cell->title->setMarginRight(0);
            cell->title->setFontSize(kit::TEXT_NOTE);
            // On one line: a tile's width holds the longest name ("Einschränkungen")
            // at the larger text size.
            cell->title->setSingleLine(true);
            cell->title->setHorizontalAlign(aurore::HorizontalAlign::CENTER);
            if (l.checked)
                cell->title->setThemeColor("textColor", "cadran/teal");
            if (!l.icon.empty())
            {
                CadranIcon* icon = kit::icon(l.icon.c_str(), 32, l.checked ? "cadran/teal" : "cadran/text2");
                icon->setMarginBottom(aurore::dp(12));
                icon->setShrink(0);
                cell->addView(icon, 0);
            }
            const std::function<void()> run = l.run;
            cell->registerClickAction([run](aurore::View*)
                {
            close_sheet(run);
            return true; });
            if (this->cells.size() == index)
                this->cells.push_back(cell);
            return cell;
        }

        aurore::DetailCell* line(const Line& l, size_t index)
        {
            aurore::DetailCell* cell;
            if (l.toggle >= 0)
            {
                auto* toggle = new aurore::BooleanCell();
                toggle->init(l.label, l.toggle == 1, [](bool) {});
                cell = toggle;
            }
            else
            {
                cell = new aurore::DetailCell();
                cell->setText(l.label);
                cell->setDetailText(l.detail);
            }
            if (l.picture && !l.picture->empty())
                cadran::List::row_picture(cell, l.picture->data(), (int)l.picture->size());
            else if (l.swatch)
                cadran::List::row_swatch(cell, *l.swatch);
            else if (!l.image.empty())
                cadran::List::row_image(cell, l.image);
            else
                cadran::List::row(cell, l.icon.empty() ? nullptr : l.icon.c_str());
            if (l.toggle < 0)
            {
                cell->detail->setMaxWidth(aurore::dp(340));
                cell->detail->setSingleLine(true);
                cell->detail->setVisibility(l.detail.empty() ? aurore::Visibility::GONE : aurore::Visibility::VISIBLE);
            }
            const char* tint = nullptr;
            switch (l.tone)
            {
                case Choice::Tone::PLAIN:
                    break;
                case Choice::Tone::ON:
                    tint = "cadran/teal";
                    break;
                case Choice::Tone::WARN:
                    tint = "cadran/amber";
                    break;
                case Choice::Tone::DANGER:
                    tint = "cadran/coral";
                    cell->title->setThemeColor("textColor", "cadran/coral");
                    break;
            }
            if (l.toggle < 0)
                cell->detail->setThemeColor("textColor", tint ? tint : "cadran/text2");
            if (tint && !l.icon.empty())
                cadran::List::recolor_icon(cell, nullptr, tint,
                    l.tone == Choice::Tone::DANGER     ? "cadran/coral_soft"
                        : l.tone == Choice::Tone::WARN ? "cadran/amber_surf"
                                                       : "cadran/teal_soft");
            // The value chosen: a soft fill and its check at the end.
            if (l.checked)
            {
                cell->setAccessibilitySelected(true);
                cell->setThemeColor("backgroundColor", "cadran/teal_soft");
                cell->setCornerRadius(aurore::dp(kit::RADIUS_ROW));
                CadranIcon* check = kit::icon("check", 22, "cadran/teal");
                check->setMarginLeft(l.detail.empty() ? 0 : aurore::dp(14));
                check->setShrink(0);
                cell->addView(check);
            }
            // Chosen: the sheet goes, then the line's action (none: it only closes).
            const std::function<void()> run = l.run;
            cell->registerClickAction([run](aurore::View*)
                {
            close_sheet(run);
            return true; });
            if (this->cells.size() == index)
                this->cells.push_back(cell);
            return cell;
        }
    };

    void open(const std::string& title, const std::string& subtitle, int image, std::vector<Line> lines, size_t focus,
        int columns = 0)
    {
        if (lines.empty())
            return;
        if (focus >= lines.size())
            focus = 0;
        aurore::Application::pushActivity(new aurore::Activity(new Sheet(title, subtitle, image, std::move(lines), focus, columns)),
            aurore::TransitionAnimation::FADE);
    }

    std::vector<Line> value_lines(const std::vector<std::string>& values, const std::vector<std::string>& details,
        std::function<bool(size_t)> checked, std::function<void(int)> on_pick,
        const std::vector<std::string>& icons = {}, const std::vector<Swatch>& swatches = {},
        const std::vector<std::shared_ptr<const std::vector<unsigned char>>>& pictures = {})
    {
        std::vector<Line> lines;
        auto pick = std::make_shared<std::function<void(int)>>(std::move(on_pick));
        for (size_t i = 0; i < values.size(); i++)
        {
            Line l;
            l.label  = values[i];
            l.detail = i < details.size() ? details[i] : "";
            // A picture ("img/…png") or a Cadran icon's name.
            const std::string icon                        = i < icons.size() ? icons[i] : "";
            (icon.starts_with("img/") ? l.image : l.icon) = icon;
            if (i < swatches.size())
                l.swatch = swatches[i];
            if (i < pictures.size())
                l.picture = pictures[i];
            l.checked       = checked(i);
            const int index = (int)i;
            l.run           = [pick, index]()
            {
                if (*pick)
                    (*pick)(index);
            };
            lines.push_back(std::move(l));
        }
        return lines;
    }
} // namespace

bool is_sheet(aurore::View* view)
{
    return view && view->getId() == "cadran_sheet";
}

void pick_actions(const std::string& title, const std::vector<Choice>& choices, const std::string& subtitle, int image)
{
    std::vector<Line> lines;
    for (const Choice& c : choices)
    {
        Line l;
        l.label  = c.label;
        l.detail = c.detail;
        l.group  = c.group;
        l.icon   = c.icon ? c.icon : "";
        l.tone   = c.tone;
        l.toggle = c.toggle;
        l.run    = c.run;
        lines.push_back(std::move(l));
    }
    open(title, subtitle, image, std::move(lines), 0);
}

namespace
{
    using Picture = std::shared_ptr<const std::vector<unsigned char>>;

    // What pick() was given, kept for as long as its search opens it again.
    struct PickData
    {
        std::string title;
        std::vector<std::string> values, details, icons;
        std::vector<Swatch> swatches;
        std::vector<Picture> pictures;
        std::function<void(int)> on_pick;
        bool checked = true;
        int selected = 0;
        std::string search, clear;
    };

    std::string lowered(std::string text)
    {
        for (char& c : text)
            if (c >= 'A' && c <= 'Z')
                c = (char)(c - 'A' + 'a');
        return text;
    }

    // The sheet of the rows whose label holds `query` (every row: empty), under
    // the search row when the caller asked for one.
    void open_pick(const std::shared_ptr<const PickData>& d, const std::string& query)
    {
        const std::vector<size_t> shown = filter_rows(d->values, query);

        const auto pick_of = [&d, &shown](const auto& all)
        {
            std::decay_t<decltype(all)> some;
            for (size_t i : shown)
                if (i < all.size())
                    some.push_back(all[i]);
                else
                    break; // the lists may be shorter than the values: the rest has none
            return some;
        };
        const int selected = d->selected;
        std::vector<Line> lines;
        if (!d->search.empty())
        {
            Line search;
            search.label  = d->search;
            search.detail = query;
            search.icon   = "pencil";
            search.run    = [d, query]()
            { prompt_text(d->search, query, 64, [d](std::string text)
                  { open_pick(d, text); }); };
            lines.push_back(std::move(search));
            if (!query.empty() && !d->clear.empty())
            {
                Line clear;
                clear.label = d->clear;
                clear.icon  = "refresh";
                clear.run   = [d]()
                { open_pick(d, ""); };
                lines.push_back(std::move(clear));
            }
        }
        const size_t before = lines.size();
        auto rows           = value_lines(pick_of(d->values), pick_of(d->details), [&](size_t k)
                      { return d->checked && (int)shown[k] == selected; }, [d, shown](int k)
                      { if (d->on_pick && k >= 0 && (size_t)k < shown.size()) d->on_pick((int)shown[k]); }, pick_of(d->icons), pick_of(d->swatches), pick_of(d->pictures));
        const bool any      = !rows.empty();
        for (Line& l : rows)
            lines.push_back(std::move(l));

        size_t focus = any ? before : 0;
        for (size_t k = 0; k < shown.size(); k++)
            if ((int)shown[k] == selected)
                focus = before + k;
        open(d->title, "", 0, std::move(lines), focus);
    }
} // namespace

std::vector<size_t> filter_rows(const std::vector<std::string>& values, const std::string& query)
{
    const std::string needle = lowered(query);
    std::vector<size_t> rows;
    for (size_t i = 0; i < values.size(); i++)
        if (needle.empty() || lowered(values[i]).find(needle) != std::string::npos)
            rows.push_back(i);
    return rows;
}

void pick(const std::string& title, const std::vector<std::string>& values, int selected,
    std::function<void(int)> on_pick, bool checked, const std::vector<std::string>& details,
    const std::vector<std::string>& icons, const std::vector<Swatch>& swatches, const PickOptions& options)
{
    auto data      = std::make_shared<PickData>();
    data->title    = title;
    data->values   = values;
    data->details  = details;
    data->icons    = icons;
    data->swatches = swatches;
    for (const auto& bytes : options.pictures)
        data->pictures.push_back(bytes.empty() ? nullptr : std::make_shared<const std::vector<unsigned char>>(bytes));
    data->on_pick  = std::move(on_pick);
    data->checked  = checked;
    data->selected = selected < 0 ? 0 : selected;
    data->search   = options.search;
    data->clear    = options.clear;
    open_pick(data, "");
}

void pick_grid(const std::string& title, const std::vector<std::string>& values, const std::vector<std::string>& icons,
    int selected, std::function<void(int)> on_pick, int columns)
{
    const size_t focus = selected < 0 ? 0 : (size_t)selected;
    open(title, "", 0,
        value_lines(values, {}, [focus](size_t i)
            { return i == focus; }, std::move(on_pick), icons),
        focus, columns < 1 ? 1 : columns);
}

void pick_marked(const std::string& title, const std::vector<std::string>& values, const std::vector<bool>& marks,
    int focus, std::function<void(int)> on_pick, const std::vector<std::string>& details)
{
    open(title, "", 0,
        value_lines(values, details, [marks](size_t i)
            { return i < marks.size() && marks[i]; }, std::move(on_pick)),
        focus < 0 ? 0 : (size_t)focus);
}

} // namespace ui
