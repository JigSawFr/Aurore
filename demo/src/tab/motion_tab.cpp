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

#include "tab/motion_tab.hpp"

#include <aurore/cadran.hpp>

#include "demo_app.hpp"
#include "tab/transform_tab.hpp"
#include "view/charts.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
struct Easing
{
    aurore::EasingFunction function;
    const char* name;
};

const Easing EASINGS[] = {
    { aurore::EasingFunction::cubicOut, "cubicOut" },
    { aurore::EasingFunction::backOut, "backOut" },
    { aurore::EasingFunction::elasticOut, "elasticOut" },
    { aurore::EasingFunction::bounceOut, "bounceOut" },
};

// A tile the focus can land on, to watch the highlight move
aurore::Box* tile(const std::string& text)
{
    aurore::Box* box = show::card(true, true);
    box->setHeight(84);
    box->setJustifyContent(aurore::JustifyContent::CENTER);
    box->setAlignItems(aurore::AlignItems::CENTER);
    box->addView(kit::label(text, kit::TEXT_BODY, "cadran/text"));
    return box;
}
} // namespace

MotionTab::MotionTab()
{
    aurore::Box* page = show::page(this);

    // Easings, each drawn as its curve, a dot running along it
    aurore::Box* easings = show::section(page, "bolt", "demo/motion/easings"_i18n, "demo/motion/easings_lead"_i18n);
    aurore::Box* plots   = show::row();
    for (const Easing& easing : EASINGS)
    {
        aurore::Box* card = show::card();
        card->setPadding(16, 18, 14, 18);
        auto* plot = new EasingPlot(easing.function);
        plot->setHeight(110);
        this->plots.push_back(plot);
        card->addView(plot);
        aurore::Label* name = kit::label(easing.name, kit::TEXT_NOTE, "cadran/text2");
        name->setMarginTop(8);
        card->addView(show::centred(name));
        plots->addView(card);
    }
    easings->addView(plots);
    aurore::Box* play = show::row(true, 20);
    play->setMarginTop(show::GAP);
    play->setAlignItems(aurore::AlignItems::CENTER);
    play->addView(show::button("demo/motion/play"_i18n, [this]
        { this->play(); }, true));
    aurore::Label* how = show::note("demo/motion/play_note"_i18n);
    how->setMarginTop(0);
    how->setGrow(1);
    how->setWidth(0);
    play->addView(how);
    easings->addView(play);

    // State styles and transitions, from XML alone
    aurore::Box* states = show::section(page, "sparkle", "demo/styles/states"_i18n, "demo/motion/states_lead"_i18n);
    states->addView(aurore::View::createFromXMLResource("views/states.xml"));
    aurore::Box* snippet = show::code("<aurore:Box focusable=\"true\"\n"
                                      "    backgroundColor=\"@theme/cadran/surf\"\n"
                                      "    focus:backgroundColor=\"@theme/cadran/teal_fill\"\n"
                                      "    pressed:scale=\"0.94\"\n"
                                      "    transition=\"backgroundColor 220ms ease-out, scale 120ms\" />");
    snippet->setMarginTop(show::GAP);
    states->addView(snippet);

    // The focus: it glides, and shakes where it cannot go
    aurore::Box* focus = show::section(page, "eye", "demo/motion/focus"_i18n, "demo/motion/focus_lead"_i18n);
    aurore::Box* tiles = show::row();
    for (int i = 1; i <= 4; i++)
        tiles->addView(tile(std::to_string(i)));
    focus->addView(tiles);
    aurore::Box* shake = show::row(true, 20);
    shake->setMarginTop(show::GAP);
    shake->setAlignItems(aurore::AlignItems::CENTER);
    aurore::Button* button = show::button("demo/motion/shake"_i18n, [] {});
    button->registerClickAction([](aurore::View* view)
        {
        view->shakeHighlight(aurore::FocusDirection::RIGHT);
        return true; });
    shake->addView(button);
    aurore::Label* edge = show::note("demo/motion/shake_note"_i18n);
    edge->setMarginTop(0);
    edge->setGrow(1);
    edge->setWidth(0);
    shake->addView(edge);
    focus->addView(shake);

    // Reduced motion, and the transform lab
    aurore::Box* more = show::section(page, "settings", "demo/motion/more"_i18n);
    aurore::Box* rows = cadran::List::rows_in(more);
    auto* reduce      = new aurore::BooleanCell();
    reduce->init("demo/styles/reduce_motion"_i18n, aurore::Application::isReduceMotionSet(), [](bool on)
        { aurore::Application::setReduceMotion(on); });
    cadran::List::row(reduce, "pause");
    rows->addView(reduce);
    auto* lab = new aurore::DetailCell();
    lab->setText("demo/motion/transform"_i18n);
    lab->setDetailText("demo/motion/transform_detail"_i18n + "  ›");
    lab->registerClickAction([](aurore::View*)
        {
        demo::pushScreen(TransformTab::create(), "demo/motion/transform"_i18n);
        return true; });
    cadran::List::row(lab, "refresh");
    rows->addView(lab);
}

void MotionTab::play()
{
    this->progress.stop();
    this->progress.reset(0.0f);
    this->progress.addStep(1.0f, 1400, aurore::EasingFunction::linear);
    this->progress.setTickCallback([this]
        {
        for (EasingPlot* plot : this->plots)
            plot->setProgress(this->progress.getValue());
        aurore::Application::requestRedraw(); });
    this->progress.start();
}

aurore::View* MotionTab::create()
{
    return new MotionTab();
}
