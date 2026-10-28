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

#pragma once

#include <aurore.hpp>

class RecyclerHeader
    : public aurore::RecyclerHeader
{
};

class RecyclerCell
    : public aurore::RecyclerCell
{
  public:
    RecyclerCell();

    AURORE_BIND(aurore::Rectangle, accent, "aurore/sidebar/item_accent");
    AURORE_BIND(aurore::Label, label, "title");
    AURORE_BIND(aurore::Image, image, "image");

    static RecyclerCell* create();
};

class DataSource
    : public aurore::RecyclerDataSource
{
  public:
    int numberOfSections(aurore::RecyclerFrame* recycler) override;
    int numberOfRows(aurore::RecyclerFrame* recycler, int section) override;
    aurore::RecyclerCell* cellForRow(aurore::RecyclerFrame* recycler, aurore::IndexPath index) override;
    void didSelectRowAt(aurore::RecyclerFrame* recycler, aurore::IndexPath indexPath) override;
    std::string titleForHeader(aurore::RecyclerFrame* recycler, int section) override;
};

class RecyclingListTab : public aurore::Box
{
  public:
    RecyclingListTab();

    static aurore::View* create();

  private:
    AURORE_BIND(aurore::RecyclerFrame, recycler, "recycler");
};
