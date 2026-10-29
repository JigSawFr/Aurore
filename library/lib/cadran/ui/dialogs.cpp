// ui — dialogs, confirmations, a web page's QR code, the dropdown, and keyboard prompts.
// Copyright (C) 2026 JigSawFr, (C) 2026 Taylor.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/ui/ui.hpp>

#include <aurore/cadran/ui/cadran_frame.hpp>
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/cadran_list.hpp>
#include <aurore/cadran/ui/cadran_modal.hpp>
#include <aurore/cadran/ui/cadran_text.hpp>
#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/live_prefs.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/text_size.hpp>
#include <aurore/cadran/util/text.hpp>

#include <yoga/Yoga.h>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>

using namespace aurore::literals;

namespace ui
{

void on_cancel(aurore::Dialog* dialog, std::function<void()> on_cancel)
{
    dialog->setCancelable(true);
    dialog->getAppletFrame()->registerAction(
        "hints/back"_i18n, aurore::BUTTON_B,
        [dialog, on_cancel](aurore::View*)
        {
            dialog->close([on_cancel]()
                { if (on_cancel) on_cancel(); });
            return true;
        },
        false, false, aurore::SOUND_BACK);
}

void add_cancel(aurore::Dialog* dialog, std::function<void()> cancel)
{
    dialog->addButton("hints/cancel"_i18n, cancel);
    on_cancel(dialog, cancel);
}

// Height of the dialog's content above its button row (its card keeps a
// margin from the screen's top and bottom).
static float content_height()
{
    return aurore::Application::contentHeight - aurore::dp(cadran::DIALOG_BUTTONS + 2 * cadran::DIALOG_MARGIN);
}

static void scroll_with(aurore::Dialog* dialog, aurore::ScrollingFrame* frame, aurore::View* text)
{
    for (const aurore::ControllerButton button : { aurore::BUTTON_NAV_UP, aurore::BUTTON_NAV_DOWN })
    {
        const bool down = button == aurore::BUTTON_NAV_DOWN;
        dialog->getAppletFrame()->registerAction(
            "", button,
            [frame, text, down](aurore::View*)
            {
                const float page   = std::round(frame->getHeight() / 2);
                const float limit  = std::max(0.0f, text->getHeight() - frame->getHeight());
                const float target = std::min(std::max(frame->getContentOffsetY() + (down ? page : -page), 0.0f), limit);
                frame->setContentOffsetY(target, true);
                return true;
            },
            true, true, aurore::SOUND_NONE);
    }
}

namespace
{
    // A dialog's text that scrolls: its foot fades into the card while more
    // follows below the buttons' row.
    class DialogScroll : public aurore::ScrollingFrame
    {
      public:
        void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
            aurore::FrameContext* ctx) override
        {
            aurore::ScrollingFrame::draw(vg, x, y, width, height, style, ctx);
            cadran::kit::draw_more_below(vg, this, x, y, width, height, "cadran/surf");
        }
    };
} // namespace

aurore::Dialog* box_dialog(aurore::Box* body)
{
    // As wide as the dialog's card (aurore::Dialog): 720, 94% of a compact screen
    const float width = aurore::Application::getWidthClass() == aurore::WidthClass::COMPACT
        ? aurore::Application::contentWidth * 0.94f
        : 720.0f;
    YGNodeCalculateLayout(body->getYGNode(), width, YGUndefined, YGDirectionLTR);
    const float room = content_height();
    if (YGNodeLayoutGetHeight(body->getYGNode()) <= room)
        return new aurore::Dialog(body);
    // In a scrolling frame, each line keeps the height of its text (Aurore
    // caps a label at its parent's, which a scrolling body does not settle).
    std::function<void(aurore::View*)> unbounded = [&unbounded](aurore::View* view)
    {
        view->setShrink(0);
        if (dynamic_cast<aurore::Label*>(view))
            YGNodeStyleSetMaxHeight(view->getYGNode(), YGUndefined);
        if (auto* box = dynamic_cast<aurore::Box*>(view))
            for (aurore::View* child : box->getChildren())
                unbounded(child);
    };
    for (aurore::View* child : body->getChildren())
        unbounded(child);
    auto* frame = new DialogScroll();
    frame->setHeight(room);
    frame->setScrollingBehavior(aurore::ScrollingBehavior::NATURAL);
    frame->setContentView(body);
    auto* box = new aurore::Box(aurore::Axis::COLUMN);
    box->setAlignItems(aurore::AlignItems::STRETCH);
    box->addView(frame);
    auto* dialog = new aurore::Dialog(box);
    scroll_with(dialog, frame, body);
    return dialog;
}

// The text laid out by cadran::dialog_body, `below` under it (the card's
// width unless it aligns itself), scrolling when taller than the dialog's room.
static aurore::Dialog* body_dialog(const std::string& text, aurore::View* below = nullptr)
{
    aurore::Box* body = cadran::dialog_body(text);
    if (below)
    {
        if (!body->getChildren().empty())
            below->setMarginTop(aurore::dp(18));
        body->addView(below);
    }
    return box_dialog(body);
}

aurore::Dialog* dialog(const std::string& text)
{
    return body_dialog(text);
}

aurore::Dialog* dialog(const std::string& text, aurore::View* below)
{
    return body_dialog(text, below);
}

static void open_confirm(const std::string& body, const std::string& confirm_label,
    std::function<void()> on_yes, std::function<void()> on_no, bool danger)
{
    auto* dialog = body_dialog(body);
    dialog->addButton("hints/cancel"_i18n, [on_no]()
        { if (on_no) on_no(); });
    dialog->addButton(confirm_label, [on_yes]()
        { if (on_yes) on_yes(); });
    on_cancel(dialog, on_no);
    cadran::style_dialog(dialog, danger ? cadran::Emphasis::DANGER : cadran::Emphasis::PRIMARY);
    dialog->open();
}

void confirm(const std::string& body, const std::string& confirm_label,
    std::function<void()> on_yes, std::function<void()> on_no, bool danger)
{
    open_confirm(body, confirm_label, std::move(on_yes), std::move(on_no), danger);
}

void confirm_danger(const std::string& body, const std::string& confirm_label, std::function<void()> on_yes)
{
    open_confirm(body, confirm_label, std::move(on_yes), nullptr, true);
}

void show_failure(const cadran_text::Failure& f)
{
    const std::string& title = f.title;
    const std::string& rest  = f.reason;
    const std::string& code  = f.code;
    const bool report        = f.report;
    const std::string body   = title.empty() ? rest : rest.empty() ? title
                                                                   : title + "\n\n" + rest;
    auto* dialog             = body_dialog(body.empty() ? code : body);
    if (!code.empty() && !body.empty())
    {
        if (auto* box = dynamic_cast<aurore::Box*>(dialog->getView("cadran_dialog_body")))
        {
            aurore::Label* small = cadran::kit::label(code, 15, "cadran/text3");
            small->setMarginTop(aurore::dp(10));
            box->addView(small);
        }
    }
    if (report)
    {
        dialog->addButton("cadran/notices/send_report"_i18n, []()
            {
            if (hooks::get().send_report) hooks::get().send_report(); });
    }
    dialog->addButton("hints/ok"_i18n, []() {});
    dialog->setCancelable(true);
    cadran::style_dialog(dialog, cadran::Emphasis::FAILURE);
    // A closes it, as before: the report is a choice, not the default.
    if (report)
        if (aurore::View* ok = dialog->getView("aurore/dialog/button2"))
            dialog->setLastFocusedView(ok);
    dialog->open();
}

void info(const std::string& body)
{
    auto* dialog = ui::dialog(body);
    dialog->addButton("hints/ok"_i18n, []() {});
    dialog->open();
}

void qr_dialog(const std::string& body, const std::string& url)
{
    aurore::Box* code = cadran::kit::qr(url, 240, "", text::short_url(url));
    code->setAlignSelf(aurore::AlignSelf::CENTER);
    auto* dialog = body_dialog(body, code);
    dialog->addButton("hints/ok"_i18n, []() {});
    dialog->open();
}

aurore::Box* check_row(const CheckLine& line, float size)
{
    namespace kit = cadran::kit;
    auto* row     = new aurore::Box(aurore::Axis::ROW);
    row->setAlignItems(aurore::AlignItems::FLEX_START);
    const char* color = line.state == Check::Ok ? "cadran/teal"
        : line.state == Check::Warn             ? "cadran/amber"
        : line.state == Check::Bad              ? "cadran/coral"
                                                : "cadran/text3";
    auto* mark        = new kit::Mark(color);
    mark->setMarginTop(size * 0.3f);
    mark->setMarginRight(aurore::dp(14));
    mark->setShrink(0);
    row->addView(mark);
    aurore::Label* text = kit::label(line.text, size, line.state == Check::Info ? "cadran/text2" : "cadran/text");
    text->setGrow(1.0f);
    text->setWidth(0);
    row->addView(text);
    return row;
}

namespace
{
    // A panel of check lines, as dialog_body's tables: the surface, rounded.
    aurore::Box* check_panel()
    {
        auto* panel = new aurore::Box(aurore::Axis::COLUMN);
        panel->setAlignItems(aurore::AlignItems::STRETCH);
        panel->setThemeColor("backgroundColor", "cadran/surf2");
        panel->setCornerRadius(aurore::dp(14));
        panel->setPadding(aurore::dp(12), aurore::dp(18), aurore::dp(14), aurore::dp(18));
        return panel;
    }
} // namespace

aurore::Box* checklist_body(const std::string& title, const std::vector<CheckLine>& lines, const std::string& note,
    const std::string& subtitle)
{
    namespace kit = cadran::kit;
    auto* body    = new aurore::Box(aurore::Axis::COLUMN);
    body->setAlignItems(aurore::AlignItems::STRETCH);
    body->setPadding(aurore::dp(30), aurore::dp(40), aurore::dp(20), aurore::dp(40));
    body->addView(cadran::dialog_title(title));
    if (!subtitle.empty())
    {
        aurore::Label* sub = kit::label(subtitle, 17, "cadran/text2");
        sub->setMarginTop(aurore::dp(6));
        body->addView(sub);
    }
    // A Header line starts a panel of its own, its name small above it.
    aurore::Box* panel = nullptr;
    for (const CheckLine& line : lines)
    {
        if (line.state == Check::Header)
        {
            aurore::Label* head = kit::label(cadran::upper(line.text), 13, "cadran/text3");
            head->setMarginTop(aurore::dp(16));
            head->setMarginBottom(aurore::dp(6));
            head->setMarginLeft(aurore::dp(4));
            body->addView(head);
            panel = check_panel();
            body->addView(panel);
            continue;
        }
        if (!panel)
        {
            panel = check_panel();
            panel->setMarginTop(aurore::dp(16));
            body->addView(panel);
        }
        aurore::Box* row = check_row(line, 16.5f);
        row->setMarginTop(panel->getChildren().empty() ? 0 : aurore::dp(8));
        panel->addView(row);
    }
    if (!note.empty())
    {
        auto* foot = new aurore::Box(aurore::Axis::ROW);
        foot->setAlignItems(aurore::AlignItems::FLEX_START);
        foot->setMarginTop(aurore::dp(14));
        CadranIcon* icon = kit::icon("info", 16, "cadran/text3");
        icon->setMarginTop(aurore::dp(2));
        icon->setMarginRight(aurore::dp(10));
        icon->setShrink(0);
        foot->addView(icon);
        aurore::Label* small = kit::label(note, 15, "cadran/text3");
        small->setGrow(1.0f);
        small->setWidth(0);
        foot->addView(small);
        body->addView(foot);
    }
    return body;
}

void notice(const char* icon, Check tone, const std::string& title, const std::string& text)
{
    namespace kit     = cadran::kit;
    const char* color = tone == Check::Ok ? "cadran/teal" : tone == Check::Bad ? "cadran/coral"
        : tone == Check::Warn                                                  ? "cadran/amber"
                                                                               : "cadran/text2";
    const char* soft  = tone == Check::Ok ? "cadran/teal_soft" : tone == Check::Bad ? "cadran/coral_soft"
         : tone == Check::Warn                                                      ? "cadran/amber_surf"
                                                                                    : "cadran/surf2";
    auto* body        = new aurore::Box(aurore::Axis::COLUMN);
    body->setAlignItems(aurore::AlignItems::STRETCH);
    body->setPadding(aurore::dp(30), aurore::dp(40), aurore::dp(22), aurore::dp(40));
    auto* head = new aurore::Box(aurore::Axis::ROW);
    head->setAlignItems(aurore::AlignItems::CENTER);
    aurore::Box* badge = cadran::dialog_badge(icon, color, soft);
    badge->setMarginRight(aurore::dp(16));
    head->addView(badge);
    aurore::Label* label = cadran::dialog_title(title);
    label->setGrow(1.0f);
    label->setWidth(0);
    head->addView(label);
    body->addView(head);
    aurore::Label* words = kit::label(text, 18, "cadran/text2");
    words->setMarginTop(aurore::dp(14));
    body->addView(words);
    auto* dialog = box_dialog(body);
    dialog->addButton("hints/ok"_i18n, []() {});
    dialog->setCancelable(true);
    cadran::style_dialog(dialog, cadran::Emphasis::NONE);
    dialog->open();
}

namespace
{
    // How long A is held so far (hold_confirm); 0 once let go.
    struct Held
    {
        int64_t start = -1, last = 0;
        bool done     = false;
        bool touching = false; // a finger held on the button
        std::function<void()> full; // held long enough
    };
    constexpr int64_t HOLD_US = 2000000;
    // Aurore repeats a held key's action after 250 ms, then every 100 ms: a
    // longer gap is a key let go.
    constexpr int64_t LET_GO_US = 350000;

    double held_part(Held& h, int64_t now)
    {
        if (h.start < 0)
            return 0;
        if (now - h.last > LET_GO_US)
        {
            h.start = -1;
            return 0;
        }
        return std::min(1.0, (double)(now - h.start) / HOLD_US);
    }

    // Over the button held: a lighter fill from its left edge as it is held,
    // within its pill; gone when it is let go.
    class HoldFill : public aurore::View
    {
      public:
        explicit HoldFill(std::shared_ptr<Held> hold)
            : held(std::move(hold))
        {
            this->setFocusable(false);
            this->setPositionType(aurore::PositionType::ABSOLUTE);
            this->setPositionTop(0);
            this->setPositionLeft(0);
            this->setWidthPercentage(100);
            this->setHeightPercentage(100);
        }
        void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style, aurore::FrameContext* ctx) override
        {
            const int64_t now = aurore::getCPUTimeUsec();
            // A finger on the button: held as long as it stays (no key repeats).
            if (this->held->touching)
                this->held->last = now;
            const double part = held_part(*this->held, now);
            if (part >= 1 && !this->held->done && this->held->full)
            {
                this->held->done = true;
                // Not while drawing: closing the dialog changes the views drawn.
                aurore::sync([held = this->held]()
                    { held->full(); });
            }
            if (part <= 0)
                return;
            nvgSave(vg);
            nvgIntersectScissor(vg, x, y, (float)(width * part), height);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, width, height, height / 2);
            nvgFillColor(vg, ctx->theme["cadran/hold_fill"]);
            nvgFill(vg);
            nvgRestore(vg);
        }

      private:
        std::shared_ptr<Held> held;
    };
} // namespace

void hold_confirm(aurore::Box* body, const std::string& hold_label, std::function<void()> on_yes,
    std::function<void()> on_no)
{
    namespace kit = cadran::kit;
    auto held     = std::make_shared<Held>();
    // How: the A key's bubble, then the words; the bubble only for a
    // controller or a keyboard, not for a finger or the mouse.
    auto* how = new aurore::Box(aurore::Axis::ROW);
    how->setAlignItems(aurore::AlignItems::CENTER);
    how->setJustifyContent(aurore::JustifyContent::CENTER);
    how->setMarginTop(aurore::dp(16));
    if (aurore::Application::getInputType() == aurore::InputType::GAMEPAD)
    {
        aurore::Box* a = kit::key("A", 22);
        a->setMarginRight(aurore::dp(8));
        how->addView(a);
    }
    how->addView(kit::label(aurore::getStr("cadran/common/hold_hint", hold_label), 15, "cadran/text2"));
    body->addView(how);
    auto* dialog = box_dialog(body);
    dialog->addButton("hints/cancel"_i18n, [on_no]()
        { if (on_no) on_no(); });
    dialog->addButton(hold_label, []() {});
    on_cancel(dialog, on_no);
    cadran::style_dialog(dialog, cadran::Emphasis::PRIMARY);
    held->full = [dialog, on_yes]()
    { dialog->close([on_yes]()
          { if (on_yes) on_yes(); }); };
    if (auto* yes = dynamic_cast<aurore::Box*>(dialog->getView("aurore/dialog/button2")))
    {
        yes->addView(new HoldFill(held));
        yes->registerAction(hold_label, aurore::BUTTON_A, [held](aurore::View*)
            {
            const int64_t now = aurore::getCPUTimeUsec();
            if (held->done) return true;
            if (held->start < 0 || now - held->last > LET_GO_US) held->start = now;
            held->last = now;
            if (now - held->start >= HOLD_US) {
                held->done = true;
                held->full();
            }
            return true; }, false, true, aurore::SOUND_NONE);
        // A finger held on it does the same; lifted or moved away, it stops.
        yes->addGestureRecognizer(new aurore::TapGestureRecognizer([held](aurore::TapGestureStatus status, aurore::Sound*)
            {
            const bool down = status.state == aurore::GestureState::UNSURE || status.state == aurore::GestureState::START ||
                              status.state == aurore::GestureState::STAY;
            if (down && !held->touching) held->start = held->last = aurore::getCPUTimeUsec();
            held->touching = down; }));
    }
    dialog->open();
}

void prompt_text(const std::string& header, const std::string& initial, int max_len,
    std::function<void(std::string)> on_value)
{
    aurore::Application::getImeManager()->openForText(
        [on_value](std::string text)
        { if (!text.empty()) on_value(text); },
        header, "", max_len, initial);
}

namespace
{
    template <size_t N>
    int index_of(const char* const (&list)[N], const std::string& value)
    {
        for (size_t i = 0; i < N; i++)
            if (value == list[i])
                return (int)i;
        return 0;
    }

    // Saves `value` in `field`, then applies it at once (`apply`: every screen
    // made again, the language), or refreshes when nothing changed.
    void choose(std::string& field, const char* value, std::function<void()> after, void (*apply)())
    {
        if (field == value)
        {
            if (after)
                after();
            return;
        }
        field = value;
        save_prefs();
        apply();
    }
} // namespace

// Each language's flag (ui::LANGUAGES' order), from the Twemoji set of
// img/emoji/: the console's own a globe; Latin American Spanish, Mexico's.
constexpr const char* LANGUAGE_FLAGS[] = { "1f310", "1f1fa-1f1f8", "1f1eb-1f1f7", "1f1e8-1f1e6",
    "1f1e9-1f1ea", "1f1ea-1f1f8", "1f1f2-1f1fd", "1f1ee-1f1f9",
    "1f1f3-1f1f1", "1f1f5-1f1f9", "1f1e7-1f1f7", "1f1f7-1f1fa",
    "1f1ef-1f1f5", "1f1f0-1f1f7", "1f1e8-1f1f3", "1f1f9-1f1fc" };
static_assert(std::size(LANGUAGE_FLAGS) == std::size(ui::LANGUAGES), "a flag for each language");

void pick_language(std::function<void()> after)
{
    std::vector<std::string> labels, flags;
    for (const char* l : ui::LANGUAGES)
        labels.push_back(aurore::getStr(std::string("cadran/tools/languages/") + l));
    for (const char* f : LANGUAGE_FLAGS)
        flags.push_back(std::string("img/emoji/") + f + ".png");
    pick("cadran/tools/language"_i18n, labels, index_of(ui::LANGUAGES, ui::prefs().language), [after](int i)
        { choose(ui::prefs().language, ui::LANGUAGES[i], after, live_prefs::language_changed); }, true, {}, flags);
}

namespace
{
    // The theme files the palette picker offers: Aurore's presets, then the
    // user's (ui::hooks' themes_dir), the readable ones.
    std::vector<aurore::ThemeInfo> theme_files()
    {
        std::vector<aurore::ThemeInfo> files = aurore::Theme::builtinThemes();
        if (ui::hooks::get().themes_dir)
        {
            for (aurore::ThemeInfo& own : aurore::Theme::listThemes(ui::hooks::get().themes_dir()))
            {
                if (own.ok())
                    files.push_back(std::move(own));
                else
                    aurore::Logger::warning("theme {} left out: {}", own.path, own.errors.front());
            }
        }
        return files;
    }

    void theme_chosen(std::function<void()> after)
    {
        save_prefs();
        live_prefs::apply_theme();
        if (after)
            after();
    }

    std::string name_of(const std::string& file)
    {
        if (!file.empty())
            for (const aurore::ThemeInfo& info : theme_files())
                if (info.path == file)
                    return info.name;
        return "cadran/tools/palettes/default"_i18n;
    }

    // The palette to show `variant` in: the one in use when it has that
    // variant; else the counterpart it names, when that one has it; else the
    // boards' own, which have both.
    std::string palette_for(aurore::ThemeVariant variant)
    {
        const std::string& file = ui::prefs().theme_file;
        if (file.empty())
            return file;
        const aurore::ThemeInfo info = aurore::Theme::inspect(file);
        if (info.styles(variant))
            return file;
        if (!info.counterpart.empty() && aurore::Theme::inspect(info.counterpart).styles(variant))
            return info.counterpart;
        return "";
    }

    aurore::ThemeVariant variant_of(const std::string& theme)
    {
        return theme == "light" ? aurore::ThemeVariant::LIGHT : aurore::ThemeVariant::DARK;
    }

    // A palette's row in its picker: its own colours, in the variant on screen
    // when it styles it, else in the one it styles; its motif in its accent; or
    // its `preview` picture.
    ui::Swatch swatch_of(const aurore::ThemeInfo& info)
    {
        const aurore::ThemeVariant shown    = aurore::Application::getThemeVariant();
        const aurore::ThemeVariant variant  = info.styles(shown) ? shown : info.variant();
        const aurore::ThemeInfo::Preview& p = variant == aurore::ThemeVariant::LIGHT ? info.lightPreview : info.darkPreview;
        // Its own thumbnail, when it has one (loaded off the UI thread, the
        // colours shown meanwhile: CadranSwatch)
        return { p.background, p.surface, p.accent, info.icon, info.pictures(variant).preview };
    }

    // Default's: the app's own colours (without a theme file), a palette.
    ui::Swatch default_swatch()
    {
        const auto& base = aurore::Theme::get(aurore::Application::getThemeVariant()).getValues()->getBaseColors();
        auto color       = [&base](const char* key, const char* fallback)
        {
            for (const char* name : { key, fallback })
            {
                auto it = base.find(name);
                if (it != base.end())
                    return it->second;
            }
            return nvgRGB(0, 0, 0);
        };
        // The keys of ThemeInfo's previews (core/theme.cpp)
        return { color("aurore/background", "aurore/clear"), color("cadran/surf", "aurore/highlight/background"),
            color("aurore/sidebar/active_item", "aurore/accent"), "palette" };
    }
} // namespace

std::string palette_name()
{
    return name_of(ui::prefs().theme_file);
}

void pick_palette(std::function<void()> after)
{
    const std::vector<aurore::ThemeInfo> files = theme_files();
    std::vector<std::string> labels = { "cadran/tools/palettes/default"_i18n }, paths = { "" };
    // A palette with one variant switches to it ("" : both, the variant kept).
    std::vector<std::string> only    = { "" };
    std::vector<ui::Swatch> swatches = { default_swatch() };
    int selected                     = 0;
    for (const aurore::ThemeInfo& info : files)
    {
        if (info.path == ui::prefs().theme_file)
            selected = (int)labels.size();
        // A user's theme named as one above it: told apart by its file
        if (std::find(labels.begin(), labels.end(), info.name) != labels.end())
            labels.push_back(info.name + " (" + info.id + ")");
        else
            labels.push_back(info.name);
        paths.push_back(info.path);
        only.push_back(info.light == info.dark ? "" : info.light ? "light"
                                                                 : "dark");
        swatches.push_back(swatch_of(info));
    }
    pick("cadran/tools/palette"_i18n, labels, selected, [after, paths, only](int i)
        {
        ui::prefs().theme_file = paths[i];
        if (!only[i].empty()) ui::prefs().theme = only[i];
        theme_chosen(after); }, true, {}, {}, swatches);
}

void pick_theme(std::function<void()> after)
{
    std::vector<std::string> labels, details;
    for (const char* t : ui::THEMES)
    {
        labels.push_back(aurore::getStr(std::string("cadran/tools/themes/") + t));
        // A variant the palette has not: the palette it switches to.
        const std::string theme = t;
        const std::string file  = theme == "system" ? ui::prefs().theme_file : palette_for(variant_of(theme));
        details.push_back(file != ui::prefs().theme_file ? name_of(file) : "");
    }
    // Its colours, after the variants: a picker of their own.
    labels.push_back("cadran/tools/palette"_i18n);
    details.push_back(palette_name());
    // As the header's button draws them (view/header_settings.cpp).
    const std::vector<std::string> icons = { "contrast", "sun", "moon", "sparkle" };
    static_assert(std::size(ui::THEMES) == 3, "an icon for each theme");
    pick("cadran/tools/theme"_i18n, labels, index_of(ui::THEMES, ui::prefs().theme), [after](int i)
        {
        if (i == (int)std::size(ui::THEMES)) {
            pick_palette(after);
            return;
        }
        const std::string theme = ui::THEMES[i];
        const std::string file = theme == "system" ? ui::prefs().theme_file : palette_for(variant_of(theme));
        if (ui::prefs().theme == theme && ui::prefs().theme_file == file) {
            if (after) after();
            return;
        }
        ui::prefs().theme = theme;
        ui::prefs().theme_file = file;
        theme_chosen(after); }, true, details, icons);
}

void pick_text_size(std::function<void()> after)
{
    std::vector<std::string> labels;
    for (const char* t : ui::TEXT_SIZES)
        labels.push_back(aurore::getStr(std::string("cadran/prefs/text_sizes/") + t));
    const std::vector<std::string> icons = { "tv", "text-normal", "text-large", "text-largest" };
    static_assert(std::size(ui::TEXT_SIZES) == 4, "an icon for each text size");
    pick("cadran/prefs/text_size"_i18n, labels, index_of(ui::TEXT_SIZES, ui::prefs().text_size), [after](int i)
        {
        // At once, no restart: every screen is laid out again.
        ui::prefs().text_size = ui::TEXT_SIZES[i];
        save_prefs();
        text_size::apply();
        if (after) after(); }, true, {}, icons);
}

void pick_rail(std::function<void()> after)
{
    std::vector<std::string> labels;
    for (const char* r : ui::RAILS)
        labels.push_back(aurore::getStr(std::string("cadran/prefs/rail_values/") + r));
    const std::vector<std::string> icons = { "rail-auto", "rail-open", "rail-icons" };
    static_assert(std::size(ui::RAILS) == 3, "an icon for each sidebar");
    pick("cadran/prefs/rail"_i18n, labels, index_of(ui::RAILS, ui::prefs().rail), [after](int i)
        {
        // At once: the main screen's rail wider or narrower.
        ui::prefs().rail = ui::RAILS[i];
        save_prefs();
        cadran::apply_rail();
        if (after) after(); }, true, {}, icons);
}

std::string rail_name()
{
    return aurore::getStr("cadran/prefs/rail_values/" + std::string(ui::RAILS[index_of(ui::RAILS, ui::prefs().rail)]));
}

std::string text_size_name(bool tile)
{
    if (tile && ui::prefs().text_size == std::string("auto"))
        return "cadran/tiles/text_size_auto"_i18n;
    return aurore::getStr("cadran/prefs/text_sizes/" + std::string(ui::TEXT_SIZES[index_of(ui::TEXT_SIZES, ui::prefs().text_size)]));
}

std::string language_name(bool tile)
{
    // The names of a language spoken in two places, shorter on a tile: the
    // place as its code.
    static const std::pair<const char*, const char*> SHORT[] = {
        { "fr", "Français (FR)" },
        { "fr-CA", "Français (CA)" },
        { "es", "Español (ES)" },
        { "es-419", "Español (LatAm)" },
        { "pt", "Português (PT)" },
        { "pt-BR", "Português (BR)" },
    };
    if (tile)
        for (const auto& [code, name] : SHORT)
            if (ui::prefs().language == code)
                return name;
    return aurore::getStr("cadran/tools/languages/" + std::string(ui::LANGUAGES[index_of(ui::LANGUAGES, ui::prefs().language)]));
}

std::string theme_name(bool tile)
{
    return aurore::getStr(std::string(tile ? "cadran/tiles/themes/" : "cadran/tools/themes/") + std::string(ui::THEMES[index_of(ui::THEMES, ui::prefs().theme)]));
}

void quick_settings(std::function<void()> after)
{
    const std::vector<std::string> labels = { "cadran/tools/language"_i18n + " · " + language_name(),
        "cadran/tools/theme"_i18n + " · " + theme_name(),
        "cadran/prefs/text_size"_i18n + " · " + text_size_name() };
    // As an app's Preferences tiles draw them.
    const std::vector<std::string> icons = { "language", "contrast", "eye" };
    pick("cadran/quick/title_text"_i18n, labels, 0, [after](int i)
        {
        if (i == 0) pick_language(after);
        else if (i == 1) pick_theme(after);
        else pick_text_size(after); }, false, {}, icons);
}

} // namespace ui
