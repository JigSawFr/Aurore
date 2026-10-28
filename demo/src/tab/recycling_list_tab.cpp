/*
    Copyright 2020-2021 natinusala

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

#include "tab/recycling_list_tab.hpp"

using namespace aurore::literals; // for _i18n
#include "view/gem_view.hpp"

std::vector<Gem> gems;

RecyclerCell::RecyclerCell()
{
    this->inflateFromXMLRes("xml/cells/cell.xml");
}

RecyclerCell* RecyclerCell::create()
{
    return new RecyclerCell();
}

// DATA SOURCE

int DataSource::numberOfSections(aurore::RecyclerFrame* recycler)
{
    return 2;
}

int DataSource::numberOfRows(aurore::RecyclerFrame* recycler, int section)
{
    return gems.size();
}

std::string DataSource::titleForHeader(aurore::RecyclerFrame* recycler, int section)
{
    if (section == 0)
        return "";
    return aurore::getStr("demo/list/section", section + 1);
}

aurore::RecyclerCell* DataSource::cellForRow(aurore::RecyclerFrame* recycler, aurore::IndexPath indexPath)
{
    RecyclerCell* item = (RecyclerCell*)recycler->dequeueReusableCell("Cell");
    item->label->setText(gems[indexPath.row].name);
    item->image->setImageFromRes("img/gallery/thumbnails/" + gems[indexPath.row].id + ".png");
    return item;
}

void DataSource::didSelectRowAt(aurore::RecyclerFrame* recycler, aurore::IndexPath indexPath)
{
    //    aurore::Logger::info("Item Index(" + std::to_string(index.section) + ":" + std::to_string(index.row) + ") selected.");
    recycler->present(new GemView(gems[indexPath.row]));
}

// RECYCLER VIEW

RecyclingListTab::RecyclingListTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/recycling_list.xml");

    gems.clear();
    gems.push_back(Gem("amber", "demo/gems/amber"_i18n));
    gems.push_back(Gem("coral", "demo/gems/coral"_i18n));
    gems.push_back(Gem("jade", "demo/gems/jade"_i18n));
    gems.push_back(Gem("lapis", "demo/gems/lapis"_i18n));
    gems.push_back(Gem("onyx", "demo/gems/onyx"_i18n));
    gems.push_back(Gem("opal", "demo/gems/opal"_i18n));
    gems.push_back(Gem("pearl", "demo/gems/pearl"_i18n));
    gems.push_back(Gem("ruby", "demo/gems/ruby"_i18n));
    gems.push_back(Gem("topaz", "demo/gems/topaz"_i18n));

    recycler->estimatedRowHeight = 70;
    recycler->registerCell("Header", []()
        { return RecyclerHeader::create(); });
    recycler->registerCell("Cell", []()
        { return RecyclerCell::create(); });
    recycler->setDataSource(new DataSource());
}

aurore::View* RecyclingListTab::create()
{
    // Called by the XML engine to create a new RecyclingListTab
    return new RecyclingListTab();
}
