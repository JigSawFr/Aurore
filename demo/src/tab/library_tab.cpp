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

#include "tab/library_tab.hpp"

#include <aurore/cadran.hpp>

#include <algorithm>

#include "demo_app.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// What the user chose, kept while the app runs (the tab is built again
// when the language changes)
enum Sort
{
    SORT_RECENT,
    SORT_TITLE,
    SORT_PLAYTIME,
    SORT_YEAR,
};
int sortBy = SORT_RECENT;
int genre  = -1; // all

const char* const SORT_KEYS[] = { "demo/library/sort_recent", "demo/library/sort_title", "demo/library/sort_playtime",
    "demo/library/sort_year" };

// The grid: the games shown
class GridSource : public aurore::RecyclingGridDataSource
{
  public:
    size_t getItemCount() override { return LibraryTab::shown().size(); }

    aurore::RecyclingGridItem* cellForRow(aurore::RecyclingView* recycler, size_t index) override
    {
        auto* cell = (GameCell*)recycler->dequeueReusableCell("Game");
        cell->setGame(&library::games()[LibraryTab::shown()[index]]);
        return cell;
    }

    void onItemSelected(aurore::View* recycler, size_t index) override { openGame(LibraryTab::shown()[index]); }
};

// The list: a line per game, a section per genre
class ListCell : public aurore::RecyclerCell
{
  public:
    ListCell()
    {
        this->setFocusable(true);
        this->setAxis(aurore::Axis::ROW);
        this->setAlignItems(aurore::AlignItems::CENTER);
        this->setPadding(10, 16, 10, 12);
        this->setHeight(84);
        this->cover = new CoverArt();
        this->cover->setWidth(48);
        this->cover->setHeight(64);
        this->cover->setRadius(8);
        this->cover->setTitleShown(false);
        this->cover->setMarginRight(18);
        this->addView(this->cover);
        auto* texts = new aurore::Box(aurore::Axis::COLUMN);
        texts->setGrow(1);
        texts->setWidth(0);
        this->title = kit::label("", kit::TEXT_BODY + 1, "cadran/text");
        this->title->setSingleLine(true);
        texts->addView(this->title);
        this->detail = kit::label("", kit::TEXT_NOTE, "cadran/text2");
        this->detail->setSingleLine(true);
        texts->addView(this->detail);
        this->addView(texts);
        this->bar = new kit::Bar();
        this->bar->setWidth(140);
        this->bar->setMarginLeft(16);
        if (show::compact())
            this->bar->setVisibility(aurore::Visibility::GONE);
        this->addView(this->bar);
    }

    void setGame(const library::Game& game)
    {
        this->cover->setGame(&game);
        this->title->setText(game.title);
        std::string played = game.minutes ? aurore::getStr("demo/library/played", library::playtime(game.minutes))
                                          : "demo/library/never"_i18n;
        this->detail->setText(std::to_string(game.year) + "  ·  " + played);
        this->bar->set(game.completion);
    }

    static aurore::RecyclerCell* create() { return new ListCell(); }

  private:
    CoverArt* cover;
    aurore::Label* title;
    aurore::Label* detail;
    kit::Bar* bar;
};

// A section's head: its genre and how many, as a kicker
class GenreHeader : public aurore::RecyclerCell
{
  public:
    GenreHeader()
    {
        this->setFocusable(false);
        this->setAxis(aurore::Axis::ROW);
        this->setAlignItems(aurore::AlignItems::FLEX_END);
        this->setPadding(0, 4, 10, 4);
        this->label = new aurore::Label();
        kit::set_kicker(this->label);
        this->addView(this->label);
    }

    void setTitle(const std::string& title) { kit::set_kicker(this->label, title); }

    static aurore::RecyclerCell* create() { return new GenreHeader(); }

  private:
    aurore::Label* label;
};

class ListSource : public aurore::RecyclerDataSource
{
  public:
    ListSource()
    {
        // The games shown, by genre, in the order chosen
        this->sections.resize(library::GENRE_COUNT);
        for (size_t index : LibraryTab::shown())
            this->sections[library::games()[index].genre].push_back(index);
        this->sections.erase(std::remove_if(this->sections.begin(), this->sections.end(),
                                 [](const std::vector<size_t>& s)
                                 { return s.empty(); }),
            this->sections.end());
    }

    int numberOfSections(aurore::RecyclerFrame*) override { return (int)this->sections.size(); }
    int numberOfRows(aurore::RecyclerFrame*, int section) override { return (int)this->sections[section].size(); }

    aurore::RecyclerCell* cellForHeader(aurore::RecyclerFrame* recycler, int section) override
    {
        auto* header               = (GenreHeader*)recycler->dequeueReusableCell("Genre");
        const library::Game& first = library::games()[this->sections[section][0]];
        header->setTitle(library::genreName(first.genre) + "  ·  " + std::to_string(this->sections[section].size()));
        return header;
    }

    float heightForHeader(aurore::RecyclerFrame*, int section) override { return section == 0 ? 36 : 64; }

    aurore::RecyclerCell* cellForRow(aurore::RecyclerFrame* recycler, aurore::IndexPath path) override
    {
        auto* cell = (ListCell*)recycler->dequeueReusableCell("Line");
        cell->setGame(library::games()[this->sections[path.section][path.row]]);
        return cell;
    }

    void didSelectRowAt(aurore::RecyclerFrame*, aurore::IndexPath path) override
    {
        openGame(this->sections[path.section][path.row]);
    }

  private:
    std::vector<std::vector<size_t>> sections;
};
} // namespace

// The cell of the grid and of the rows

GameCell::GameCell()
{
    this->setAxis(aurore::Axis::COLUMN);
    this->setHighlightCornerRadius(kit::RADIUS_ICON + 2);
    this->cover = new CoverArt();
    this->cover->setGrow(1);
    this->addView(this->cover);
    this->caption = kit::label("", kit::TEXT_META, "cadran/text2");
    this->caption->setSingleLine(true);
    // A compact screen's narrow covers: the line drawn smaller to stay whole
    this->caption->setShrinkToFit(true);
    this->caption->setShrinkToFitMin(10);
    this->caption->setMarginTop(8);
    this->caption->setMarginLeft(2);
    this->addView(this->caption);

    // X or a long press: what can be done with it, next to it
    aurore::ContextMenu::attach(this, [this](aurore::ContextMenu* menu)
        {
        if (!this->game)
            return;
        size_t index = (size_t)(this->game - library::games().data());
        menu->setTitle(this->game->title);
        menu->addItem("demo/library/open"_i18n, [index] { openGame(index); }, kit::icon("eye", 22));
        menu->addItem("demo/library/favourite"_i18n, [] { ui::notify("demo/library/favourited"_i18n, ui::Toast::Done); },
            kit::icon("heart", 22));
        menu->addItem("demo/library/share"_i18n, [] { ui::notify("demo/library/shared"_i18n); }, kit::icon("upload", 22));
        menu->addSeparator();
        menu->addItem("demo/library/hide"_i18n, [] { ui::refuse("demo/library/hide_refused"_i18n); },
            kit::icon("trash", 22, "cadran/coral")); });
}

void GameCell::setGame(const library::Game* game)
{
    this->game = game;
    this->cover->setGame(game);
    this->caption->setText(library::genreName(game->genre) + "  ·  " + std::to_string(game->year));
    this->setAccessibilityLabel(game->title);
}

void GameCell::prepareForReuse()
{
    this->game = nullptr;
    this->cover->setGame(nullptr);
}

GameCell* GameCell::create()
{
    return new GameCell();
}

// The tab

std::vector<size_t>& LibraryTab::shown()
{
    static std::vector<size_t> list;
    return list;
}

LibraryTab::LibraryTab()
    : aurore::Box(aurore::Axis::COLUMN)
{
    // The margins on each piece: a RecyclerFrame places its cells as if
    // its box had none
    this->setPadding(show::pageTop(), 0, 0, 0);

    // The head: what is shown, then how
    aurore::Box* head = show::row(true, 12);
    head->setAlignItems(aurore::AlignItems::CENTER);
    head->setMargins(0, show::pageSides(), aurore::dp(18), show::pageSides());
    // On a compact screen, the buttons alone: the rail names the tab
    const bool compact = show::compact();
    auto* titles       = show::column(2);
    titles->setGrow(1);
    titles->addView(kit::kicker("shelf", "demo/tabs/library"_i18n));
    this->count = kit::label("", kit::TEXT_NOTE, "cadran/text2");
    titles->addView(this->count);
    if (compact)
        titles->setVisibility(aurore::Visibility::GONE);
    head->addView(titles);

    this->sortButton = show::iconButton("lines", "", [this]
        {
        std::vector<std::string> names;
        for (const char* key : SORT_KEYS)
            names.push_back(aurore::getStr(key));
        ui::pick("demo/library/sort"_i18n, names, sortBy, [this](int chosen)
            {
            sortBy = chosen;
            this->refresh(); }); });
    head->addView(this->sortButton);

    this->genreButton = show::iconButton("tag", "", [this]
        {
        std::vector<std::string> names = { "demo/library/all_genres"_i18n };
        std::vector<std::string> icons = { "list" };
        for (int g = 0; g < library::GENRE_COUNT; g++)
        {
            names.push_back(library::genreName((library::Genre)g));
            icons.push_back(library::genreIcon((library::Genre)g));
        }
        ui::pick("demo/library/genre"_i18n, names, genre + 1, [this](int chosen)
            {
            genre = chosen - 1;
            this->refresh(); }, true, {}, icons); });
    head->addView(this->genreButton);

    aurore::IconButton* list = show::iconButton("list", compact ? "" : "demo/library/by_genre"_i18n, []
        { openGameList(); });
    list->setAccessibilityLabel("demo/library/by_genre"_i18n);
    head->addView(list);
    this->addView(head);

    // The grid: as many columns 160 wide as fit, a cover 3:4 and its line
    this->grid = new aurore::RecyclingGrid();
    this->grid->setGrow(1);
    this->grid->setColumnWidth(aurore::dp(160));
    this->grid->setItemHeight(aurore::dp(262));
    this->grid->setItemSpace(aurore::dp(18));
    this->grid->setPadding(4, show::pageSides() + 4, aurore::dp(32), show::pageSides() + 4);
    this->grid->registerCell("Game", []
        { return GameCell::create(); });

    this->addView(this->grid);
    this->refresh();
}

void LibraryTab::refresh()
{
    const std::vector<library::Game>& all = library::games();
    std::vector<size_t>& list             = shown();
    list.clear();
    for (size_t i = 0; i < all.size(); i++)
        if (genre < 0 || all[i].genre == genre)
            list.push_back(i);
    std::stable_sort(list.begin(), list.end(), [&all](size_t a, size_t b)
        {
        switch (sortBy)
        {
            case SORT_TITLE: return all[a].title < all[b].title;
            case SORT_PLAYTIME: return all[a].minutes > all[b].minutes;
            case SORT_YEAR: return all[a].year > all[b].year;
            default: return all[a].recent < all[b].recent;
        } });

    this->count->setText(aurore::getStr("demo/library/count", list.size()));
    this->sortButton->setText(aurore::getStr(SORT_KEYS[sortBy]));
    this->genreButton->setText(genre < 0 ? "demo/library/all_genres"_i18n : library::genreName((library::Genre)genre));
    kit::fit_button(this->sortButton);
    kit::fit_button(this->genreButton);

    this->grid->setDataSource(new GridSource());
}

void openGameList()
{
    // A screen of its own: a RecyclerFrame placed as a screen's content
    auto* box  = new aurore::Box(aurore::Axis::COLUMN);
    auto* list = new aurore::RecyclerFrame();
    list->setGrow(1);
    list->estimatedRowHeight = 84;
    list->setPadding(show::pageTop(), show::screenSides(), show::pageBottom(), show::screenSides());
    list->registerCell("Genre", []
        { return GenreHeader::create(); });
    list->registerCell("Line", []
        { return ListCell::create(); });
    list->setDefaultCellFocus(aurore::IndexPath(0, 0));
    list->setDataSource(new ListSource());
    box->addView(list);
    demo::pushScreen(box, "demo/library/by_genre_title"_i18n);
}

aurore::View* LibraryTab::create()
{
    return new LibraryTab();
}
