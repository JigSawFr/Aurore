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

#pragma once

#include <memory>
#include <aurore.hpp>

// A picture and its caption, in the grid and in the row.
class PictureCell : public brls::RecyclingGridItem
{
  public:
    PictureCell();

    void prepareForReuse() override;

    static PictureCell* create();

    BRLS_BIND(brls::Image, image, "picture/image");
    BRLS_BIND(brls::Label, label, "picture/label");
};

// Items the grid shows: a page more each time its end comes into view.
class PictureSource : public brls::RecyclingGridDataSource
{
  public:
    explicit PictureSource(size_t count)
        : count(count)
    {
    }

    size_t getItemCount() override { return this->count; }
    brls::RecyclingGridItem* cellForRow(brls::RecyclingView* recycler, size_t index) override;
    void onItemSelected(brls::View* recycler, size_t index) override;
    void clearData() override { this->count = 0; }

    size_t count;
};

class GridTab : public brls::Box
{
  public:
    GridTab();
    ~GridTab() override;

    static brls::View* create();

  private:
    BRLS_BIND(brls::HRecyclerFrame, row, "row");
    BRLS_BIND(brls::RecyclingGrid, grid, "grid");
    BRLS_BIND(brls::Header, gridHeader, "grid_header");

    PictureSource* gridSource;
    std::shared_ptr<bool> alive = std::make_shared<bool>(true);

    void loadMore();
    void updateHeader();
};
