// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tab/cadran_tab.hpp"

#include <aurore/cadran.hpp>

#include <string>
#include <vector>

#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
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
} // namespace

namespace cadran_pieces
{

aurore::Box* dialogs()
{
    aurore::Box* buttons = show::row(true, 12);
    buttons->addView(show::button("demo/cadran/confirm"_i18n, []
        { ui::confirm("demo/cadran/confirm_text"_i18n, "demo/cadran/confirm"_i18n,
              []
              { ui::notify("demo/cadran/done"_i18n, ui::Toast::Done); }); }, true));
    aurore::Button* remove = show::button("demo/cadran/delete"_i18n, []
        { ui::confirm_danger("demo/cadran/delete_text"_i18n, "demo/cadran/delete"_i18n,
              []
              { ui::notify("demo/cadran/deleted"_i18n, ui::Toast::Done); }); });
    cadran::style_button(remove, cadran::Emphasis::DANGER);
    buttons->addView(remove);
    buttons->addView(show::button("demo/cadran/sheet"_i18n, []
        { ui::pick_actions("demo/cadran/sheet"_i18n,
              { ui::Choice("demo/cadran/share"_i18n, []
                    { ui::notify("demo/cadran/shared"_i18n); }, "upload"),
                  ui::Choice("demo/cadran/favourite"_i18n, [] {}, "heart").switched(true), ui::Choice("demo/cadran/rename"_i18n, [] {}, "pencil", "Aurore"), ui::Choice("demo/cadran/delete"_i18n, [] {}, "trash", "", "", ui::Choice::Tone::DANGER) },
              "demo/cadran/sheet_subtitle"_i18n); }));
    buttons->addView(show::button("demo/cadran/pick"_i18n, []
        {
        static int chosen = 1;
        ui::pick("demo/cadran/pick"_i18n, { "15 min", "30 min", "1 h", "2 h" }, chosen,
                 [](int i) { chosen = i; }, true, {}, { "timer", "timer", "timer", "timer" }); }));
    buttons->addView(show::button("demo/cadran/tiles"_i18n, []
        {
        static int chosen = 0;
        ui::pick_grid("demo/cadran/tiles"_i18n,
            { "demo/tabs/home"_i18n, "demo/tabs/library"_i18n, "demo/tabs/themes"_i18n, "demo/tabs/input"_i18n },
            { "home", "shelf", "palette", "gamepad" }, chosen, [](int i) { chosen = i; }); }));
    buttons->addView(show::button("demo/cadran/marked"_i18n, []
        {
        static std::vector<bool> marks = { true, false, true, false };
        static std::function<void(int)> open;
        open = [](int focus)
        {
            ui::pick_marked("demo/cadran/marked"_i18n,
                { "demo/library/genres/adventure"_i18n, "demo/library/genres/racing"_i18n,
                  "demo/library/genres/puzzle"_i18n, "demo/library/genres/rhythm"_i18n },
                marks, focus, [](int i)
                {
                    marks[i] = !marks[i];
                    open(i);
                });
        };
        open(0); }));
    buttons->addView(show::button("demo/cadran/checklist"_i18n, []
        {
        aurore::Box* body = ui::checklist_body("demo/cadran/checklist"_i18n,
                                             { { ui::Check::Ok, "demo/cadran/check_ok"_i18n },
                                               { ui::Check::Warn, "demo/cadran/check_warn"_i18n },
                                               { ui::Check::Bad, "demo/cadran/check_bad"_i18n },
                                               { ui::Check::Info, "demo/cadran/check_info"_i18n } },
                                             "demo/cadran/check_note"_i18n);
        aurore::Dialog* dialog = ui::box_dialog(body);
        dialog->addButton("hints/ok"_i18n, [] {});
        dialog->open(); }));
    buttons->addView(show::button("demo/cadran/hold"_i18n, []
        { ui::hold_confirm(cadran::dialog_body("demo/cadran/hold_text"_i18n), "demo/cadran/hold"_i18n,
              []
              { ui::notify("demo/cadran/done"_i18n, ui::Toast::Done); }); }));
    buttons->addView(show::button("demo/cadran/wait"_i18n, []
        {
        // A spinner while something is under way, closed after a while
        aurore::Dialog* dialog = ui::box_dialog(cadran::wait_body("demo/cadran/wait_text"_i18n));
        ui::add_cancel(dialog, [] {});
        dialog->open();
        aurore::delay(2500, [dialog]
            {
            if (aurore::Application::getActivitiesStack().back()->getContentView() == dialog)
                dialog->close();
            ui::notify("demo/cadran/done"_i18n, ui::Toast::Done); }); }));
    buttons->addView(show::button("demo/cadran/notice"_i18n, []
        { ui::notice("sparkle", ui::Check::Ok, "demo/cadran/notice_title"_i18n, "demo/cadran/notice_text"_i18n); }));
    buttons->addView(show::button("demo/cadran/qr"_i18n, []
        { ui::qr_dialog("demo/cadran/qr_text"_i18n, "https://aurore.boreale.dev/"); }));
    buttons->addView(show::button("demo/cadran/keyboard"_i18n, []
        { ui::prompt_text("demo/cadran/keyboard_title"_i18n, "Aurore", 32, [](std::string text)
              { ui::notify(aurore::getStr("demo/cadran/typed", text)); }); }));
    buttons->addView(show::button("demo/cadran/quick"_i18n, []
        { ui::quick_settings(); }));
    buttons->addView(show::button("cadran/prefs/rail"_i18n, []
        { ui::pick_rail(); }));
    buttons->addView(show::button("cadran/prefs/density"_i18n, []
        { ui::pick_density(); }));
    return buttons;
}

aurore::Box* toasts()
{
    aurore::Box* buttons = show::row(true, 12);
    buttons->addView(show::button("demo/cadran/toast"_i18n, []
        { ui::notify("demo/cadran/toast_text"_i18n); }));
    buttons->addView(show::button("demo/cadran/toast_done"_i18n, []
        { ui::notify("demo/cadran/done"_i18n, ui::Toast::Done); }));
    buttons->addView(show::button("demo/cadran/refused"_i18n, []
        { ui::refuse("demo/cadran/refused_text"_i18n); }));
    buttons->addView(show::button("demo/cadran/error"_i18n, []
        { ui::error("demo/cadran/error_what"_i18n, "demo/cadran/error_why"_i18n); }));
    buttons->addView(show::button("demo/cadran/notification"_i18n, []
        { aurore::Application::notify("demo/cadran/notification_text"_i18n); }));
    return buttons;
}

aurore::Box* icons()
{
    aurore::Box* grid = show::row(true, 0);
    for (const std::string& name : CadranIcon::names())
    {
        auto* cell = new aurore::Box(aurore::Axis::COLUMN);
        cell->setWidth(52);
        cell->setHeight(52);
        cell->setAlignItems(aurore::AlignItems::CENTER);
        cell->setJustifyContent(aurore::JustifyContent::CENTER);
        cell->addView(kit::icon(name.c_str(), 28, "cadran/text2"));
        grid->addView(cell);
    }
    return grid;
}

void markdown(aurore::Box* box)
{
    markdown_view::fill(box, markdown::parse(MARKDOWN));
}

} // namespace cadran_pieces
