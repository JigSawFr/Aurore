// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include "tab/cadran_tab.hpp"

#include <borealis/cadran.hpp>

#include <string>
#include <vector>

using namespace brls::literals;   // for _i18n

namespace kit = cadran::kit;

namespace
{
// A section: its kicker, then a card holding `content`.
brls::Box* section(brls::Box* column, const char* icon, const std::string& title)
{
    brls::Box* head = kit::kicker(icon, title);
    head->setMarginTop(column->getChildren().empty() ? 0 : 28);
    head->setMarginBottom(12);
    column->addView(head);
    brls::Box* card = kit::card(false, false);
    card->setAxis(brls::Axis::COLUMN);
    card->setPadding(20, 22, 20, 22);
    column->addView(card);
    return card;
}

// A row of the cards above: their figure, its caption, a pill.
brls::Box* figure(const std::string& value, const std::string& caption, const std::string& state, const char* dot)
{
    brls::Box* card = kit::top_card(true);
    card->setAxis(brls::Axis::COLUMN);
    card->setPadding(18, 20, 18, 20);
    auto* digits = new CadranDigits();
    digits->setText(value);
    digits->setFontSize(44);
    digits->setTextColor(kit::color("cadran/text"));
    digits->setHeight(54);
    card->addView(digits);
    brls::Label* label = kit::label(caption, kit::TEXT_NOTE, "cadran/text2");
    label->setMarginBottom(12);
    card->addView(label);
    brls::Box* pill = kit::pill(state, dot);
    pill->setAlignSelf(brls::AlignSelf::FLEX_START);
    card->addView(pill);
    return card;
}

// A button of the dialogs' card: its text, what it opens.
brls::Button* action(const std::string& text, std::function<void()> run, cadran::Emphasis emphasis)
{
    auto* button = new brls::Button();
    button->setText(text);
    button->setMarginRight(12);
    button->setMarginBottom(12);
    button->registerClickAction([run](brls::View*) {
        run();
        return true;
    });
    cadran::style_button(button, emphasis);
    kit::fit_button(button);
    return button;
}

const char* const ICONS[] = { "today", "timer",   "activity", "shield",   "lock",    "globe",   "settings", "tools",
                              "info",  "eye",     "check",    "moon",     "sun",     "list",    "download", "key",
                              "phone", "trash",   "server",   "refresh",  "history", "save",    "upload",   "home",
                              "bolt",  "sparkle", "bell",     "code",     "heart",   "folder",  "file",     "flag",
                              "gamepad", "calendar", "alert", "pencil",   "tag",     "user",    "tv",       "chip" };

const char* const MARKDOWN = "## Markdown\n"
                             "Cadran draws **headings**, *emphasis*, `code`, lists and tables, "
                             "with the emoji they open on.\n\n"
                             "- 🚀 A bullet that opens on an emoji\n"
                             "- [x] A task done\n"
                             "- [ ] A task to do\n\n"
                             "> [!TIP]\n"
                             "> Alerts as GitHub writes them.\n\n"
                             "| Piece | Header |\n"
                             "|---|---|\n"
                             "| Rail | `TabFrame` |\n"
                             "| Cards | `cadran::kit` |\n";
}   // namespace

CadranTab::CadranTab()
{
    auto* column = new brls::Box(brls::Axis::COLUMN);
    column->setPadding(28, 40, 40, 40);
    this->setContentView(column);

    // The figures, as a dashboard's top cards.
    brls::Box* head = kit::kicker("today", "demo/cadran/figures"_i18n);
    head->setMarginBottom(12);
    column->addView(head);
    auto* figures = new brls::Box(brls::Axis::ROW);
    figures->addView(figure("2:45", "demo/cadran/played"_i18n, "demo/cadran/on_track"_i18n, "cadran/teal"));
    figures->addView(figure("12", "demo/cadran/games"_i18n, "demo/cadran/updates"_i18n, "cadran/amber"));
    figures->addView(figure("98%", "demo/cadran/storage"_i18n, "demo/cadran/almost_full"_i18n, "cadran/coral"));
    for (size_t i = 0; i + 1 < figures->getChildren().size(); i++) figures->getChildren()[i]->setMarginRight(16);
    column->addView(figures);

    // Chips and a bar.
    brls::Box* chips = section(column, "timer", "demo/cadran/chips"_i18n);
    auto* row = new brls::Box(brls::Axis::ROW);
    for (const char* text : { "15 min", "30 min", "1 h", "2 h" }) {
        brls::Box* chip = kit::chip(text);
        chip->setMarginRight(12);
        row->addView(chip);
    }
    chips->addView(row);
    auto* bar = new kit::Bar();
    bar->setMarginTop(20);
    bar->set(0.62f);
    chips->addView(bar);

    // Dialogs, sheets and toasts.
    brls::Box* dialogs = section(column, "sparkle", "demo/cadran/dialogs"_i18n);
    auto* buttons = new brls::Box(brls::Axis::ROW);
    kit::wrap(buttons);
    const auto E = cadran::Emphasis::NONE;
    buttons->addView(action("demo/cadran/confirm"_i18n, [] {
        ui::confirm("demo/cadran/confirm_text"_i18n, "demo/cadran/confirm"_i18n,
                    [] { ui::notify("demo/cadran/done"_i18n, ui::Toast::Done); });
    }, cadran::Emphasis::PRIMARY));
    buttons->addView(action("demo/cadran/delete"_i18n, [] {
        ui::confirm_danger("demo/cadran/delete_text"_i18n, "demo/cadran/delete"_i18n,
                           [] { ui::notify("demo/cadran/deleted"_i18n, ui::Toast::Done); });
    }, cadran::Emphasis::DANGER));
    buttons->addView(action("demo/cadran/sheet"_i18n, [] {
        ui::pick_actions("demo/cadran/sheet"_i18n,
                         { ui::Choice("demo/cadran/share"_i18n, [] { ui::notify("demo/cadran/shared"_i18n); }, "upload"),
                           ui::Choice("demo/cadran/favourite"_i18n, [] {}, "heart").switched(true),
                           ui::Choice("demo/cadran/rename"_i18n, [] {}, "pencil", "Aurore"),
                           ui::Choice("demo/cadran/delete"_i18n, [] {}, "trash", "", "",
                                      ui::Choice::Tone::DANGER) },
                         "demo/cadran/sheet_subtitle"_i18n);
    }, E));
    buttons->addView(action("demo/cadran/pick"_i18n, [] {
        static int chosen = 1;
        ui::pick("demo/cadran/pick"_i18n, { "15 min", "30 min", "1 h", "2 h" }, chosen,
                 [](int i) { chosen = i; }, true, {}, { "timer", "timer", "timer", "timer" });
    }, E));
    buttons->addView(action("demo/cadran/checklist"_i18n, [] {
        brls::Box* body = ui::checklist_body("demo/cadran/checklist"_i18n,
                                             { { ui::Check::Ok, "demo/cadran/check_ok"_i18n },
                                               { ui::Check::Warn, "demo/cadran/check_warn"_i18n },
                                               { ui::Check::Bad, "demo/cadran/check_bad"_i18n },
                                               { ui::Check::Info, "demo/cadran/check_info"_i18n } },
                                             "demo/cadran/check_note"_i18n);
        brls::Dialog* dialog = ui::box_dialog(body);
        dialog->addButton("hints/ok"_i18n, [] {});
        dialog->open();
    }, E));
    buttons->addView(action("demo/cadran/hold"_i18n, [] {
        ui::hold_confirm(cadran::dialog_body("demo/cadran/hold_text"_i18n), "demo/cadran/hold"_i18n,
                         [] { ui::notify("demo/cadran/done"_i18n, ui::Toast::Done); });
    }, E));
    buttons->addView(action("demo/cadran/qr"_i18n, [] {
        ui::qr_dialog("demo/cadran/qr_text"_i18n, "https://github.com/JigSawFr/aurore");
    }, E));
    buttons->addView(action("demo/cadran/toast"_i18n, [] { ui::notify("demo/cadran/toast_text"_i18n); }, E));
    buttons->addView(action("demo/cadran/refused"_i18n, [] { ui::refuse("demo/cadran/refused_text"_i18n); }, E));
    buttons->addView(action("cadran/quick/title_text"_i18n, [] { ui::quick_settings(); }, E));
    buttons->addView(action("cadran/prefs/rail"_i18n, [] { ui::pick_rail(); }, E));
    dialogs->addView(buttons);

    // The icons.
    brls::Box* icons = section(column, "eye", "demo/cadran/icons"_i18n);
    auto* grid = new brls::Box(brls::Axis::ROW);
    kit::wrap(grid);
    for (const char* name : ICONS) {
        CadranIcon* icon = kit::icon(name, 30, "cadran/text2");
        icon->setMargins(8, 12, 8, 12);
        grid->addView(icon);
    }
    icons->addView(grid);

    // Markdown.
    brls::Box* text = section(column, "file", "demo/cadran/markdown"_i18n);
    markdown_view::fill(text, markdown::parse(MARKDOWN));
}

brls::View* CadranTab::create()
{
    return new CadranTab();
}
