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

int DataSource::numberOfSections(brls::RecyclerFrame* recycler)
{
    return 2;
}

int DataSource::numberOfRows(brls::RecyclerFrame* recycler, int section)
{
    return gems.size();
}

std::string DataSource::titleForHeader(brls::RecyclerFrame* recycler, int section)
{
    if (section == 0)
        return "";
    return "Section #" + std::to_string(section + 1);
}

brls::RecyclerCell* DataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    RecyclerCell* item = (RecyclerCell*)recycler->dequeueReusableCell("Cell");
    item->label->setText(gems[indexPath.row].name);
    item->image->setImageFromRes("img/gallery/thumbnails/" + gems[indexPath.row].id + ".png");
    return item;
}

void DataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    //    brls::Logger::info("Item Index(" + std::to_string(index.section) + ":" + std::to_string(index.row) + ") selected.");
    recycler->present(new GemView(gems[indexPath.row]));
}

// RECYCLER VIEW

RecyclingListTab::RecyclingListTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/recycling_list.xml");

    gems.clear();
    gems.push_back(Gem("amber", "Amber"));
    gems.push_back(Gem("coral", "Coral"));
    gems.push_back(Gem("jade", "Jade"));
    gems.push_back(Gem("lapis", "Lapis"));
    gems.push_back(Gem("onyx", "Onyx"));
    gems.push_back(Gem("opal", "Opal"));
    gems.push_back(Gem("pearl", "Pearl"));
    gems.push_back(Gem("ruby", "Ruby"));
    gems.push_back(Gem("topaz", "Topaz"));

    recycler->estimatedRowHeight = 70;
    recycler->registerCell("Header", []()
        { return RecyclerHeader::create(); });
    recycler->registerCell("Cell", []()
        { return RecyclerCell::create(); });
    recycler->setDataSource(new DataSource());
}

brls::View* RecyclingListTab::create()
{
    // Called by the XML engine to create a new RecyclingListTab
    return new RecyclingListTab();
}
