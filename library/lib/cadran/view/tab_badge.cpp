// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/view/tab_badge.hpp>

#include <string>

#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/ui/effects.hpp>
#include <aurore/cadran/ui/motion.hpp>
#include <aurore/cadran/ui/ui.hpp>
#include <aurore/cadran/view/cadran_icon.hpp>

static constexpr float TAB_BADGE_SIZE     = 26;
static constexpr const char* TAB_BADGE_ID = "tab_badge";
static constexpr float TAB_BADGE_POP_US   = 380000.0f;
static constexpr float DOCKED             = 18; // on the icon's corner

TabBadge::TabBadge()
{
    this->setId(TAB_BADGE_ID);
    this->setFocusable(false);
    this->setWidth(TAB_BADGE_SIZE);
    this->setHeight(TAB_BADGE_SIZE);
    this->setAlignSelf(aurore::AlignSelf::CENTER);
    this->setMarginRight(16);
    this->setVisibility(aurore::Visibility::GONE);
}

void TabBadge::set_count(int n)
{
    if (n == this->count)
        return;
    // A number that comes or changes pops in (one that goes takes the badge
    // with it).
    if (n > 0 && effects::motion())
        this->pop_since = motion::now();
    this->count = n;
    this->setVisibility(n > 0 ? aurore::Visibility::VISIBLE : aurore::Visibility::GONE);
}

void TabBadge::dock(float t, bool out_of_row)
{
    this->open = t;
    if (out_of_row == this->floating)
        return;
    this->floating = out_of_row;
    this->setPositionType(out_of_row ? aurore::PositionType::ABSOLUTE : aurore::PositionType::RELATIVE);
}

void TabBadge::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    (void)style;
    (void)ctx;
    if (this->count <= 0)
        return;
    const std::string n = this->count > 9 ? "9+" : std::to_string(this->count);
    float r = height * 0.5f, cx = x + width * 0.5f, cy = y + height * 0.5f;
    // Docked on the icon of its sidebar item: its top right corner.
    CadranIcon* glyph = nullptr;
    if (this->open < 1.0f && this->hasParent())
        for (aurore::View* v : this->getParent()->getChildren())
            if ((glyph = dynamic_cast<CadranIcon*>(v)))
                break;
    if (glyph)
    {
        const aurore::Rect at = glyph->getFrame();
        const float t         = this->open;
        cx                    = at.getMaxX() - 1 + (cx - at.getMaxX() + 1) * t;
        cy                    = at.getMinY() + 1 + (cy - at.getMinY() - 1) * t;
        r                     = DOCKED * 0.5f + (r - DOCKED * 0.5f) * t;
    }
    nvgSave(vg);
    if (this->pop_since)
    {
        const float t = motion::progress(this->pop_since, TAB_BADGE_POP_US);
        if (t >= 1.0f)
        {
            this->pop_since = 0;
        }
        else
        {
            const float scale = motion::back_out(t);
            nvgTranslate(vg, cx, cy);
            nvgScale(vg, scale, scale);
            nvgTranslate(vg, -cx, -cy);
        }
    }
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, r);
    nvgFillColor(vg, cadran::kit::color("cadran/amber"));
    nvgFill(vg);
    nvgFontFaceId(vg, aurore::Application::getDefaultFont());
    nvgFontSize(vg, r * 1.3f);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, aurore::Application::getTheme()["brls/background"]);
    nvgText(vg, cx, cy + r * 0.06f, n.c_str(), nullptr);
    nvgRestore(vg);
}

TabBadge* TabBadge::on_row(aurore::Box* row)
{
    if (!row)
        return nullptr;
    if (auto* badge = dynamic_cast<TabBadge*>(row->getView(TAB_BADGE_ID)))
        return badge;
    auto* badge = new TabBadge();
    // Last, so it lines up at the right whatever the value (an empty one
    // still takes room).
    badge->setMarginLeft(12);
    badge->setMarginRight(0);
    row->addView(badge);
    return badge;
}

TabBadge* TabBadge::find(aurore::SidebarItem* item)
{
    return item ? dynamic_cast<TabBadge*>(item->getView(TAB_BADGE_ID)) : nullptr;
}

TabBadge* TabBadge::of(aurore::SidebarItem* item)
{
    if (!item)
        return nullptr;
    if (auto* badge = dynamic_cast<TabBadge*>(item->getView(TAB_BADGE_ID)))
        return badge;
    auto* badge = new TabBadge();
    item->addView(badge);
    return badge;
}
