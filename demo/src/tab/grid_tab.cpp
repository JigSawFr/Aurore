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

#include "tab/grid_tab.hpp"

#include <array>

using namespace brls::literals; // for _i18n

namespace
{
const std::array<const char*, 9> names = { "Bulbasaur", "Charmander", "Squirtle", "Metapod", "Kakuna",
    "Pidgeotto", "Spearow", "Arbok", "Sandshrew" };
const std::array<const char*, 9> ids   = { "001", "004", "007", "011", "014", "017", "021", "024", "027" };

constexpr size_t PAGE      = 24;
constexpr size_t MAX_ITEMS = 240;

const std::string pictureCellXML = R"xml(
    <brls:Box
        width="auto"
        height="auto"
        axis="column"
        alignItems="center"
        padding="10"
        cornerRadius="12"
        highlightCornerRadius="12"
        backgroundColor="@theme/brls/highlight/background">

        <brls:Image
            id="picture/image"
            width="auto"
            height="auto"
            grow="1.0"
            scalingType="fit" />

        <brls:Label
            id="picture/label"
            width="auto"
            height="auto"
            marginTop="6"
            fontSize="16"
            singleLine="true"
            horizontalAlign="center" />

    </brls:Box>
)xml";

// The row: always the same 40 thumbnails
class RowSource : public brls::RecyclingGridDataSource
{
  public:
    size_t getItemCount() override { return 40; }

    brls::RecyclingGridItem* cellForRow(brls::RecyclingView* recycler, size_t index) override
    {
        auto* cell = (PictureCell*)recycler->dequeueReusableCell("Picture");
        cell->image->setImageFromRes(std::string("img/pokemon/thumbnails/") + ids[index % ids.size()] + ".png");
        cell->label->setText("#" + std::to_string(index + 1));
        return cell;
    }

    void onItemSelected(brls::View* recycler, size_t index) override
    {
        brls::Application::notify(std::string(names[index % names.size()]) + " (#" + std::to_string(index + 1) + ")");
    }
};
}

PictureCell::PictureCell()
{
    this->inflateFromXMLString(pictureCellXML);

    // X or a long press: the item's menu, next to it
    brls::ContextMenu::attach(this, [this](brls::ContextMenu* menu)
        {
        std::string name = names[this->getIndex() % names.size()];
        auto chosen      = [name](const std::string& what) {
            return [name, what]() { brls::Application::notify(name + ": " + what); };
        };
        menu->setTitle(name);
        menu->addItem("demo/components/menu_open"_i18n, chosen("demo/components/menu_open"_i18n), "\uE89E");
        menu->addItem("demo/components/menu_favourite"_i18n, chosen("demo/components/menu_favourite"_i18n), "\uE87D");
        menu->addSeparator();
        menu->addItem("demo/components/menu_delete"_i18n, chosen("demo/components/menu_delete"_i18n), "\uE872"); });
}

void PictureCell::prepareForReuse()
{
    this->label->setText("");
}

PictureCell* PictureCell::create()
{
    return new PictureCell();
}

brls::RecyclingGridItem* PictureSource::cellForRow(brls::RecyclingView* recycler, size_t index)
{
    auto* cell = (PictureCell*)recycler->dequeueReusableCell("Picture");
    cell->image->setImageFromRes(std::string("img/pokemon/") + ids[index % ids.size()] + ".png");
    cell->label->setText(std::to_string(index + 1) + ". " + names[index % names.size()]);
    return cell;
}

void PictureSource::onItemSelected(brls::View* recycler, size_t index)
{
    brls::Application::notify(std::string(names[index % names.size()]) + " (#" + std::to_string(index + 1) + ")");
}

GridTab::GridTab()
{
    this->inflateFromXMLRes("xml/tabs/grid.xml");

    this->row->registerCell("Picture", []()
        { return PictureCell::create(); });
    this->row->setDataSource(new RowSource());

    this->gridSource = new PictureSource(PAGE * 2);
    this->grid->registerCell("Picture", []()
        { return PictureCell::create(); });
    this->grid->onEndReached([this]()
        { this->loadMore(); });
    this->grid->setDataSource(this->gridSource);
    this->updateHeader();
}

GridTab::~GridTab()
{
    *this->alive = false;
}

// A page more, a moment later, as if it came from a server
void GridTab::loadMore()
{
    if (this->gridSource->count >= MAX_ITEMS)
        return;

    std::weak_ptr<bool> alive = this->alive;
    brls::delay(400, [this, alive]()
        { brls::sync([this, alive]()
              {
            if (alive.expired())
                return;
            this->gridSource->count = std::min(MAX_ITEMS, this->gridSource->count + PAGE);
            this->grid->notifyDataChanged();
            this->updateHeader(); }); });
}

void GridTab::updateHeader()
{
    this->gridHeader->setSubtitle(brls::getStr("demo/grid/grid_subtitle", this->gridSource->count, MAX_ITEMS));
}

brls::View* GridTab::create()
{
    return new GridTab();
}
