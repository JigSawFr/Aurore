/*
    Copyright 2019-2021 natinusala
    Copyright 2019 WerWolv
    Copyright 2019 p-sam

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

#include <tinyxml2.h>
#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/util.hpp>
#include <borealis/views/tab_frame.hpp>

using namespace brls::literals;

const std::string tabFrameContentXML = R"xml(
    <brls:Box
        width="auto"
        height="auto"
        axis="row">

        <brls:Sidebar
            id="brls/tab_frame/sidebar"
            width="@style/brls/tab_frame/sidebar_width"
            height="auto" />

        <!-- Content will be injected here with grow="1.0" -->

    </brls:Box>
)xml";

namespace brls
{

TabFrame::TabFrame()
{
    this->inflateFromXMLString(tabFrameContentXML);
}

void TabFrame::addTab(std::string label, TabViewCreator creator)
{
    int index = this->tabCount++;
    this->sidebar->addItem(label, [this, creator, index](brls::View* view) {
        // Only trigger when the sidebar item gains focus
        if (!view->isFocused())
            return;

        // A kept tab shown again
        auto kept = this->keepTabs ? this->keptTabs.find(index) : this->keptTabs.end();
        if (kept != this->keptTabs.end() && kept->second == this->activeTab)
            return;

        // Remove the existing tab if it exists
        if (this->activeTab)
        {
            // will call willDisappear, and delete it unless it is kept
            bool keepIt = false;
            for (auto& tab : this->keptTabs)
                keepIt |= tab.second == this->activeTab;
            this->removeView(this->activeTab, !keepIt);
            this->activeTab = nullptr;
        }

        if (kept != this->keptTabs.end())
        {
            this->addView(kept->second); // addView calls willAppear
            this->activeTab = kept->second;
            return;
        }

        // Add the new tab
        View* newContent = creator();

        if (!newContent)
            return;

        newContent->setGrow(1.0f);
        this->addView(newContent); // addView calls willAppear

        this->activeTab = newContent;
        if (this->keepTabs)
            this->keptTabs[index] = newContent;

        newContent->registerAction(
            "hints/back"_i18n, BUTTON_B, [this](View* view) {
                if (Application::getInputType() == InputType::TOUCH)
                    this->dismiss();
                else
                    Application::giveFocus(this->sidebar);
                return true;
            },
            false, false, SOUND_BACK);
    });
}

void TabFrame::focusTab(int position)
{
    Application::giveFocus(this->sidebar->getItem(position));
}

void TabFrame::clearTabs()
{
    this->sidebar->clearItems();
    this->tabCount = 0;
    this->freeKeptTabs();
}

void TabFrame::setKeepTabs(bool keep)
{
    this->keepTabs = keep;
    if (!keep)
        this->freeKeptTabs();
}

void TabFrame::freeKeptTabs()
{
    // The one shown stays (freed when replaced, as without keeping); the
    // others are not in the tree and are deleted here
    for (auto& tab : this->keptTabs)
        if (tab.second != this->activeTab)
            delete tab.second;
    this->keptTabs.clear();
}

TabFrame::~TabFrame()
{
    this->freeKeptTabs();
}

void TabFrame::addSeparator()
{
    this->sidebar->addSeparator();
}

void TabFrame::handleXMLElement(tinyxml2::XMLElement* element)
{
    std::string name = element->Name();

    if (name == "brls:Tab")
    {
        const tinyxml2::XMLAttribute* labelAttribute = element->FindAttribute("label");

        if (!labelAttribute)
            fatal("\"label\" attribute missing from \"" + name + "\" tab");

        std::string label = View::getStringXMLAttributeValue(labelAttribute->Value());

        tinyxml2::XMLElement* viewElement = element->FirstChildElement();

        if (viewElement)
        {
            this->addTab(label, [viewElement] {
                return View::createFromXMLElement(viewElement);
            });

            if (viewElement->NextSiblingElement())
                fatal("\"brls:Tab\" can only contain one child element");
        }
        else
        {
            this->addTab(label, [] { return nullptr; });
        }
    }
    else if (name == "brls:Separator")
    {
        this->addSeparator();
    }
    else
    {
        fatal("Unknown child element \"" + name + "\" for \"brls:Tab\"");
    }
}

View* TabFrame::create()
{
    return new TabFrame();
}

} // namespace brls
