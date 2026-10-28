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

using namespace aurore::literals; // for _i18n

namespace
{
const std::array<const char*, 9> ids = { "amber", "coral", "jade", "lapis", "onyx", "opal", "pearl", "ruby", "topaz" };

// A gem's name in the language shown
std::string gemName(size_t index)
{
    return aurore::getStr(std::string("demo/gems/") + ids[index % ids.size()]);
}

constexpr size_t PAGE      = 24;
constexpr size_t MAX_ITEMS = 240;

const std::string pictureCellXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="column"
        alignItems="center"
        padding="10"
        cornerRadius="12"
        highlightCornerRadius="12"
        backgroundColor="@theme/aurore/highlight/background">

        <aurore:Image
            id="picture/image"
            width="auto"
            height="auto"
            grow="1.0"
            scalingType="fit" />

        <aurore:Label
            id="picture/label"
            width="auto"
            height="auto"
            marginTop="6"
            fontSize="16"
            singleLine="true"
            horizontalAlign="center" />

    </aurore:Box>
)xml";

// The row: always the same 40 thumbnails
class RowSource : public aurore::RecyclingGridDataSource
{
  public:
    size_t getItemCount() override { return 40; }

    aurore::RecyclingGridItem* cellForRow(aurore::RecyclingView* recycler, size_t index) override
    {
        auto* cell = (PictureCell*)recycler->dequeueReusableCell("Picture");
        cell->image->setImageFromRes(std::string("img/gallery/thumbnails/") + ids[index % ids.size()] + ".png");
        cell->label->setText("#" + std::to_string(index + 1));
        return cell;
    }

    void onItemSelected(aurore::View* recycler, size_t index) override
    {
        aurore::Application::notify(gemName(index) + " (#" + std::to_string(index + 1) + ")");
    }
};
}

PictureCell::PictureCell()
{
    this->inflateFromXMLString(pictureCellXML);

    // X or a long press: the item's menu, next to it
    aurore::ContextMenu::attach(this, [this](aurore::ContextMenu* menu)
        {
        std::string name = gemName(this->getIndex());
        auto chosen      = [name](const std::string& what) {
            return [name, what]() { aurore::Application::notify(name + ": " + what); };
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

aurore::RecyclingGridItem* PictureSource::cellForRow(aurore::RecyclingView* recycler, size_t index)
{
    auto* cell = (PictureCell*)recycler->dequeueReusableCell("Picture");
    cell->image->setImageFromRes(std::string("img/gallery/") + ids[index % ids.size()] + ".png");
    cell->label->setText(std::to_string(index + 1) + ". " + gemName(index));
    return cell;
}

void PictureSource::onItemSelected(aurore::View* recycler, size_t index)
{
    aurore::Application::notify(gemName(index) + " (#" + std::to_string(index + 1) + ")");
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
    aurore::delay(400, [this, alive]()
        { aurore::sync([this, alive]()
              {
            if (alive.expired())
                return;
            this->gridSource->count = std::min(MAX_ITEMS, this->gridSource->count + PAGE);
            this->grid->notifyDataChanged();
            this->updateHeader(); }); });
}

void GridTab::updateHeader()
{
    this->gridHeader->setSubtitle(aurore::getStr("demo/grid/grid_subtitle", this->gridSource->count, MAX_ITEMS));
}

aurore::View* GridTab::create()
{
    return new GridTab();
}
