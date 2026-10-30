// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "tab/developers_tab.hpp"

#include <aurore/cadran.hpp>
#include <aurore/core/inspector.hpp>
#include <aurore/core/profiler.hpp>

#include <cmath>

#include "demo_app.hpp"
#include "simulation.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// The swatch's colours: theme keys, so a theme can change them
const char* const SWATCHES[] = { "cadran/teal", "cadran/blue", "cadran/violet", "cadran/amber" };

// The model the tab's bind= attributes read: it lives as long as the app,
// the tab may be built again (a new language).
struct StylesModel
{
    aurore::Observable<int> counter { 0 };
    aurore::Observable<NVGcolor> swatch;
    aurore::Observable<bool> even { true };
    aurore::Observable<std::string> announced { aurore::getStr("demo/styles/announced_hint") };
    aurore::DataModel model;

    StylesModel()
    {
        model.add("counter", counter);
        model.add("swatch", swatch);
        model.add("even", even);
        model.add("announced", announced);

        this->updateSwatch();
        aurore::Application::getThemeChangedEvent()->subscribe([this](aurore::ThemeVariant)
            { this->updateSwatch(); });
    }

    void updateSwatch()
    {
        swatch = aurore::Application::getTheme()[SWATCHES[counter.get() % 4]];
    }
};

StylesModel& stylesModel()
{
    static StylesModel model;
    return model;
}

// A figure of the frame: its value large, what it is under it
CadranDigits* figure(aurore::Box* row, const std::string& caption)
{
    aurore::Box* card = kit::top_card(true);
    card->setPadding(16, 20, 16, 20);
    auto* digits = new CadranDigits();
    digits->setText("–");
    digits->setFontSize(32);
    digits->setSmallUnits(true);
    digits->setThemeColor("textColor", "cadran/text");
    digits->setHeight(42);
    card->addView(digits);
    aurore::Label* label = kit::label(caption, kit::TEXT_NOTE, "cadran/text2");
    label->setSingleLine(true);
    label->setShrinkToFit(true);
    label->setShrinkToFitMin(11);
    card->addView(label);
    row->addView(show::share(card, 2));
    return digits;
}

aurore::BooleanCell* toggle(aurore::Box* rows, const std::string& title, const char* icon, bool on,
    std::function<void(bool)> changed)
{
    auto* cell = new aurore::BooleanCell();
    cell->init(title, on, std::move(changed));
    cadran::List::row(cell, icon);
    rows->addView(cell);
    return cell;
}
} // namespace

DevelopersTab::DevelopersTab()
{
    StylesModel& data = stylesModel();
    aurore::Application::registerModel("styles", &data.model);
    aurore::Application::setFocusAnnouncer([](const std::string& description)
        { stylesModel().announced = aurore::getStr("demo/styles/announced", description); });

    aurore::Box* page = show::page(this);

    // The frame, measured while the tab shows
    aurore::Box* frame = show::section(page, "activity", "demo/developers/frame"_i18n, "demo/developers/frame_lead"_i18n);
    aurore::Box* row   = show::row();
    this->fps          = figure(row, "demo/developers/fps"_i18n);
    this->cpu          = figure(row, "demo/developers/cpu"_i18n);
    this->calls        = figure(row, "demo/developers/calls"_i18n);
    this->laidOut      = figure(row, "demo/developers/laid_out"_i18n);
    frame->addView(row);

    // The inspector, and switches of the core
    aurore::Box* tools = show::section(page, "tools", "demo/developers/tools"_i18n, "demo/developers/tools_lead"_i18n);
    aurore::Box* open  = show::row(true, 20);
    open->setAlignItems(aurore::AlignItems::CENTER);
    open->setMarginBottom(show::gap());
    open->addView(show::iconButton("eye", "demo/developers/inspector"_i18n, []
        {
        aurore::Application::setInspectorEnabled(true);
        aurore::Inspector::setMode(aurore::Inspector::Mode::LIVE); }));
    aurore::Label* keys = show::note("demo/developers/inspector_note"_i18n);
    keys->setMarginTop(0);
    keys->setGrow(1);
    keys->setWidth(0);
    show::share(keys, 1); // under the button on a compact screen
    open->addView(keys);
    tools->addView(open);
    aurore::Box* rows = cadran::List::rows_in(tools);
    toggle(rows, "demo/settings/debug"_i18n, "lines", aurore::Application::isDebuggingViewEnabled(), [](bool on)
        { aurore::Application::enableDebuggingView(on); });
    toggle(rows, "demo/developers/on_demand"_i18n, "pause", aurore::Application::isRedrawOnDemand(), [](bool on)
        { aurore::Application::setRedrawOnDemand(on); });
    auto* layout = new aurore::DetailCell();
    layout->setText("demo/developers/layout"_i18n);
    layout->setDetailText(show::compact() ? "›" : "demo/developers/layout_detail"_i18n + "  ›");
    layout->registerClickAction([](aurore::View*)
        {
        demo::pushScreen(aurore::View::createFromXMLResource("tabs/layout.xml"), "demo/developers/layout"_i18n);
        return true; });
    cadran::List::row(layout, "lines");
    rows->addView(layout);
    // A console's screen, the console drawn around it (demo/src/simulation.cpp)
    auto* simulate = new aurore::DetailCell();
    simulate->setText("demo/developers/simulate"_i18n);
    simulate->setDetailText(demo::simulation::displayName(demo::simulation::current()) + "  ›");
    simulate->registerClickAction([](aurore::View*)
        {
        std::vector<ui::Choice> choices;
        for (const auto& device : demo::simulation::devices())
        {
            std::string id = device.id;
            bool shown     = &device == &demo::simulation::current();
            choices.emplace_back(demo::simulation::displayName(device), [id]()
                { demo::simulation::use(id); }, nullptr, "", "", shown ? ui::Choice::Tone::ON : ui::Choice::Tone::PLAIN);
        }
        ui::pick_actions("demo/developers/simulate"_i18n, choices);
        return true; });
    cadran::List::row(simulate, "gamepad");
    rows->addView(simulate);

    // Data binding and what a screen reader would say
    aurore::Box* binding = show::section(page, "refresh", "demo/styles/binding"_i18n, "demo/developers/binding_lead"_i18n);
    aurore::Box* card    = show::card(false, false);
    aurore::View* model  = aurore::View::createFromXMLResource("views/binding.xml");
    card->addView(model);
    if (auto* add = dynamic_cast<aurore::Button*>(model->getView("binding/add")))
    {
        cadran::style_button(add, cadran::Emphasis::NONE);
        kit::fit_button(add);
        add->registerClickAction([](aurore::View*)
            {
            StylesModel& data = stylesModel();
            data.counter      = data.counter.get() + 1;
            data.updateSwatch();
            data.even = data.counter.get() % 2 == 0;
            return true; });
    }
    aurore::Label* announced = kit::label("", kit::TEXT_NOTE, "cadran/text2");
    announced->bindText(data.announced);
    announced->setMarginTop(18);
    card->addView(announced);
    binding->addView(card);

    // How this page is made
    aurore::Box* code = show::section(page, "code", "demo/developers/code"_i18n, "demo/developers/code_lead"_i18n);
    aurore::Box* pair = show::row();
    pair->setAlignItems(aurore::AlignItems::STRETCH);
    aurore::Box* cpp = show::code("// A section of this page, in C++\n"
                                  "auto* page = show::page(this);\n"
                                  "auto* box  = show::section(page, \"code\", title, lead);\n"
                                  "auto* card = cadran::kit::card(true, true);\n"
                                  "card->addView(cadran::kit::label(text, 17));\n"
                                  "box->addView(card);");
    cpp->setGrow(1);
    cpp->setWidth(0);
    pair->addView(show::share(cpp, 1));
    aurore::Box* xml = show::code("<!-- The same in XML -->\n"
                                  "<aurore:Box axis=\"column\" gap=\"16\">\n"
                                  "  <aurore:Label text=\"@i18n/my/title\" />\n"
                                  "  <aurore:Button style=\"primary\"\n"
                                  "      text=\"@i18n/my/play\" />\n"
                                  "</aurore:Box>");
    xml->setGrow(1);
    xml->setWidth(0);
    pair->addView(show::share(xml, 1));
    code->addView(pair);
    aurore::Box* test = show::code("// A test: no window, simulated time, screenshots compared\n"
                                   "AURORE_TEST(opens) {\n"
                                   "    aurore::test::Harness h;\n"
                                   "    h.push(new MainActivity());\n"
                                   "    h.tap(aurore::BUTTON_A);\n"
                                   "    AURORE_CHECK(h.matchesReference(\"opens\"));\n"
                                   "}");
    test->setMarginTop(show::gap());
    code->addView(test);
    aurore::Label* reload = show::note("demo/developers/hot_reload"_i18n);
    reload->setMarginTop(show::gap());
    code->addView(reload);
}

DevelopersTab::~DevelopersTab()
{
    this->stopMeasuring();
}

void DevelopersTab::willAppear(bool resetState)
{
    ScrollView::willAppear(resetState);
    if (this->measuring)
        return;
    this->measuring  = true;
    this->wasEnabled = aurore::Profiler::isEnabled();
    aurore::Profiler::setEnabled(true);
    this->subscription = aurore::Profiler::getFrameEvent()->subscribe([this]
        { this->frameMeasured(); });
}

void DevelopersTab::willDisappear(bool resetState)
{
    ScrollView::willDisappear(resetState);
    this->stopMeasuring();
}

void DevelopersTab::stopMeasuring()
{
    if (!this->measuring)
        return;
    this->measuring = false;
    aurore::Profiler::getFrameEvent()->unsubscribe(this->subscription);
    aurore::Profiler::setEnabled(this->wasEnabled);
}

void DevelopersTab::frameMeasured()
{
    const aurore::FrameProfile& last = aurore::Profiler::getLastFrame();
    if (!last.drawn)
        return;
    // Averaged over half a second, then shown
    this->frames++;
    this->interval += last.interval;
    this->work += last.total - last.swap - last.events;
    this->drawCalls += last.drawCalls;
    this->views += last.viewsLaidOut;
    if (this->interval < 500000)
        return;
    float perFrame = (float)this->interval / this->frames;
    this->fps->setText(std::to_string((int)std::lround(1000000.0f / std::max(perFrame, 1.0f))));
    this->cpu->setText(std::format("{:.1f} ms", this->work / 1000.0f / this->frames));
    this->calls->setText(std::to_string(this->drawCalls / this->frames));
    this->laidOut->setText(std::to_string(this->views));
    this->frames   = 0;
    this->interval = this->work = 0;
    this->drawCalls = this->views = 0;
}

aurore::View* DevelopersTab::create()
{
    return new DevelopersTab();
}
