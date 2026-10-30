// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// A game's page: pushed over the Library with a fade, in an
// AppletFrame that Cadran gives its header and the crumb back to the tab.

#include <aurore/cadran.hpp>

#include <set>

#include "demo_app.hpp"
#include "tab/library_tab.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
std::set<size_t> favourites;

const char* const BLURBS[] = { "demo/library/blurbs/adventure", "demo/library/blurbs/racing",
    "demo/library/blurbs/puzzle", "demo/library/blurbs/platformer", "demo/library/blurbs/strategy",
    "demo/library/blurbs/rhythm", "demo/library/blurbs/space", "demo/library/blurbs/ocean" };

// The row of games like it: those of its genre, but itself
class LikeSource : public aurore::RecyclingGridDataSource
{
  public:
    explicit LikeSource(size_t game)
    {
        const auto& all = library::games();
        for (size_t i = 0; i < all.size() && this->games.size() < 16; i++)
            if (i != game && all[i].genre == all[game].genre)
                this->games.push_back(i);
    }

    size_t getItemCount() override { return this->games.size(); }

    aurore::RecyclingGridItem* cellForRow(aurore::RecyclingView* recycler, size_t index) override
    {
        auto* cell = (GameCell*)recycler->dequeueReusableCell("Game");
        cell->setGame(&library::games()[this->games[index]]);
        return cell;
    }

    void onItemSelected(aurore::View*, size_t index) override { openGame(this->games[index]); }

  private:
    std::vector<size_t> games;
};

aurore::Box* pill(const char* icon, const std::string& text)
{
    auto* box = new aurore::Box(aurore::Axis::ROW);
    box->setAlignItems(aurore::AlignItems::CENTER);
    box->setGap(8);
    box->setPadding(6, 14, 6, 12);
    box->setCornerRadius(16);
    box->setBorderThickness(1);
    box->setThemeColor("borderColor", "cadran/border");
    box->addView(kit::icon(icon, 18, "cadran/text2"));
    aurore::Label* label = kit::label(text, kit::TEXT_META, "cadran/text2");
    label->setSingleLine(true);
    box->addView(label);
    return box;
}

class GamePage : public ScrollView
{
  public:
    explicit GamePage(size_t index)
    {
        const library::Game& game = library::games()[index];
        aurore::Box* page         = show::page(this);

        // The cover beside what it says, or above it on a compact screen
        const bool compact = show::compact();
        aurore::Box* top   = compact ? show::column(20) : show::row(false, 36);
        auto* cover        = new CoverArt();
        cover->setGame(&game);
        cover->setRadius(kit::RADIUS_TOP);
        cover->setWidth(aurore::dp(compact ? 180 : 270));
        cover->setHeight(aurore::dp(compact ? 240 : 360));
        cover->setShrink(0);
        top->addView(cover);

        aurore::Box* about = show::column(0);
        if (!compact)
        {
            about->setGrow(1);
            about->setWidth(0);
        }
        about->addView(kit::kicker(library::genreIcon(game.genre), library::genreName(game.genre)));
        aurore::Label* title = kit::display(game.title, compact ? 32 : 40);
        title->setMarginTop(10);
        about->addView(title);

        aurore::Box* facts = show::row(true, 8);
        facts->setMarginTop(16);
        facts->addView(pill("calendar", std::to_string(game.year)));
        facts->addView(pill("user", game.players == 1 ? "demo/library/one_player"_i18n : aurore::getStr("demo/library/players", game.players)));
        facts->addView(pill("timer", game.minutes ? aurore::getStr("demo/library/played", library::playtime(game.minutes)) : "demo/library/never"_i18n));
        about->addView(facts);

        aurore::Label* blurb = kit::label(aurore::getStr(BLURBS[game.genre]), kit::TEXT_LEAD, "cadran/text2");
        blurb->setMarginTop(18);
        blurb->setLineHeight(1.4f);
        about->addView(blurb);

        aurore::Box* actions = show::row(true, 12);
        actions->setMarginTop(24);
        std::string name     = game.title;
        aurore::Button* play = show::button(game.minutes ? "demo/library/resume"_i18n : "demo/library/play"_i18n, [name]
            { ui::notify(aurore::getStr("demo/library/starting", name)); }, true);
        actions->addView(play);
        aurore::IconButton* favourite = show::iconButton("heart", "", nullptr);
        auto setFavourite             = [favourite, index]()
        {
            bool on = favourites.contains(index);
            favourite->setText(on ? "demo/library/favourite_on"_i18n : "demo/library/favourite"_i18n);
            favourite->setIconView(kit::icon("heart", 22, on ? "cadran/coral" : "cadran/text"));
            kit::fit_button(favourite);
        };
        setFavourite();
        favourite->registerClickAction([index, setFavourite](aurore::View*)
            {
            if (favourites.contains(index))
                favourites.erase(index);
            else
            {
                favourites.insert(index);
                ui::notify("demo/library/favourited"_i18n, ui::Toast::Done);
            }
            setFavourite();
            return true; });
        actions->addView(favourite);
        actions->addView(show::iconButton("settings", "demo/library/manage"_i18n, [name]
            { ui::pick_actions(name,
                  { ui::Choice("demo/library/share"_i18n, []
                        { ui::notify("demo/library/shared"_i18n); }, "upload"),
                      ui::Choice("demo/library/cloud"_i18n, [] {}, "server", "", "demo/library/data"_i18n).switched(true), ui::Choice("demo/library/size"_i18n, [] {}, "save", "2.4 GB", "demo/library/data"_i18n), ui::Choice("demo/library/uninstall"_i18n, [name]
                                                                                                                                                                                                                        { ui::confirm_danger(aurore::getStr("demo/library/uninstall_text", name), "demo/library/uninstall"_i18n,
                                                                                                                                                                                                                              []
                                                                                                                                                                                                                              { ui::notify("demo/library/uninstalled"_i18n, ui::Toast::Done); }); }, "trash", "", "demo/library/data"_i18n, ui::Choice::Tone::DANGER) },
                  "demo/library/manage_subtitle"_i18n); }));
        about->addView(actions);
        top->addView(about);
        page->addView(top);

        // How far it was played
        aurore::Box* progress = show::section(page, "check", "demo/library/progress"_i18n);
        aurore::Box* card     = show::card(false, false);
        aurore::Box* line     = show::row(false, 20);
        line->setAlignItems(aurore::AlignItems::CENTER);
        auto* percent = new CadranDigits();
        percent->setText(std::to_string((int)(game.completion * 100 + 0.5f)) + "%");
        percent->setFontSize(36);
        percent->setSmallUnits(true);
        percent->setThemeColor("textColor", "cadran/text");
        line->addView(percent);
        aurore::Box* barColumn = show::column(8);
        barColumn->setGrow(1);
        barColumn->setWidth(0);
        auto* bar = new kit::Bar();
        bar->set(game.completion);
        barColumn->addView(bar);
        barColumn->addView(kit::label(aurore::getStr("demo/library/achievements", game.achieved, game.achievements), kit::TEXT_NOTE,
            "cadran/text2"));
        line->addView(barColumn);
        card->addView(line);
        progress->addView(card);

        // Games like it, in a row that keeps only the covers on screen
        aurore::Box* like = show::section(page, "sparkle", "demo/library/like"_i18n, "demo/library/like_lead"_i18n);
        auto* row         = new aurore::HRecyclerFrame();
        row->setHeight(aurore::dp(270));
        row->setItemWidth(aurore::dp(160));
        row->setItemSpace(aurore::dp(18));
        row->setPadding(4, 4, 4, 4);
        row->registerCell("Game", []
            { return GameCell::create(); });
        row->setDataSource(new LikeSource(index));
        like->addView(row);
    }
};
} // namespace

void openGame(size_t index)
{
    demo::pushScreen(new GamePage(index), library::games()[index].title);
}
