// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/view/version_badge.hpp>

#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/ui.hpp>

#include <algorithm>

static constexpr const char* BADGE_ID = "version_badge";
static constexpr const char* TOOLS_ID = "header_tools";

VersionBadge::VersionBadge()
    : aurore::Box(aurore::Axis::ROW)
{
    this->setId(BADGE_ID);
    this->setFocusable(false);
    this->setAlignItems(aurore::AlignItems::CENTER);
    this->setAlignSelf(aurore::AlignSelf::CENTER);
    this->setMarginLeft(16);
    // Level with the title, which Aurore moves down by this much, less 8:
    // centred like the title's box, the tag and the commit sat 4 px below
    // the middle of its capitals (the larger font has more room under them),
    // and a centred box moves by half its margin.
    this->setMarginTop(aurore::getStyle()["aurore/applet_frame/header_title_top_offset"] - 8);

    // The app's (ui::hooks): its version, its commit, a release or not.
    const auto& h             = ui::hooks::get();
    const std::string version = h.version ? h.version() : std::string(), commit = h.commit ? h.commit() : std::string();
    const bool release = !h.is_release || h.is_release();

    // The tag: filled (the amber of the warnings, or the notes' grey for a
    // release), the version in the background's colour.
    const char* ink = release ? "cadran/text3" : "cadran/amber";
    auto* tag       = new aurore::Box(aurore::Axis::ROW);
    tag->setThemeColor("backgroundColor", ink);
    tag->setCornerRadius(4);
    tag->setPadding(2, 8, 2, 8);
    auto* number = new aurore::Label();
    number->setText(version);
    number->setFontSize(18);
    number->setFontScaled(false); // a tag: the same size with the larger text
    number->setSingleLine(true);
    number->setThemeColor("textColor", "aurore/background");
    tag->addView(number);
    this->addView(tag);
    if (release || commit.empty())
        return;
    auto* text = new aurore::Label();
    text->setFontSize(18);
    text->setFontScaled(false);
    text->setSingleLine(true);
    text->setMarginLeft(10);
    text->setText(commit);
    text->setThemeColor("textColor", ink);
    this->addView(text);
}

// Adds the badge right after the title of the screen holding `focus` (its
// root view: Aurore gives a new screen the focus before putting it on the
// activity stack), unless it has one. Screens without the applet frame's
// header (the loading screen, dialogs) are left alone.
static void badge_screen_of(aurore::View* focus)
{
    if (!focus)
        return;
    while (focus->hasParent())
        focus = focus->getParent();
    auto* frame = dynamic_cast<aurore::AppletFrame*>(focus);
    // Not on a screen of its own (a child's: a version means nothing to a
    // child, and its amber reads as a warning).
    if (cadran::is_standalone_screen(frame))
        return;
    // An app that says no version (ui::hooks' version) gets no badge.
    const auto& version = ui::hooks::get().version;
    if (!version || version().empty())
        return;
    auto* label = frame ? frame->getView("aurore/applet_frame/title_label") : nullptr;
    auto* row   = label ? dynamic_cast<aurore::Box*>(label->getParent()) : nullptr;
    if (!row || row->getView(BADGE_ID))
        return;
    const auto& views = row->getChildren();
    const auto at     = std::find(views.begin(), views.end(), label) - views.begin() + 1;
    row->addView(new VersionBadge(), at);
}

aurore::Box* VersionBadge::header_tools(aurore::AppletFrame* frame)
{
    auto* header = frame ? dynamic_cast<aurore::Box*>(frame->getView("aurore/applet_frame/header")) : nullptr;
    if (!header)
        return nullptr;
    if (auto* tools = dynamic_cast<aurore::Box*>(header->getView(TOOLS_ID)))
        return tools;
    auto* tools = new aurore::Box(aurore::Axis::ROW);
    tools->setId(TOOLS_ID);
    tools->setFocusable(false);
    tools->setAlignItems(aurore::AlignItems::CENTER);
    tools->setAlignSelf(aurore::AlignSelf::CENTER);
    // 8 px more above and below, given back by the margins: the header's
    // settings take a touch that far from their drawing (HeaderSetting::hitTest)
    tools->setPaddingTop(8);
    tools->setPaddingBottom(8);
    tools->setMarginTop(-8);
    tools->setMarginBottom(-8);
    header->addView(tools);
    return tools;
}

void VersionBadge::install()
{
    static bool done = false;
    if (done)
        return;
    done = true;
    aurore::Application::getGlobalFocusChangeEvent()->subscribe(badge_screen_of);
}
