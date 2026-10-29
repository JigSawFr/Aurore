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

#include "tab/themes_tab.hpp"

#include <aurore/cadran.hpp>

#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// A palette's tile: a swatch of its colours, its name and who made it; the
// one in use outlined. Chosen, it is used at once, nothing built again.
aurore::Box* palette(ThemesTab* tab, const aurore::ThemeInfo& info)
{
    aurore::Box* tile = kit::card(true, false);
    tile->setAxis(aurore::Axis::ROW);
    tile->setAlignItems(aurore::AlignItems::CENTER);
    tile->setPadding(12, 16, 12, 12);
    tile->setGap(14);
    tile->setWidth(271.5f); // four to a row
    tile->setMinHeight(72);
    show::share(tile, 1);

    CadranSwatch::Colors colors;
    if (info.path.empty())
    {
        // Cadran's own: its navy or its cool white
        aurore::Theme theme = aurore::Application::getTheme();
        colors.background   = theme["cadran/surf"];
        colors.ring         = theme["cadran/line"];
        colors.ink          = theme["cadran/teal"];
        colors.motif        = "palette";
    }
    else
    {
        bool dark                                 = aurore::Application::getThemeVariant() == aurore::ThemeVariant::DARK ? info.dark : !info.light;
        const aurore::ThemeInfo::Preview& preview = dark ? info.darkPreview : info.lightPreview;
        colors.background                         = preview.background;
        colors.ring                               = preview.surface;
        colors.ink                                = preview.accent;
        colors.motif                              = info.icon;
    }
    auto* swatch = new CadranSwatch(colors);
    swatch->setWidth(44);
    swatch->setHeight(44);
    swatch->setShrink(0);
    tile->addView(swatch);

    aurore::Box* names = show::column(2);
    names->setGrow(1);
    names->setWidth(0);
    names->addView(kit::label(info.name, kit::TEXT_BODY, "cadran/text"));
    std::string variants  = info.path.empty() || (info.light && info.dark) ? "demo/themes/both"_i18n
         : info.dark                                                       ? "demo/themes/dark_only"_i18n
                                                                           : "demo/themes/light_only"_i18n;
    aurore::Label* detail = kit::label(variants, kit::TEXT_META, "cadran/text2");
    detail->setSingleLine(true);
    detail->setShrinkToFit(true);
    names->addView(detail);
    tile->addView(names);

    std::string path = info.path;
    tile->registerClickAction([tab, path](aurore::View*)
        {
        ui::prefs().theme_file = path;
        ui::save_prefs();
        live_prefs::apply_theme();
        tab->markChosen();
        return true; });
    tile->addGestureRecognizer(new aurore::TapGestureRecognizer(tile));
    return tile;
}
} // namespace

ThemesTab::ThemesTab()
{
    aurore::Box* page = show::page(this);

    // Light or dark
    aurore::Box* variant = show::section(page, "contrast", "demo/themes/variant"_i18n, "demo/themes/variant_lead"_i18n);
    aurore::Box* row     = show::row(true, 12);
    for (const char* theme : ui::THEMES)
    {
        std::string name  = aurore::getStr(std::string("cadran/tools/themes/") + theme);
        const char* icon  = std::string(theme) == "light" ? "sun" : std::string(theme) == "dark" ? "moon"
                                                                                                 : "tv";
        aurore::Box* tile = show::choice(icon, name, [this, theme]
            {
            ui::prefs().theme = theme;
            ui::save_prefs();
            live_prefs::apply_theme();
            this->markChosen(); });
        this->variants.emplace_back(tile, theme);
        row->addView(tile);
    }
    variant->addView(row);

    // The palettes: Cadran's, Aurore's presets, the demo's theme files
    aurore::Box* palettes = show::section(page, "palette", "demo/themes/palettes"_i18n, "demo/themes/palettes_lead"_i18n);
    aurore::Box* tiles    = show::row(true, 12);
    aurore::ThemeInfo none;
    none.name  = "demo/themes/default"_i18n;
    none.light = none.dark             = true;
    std::vector<aurore::ThemeInfo> all = { none };
    for (aurore::ThemeInfo& info : aurore::Theme::listThemes("@res/themes"))
        if (info.ok())
            all.push_back(std::move(info));
    for (aurore::ThemeInfo& info : aurore::Theme::builtinThemes())
        all.push_back(std::move(info));
    for (const aurore::ThemeInfo& info : all)
    {
        aurore::Box* tile = palette(this, info);
        this->palettes.emplace_back(tile, info.path);
        tiles->addView(tile);
    }
    palettes->addView(tiles);

    // A theme of one's own
    aurore::Box* file = show::section(page, "file", "demo/themes/file"_i18n, "demo/themes/file_lead"_i18n);
    aurore::Box* code = show::row();
    code->setAlignItems(aurore::AlignItems::STRETCH);
    aurore::Box* three = show::code("# mint.theme\n"
                                    "name = Mint\n"
                                    "inherit = dark\n"
                                    "accent = #3ddc97");
    three->setGrow(1);
    three->setWidth(0);
    code->addView(three);
    aurore::Box* more = show::code("# A preset, then two keys of its own\n"
                                   "inherit = builtin:ocean\n"
                                   "cadran/teal = #7fd1ff\n"
                                   "background_image = night.jpg");
    more->setGrow(1);
    more->setWidth(0);
    code->addView(more);
    file->addView(code);
    aurore::Box* picker = show::row(true, 20);
    picker->setMarginTop(show::gap());
    picker->setAlignItems(aurore::AlignItems::CENTER);
    picker->addView(show::iconButton("palette", "demo/themes/picker"_i18n, [this]
        { ui::pick_palette([this]
              { this->markChosen(); }); }));
    aurore::Label* checked = show::note("demo/themes/contrast"_i18n);
    checked->setMarginTop(0);
    checked->setGrow(1);
    checked->setWidth(0);
    picker->addView(checked);
    file->addView(picker);

    this->markChosen();
}

void ThemesTab::markChosen()
{
    for (auto& [tile, path] : this->palettes)
        show::markChoice(tile, path == ui::prefs().theme_file);
    for (auto& [tile, theme] : this->variants)
        show::markChoice(tile, theme == ui::prefs().theme);
}

aurore::View* ThemesTab::create()
{
    return new ThemesTab();
}
