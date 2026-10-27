/*
    Copyright 2022 xfangfang
    Copyright 2023 dragonflylee and the Switchfin contributors
    Copyright 2026 JigSawFr

    Adapted from Switchfin's recycling_grid and h_recycling
    (https://github.com/dragonflylee/switchfin, Apache-2.0).

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

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <borealis/core/box.hpp>
#include <borealis/views/h_scrolling_frame.hpp>
#include <borealis/views/scrolling_frame.hpp>

namespace brls
{

class RecyclingView;

// A cell of a RecyclingGrid or an HRecyclerFrame. Only the cells on screen
// (and a line ahead on each side) exist: the others are queued, and given
// back by dequeueReusableCell() to show another item.
class RecyclingGridItem : public Box
{
  public:
    RecyclingGridItem();

    // The item this cell shows, in the data source.
    size_t getIndex() const { return this->index; }

    // Internal: set by the recycler when the cell is placed.
    void setIndex(size_t value) { this->index = value; }

    // The identifier the cell was registered with (registerCell()).
    std::string reuseIdentifier;

    // Called by dequeueReusableCell() before the data source fills the cell
    // again: reset what one item set and another might not.
    virtual void prepareForReuse() { }

    // Called when the cell leaves the screen and is queued: let go of what
    // is heavy (a picture's texture, a request in flight).
    virtual void cacheForReuse() { }

  private:
    size_t index = 0;
};

class RecyclingGridDataSource
{
  public:
    virtual ~RecyclingGridDataSource() = default;

    // How many items there are now.
    virtual size_t getItemCount() { return 0; }

    // A cell showing the item at `index`: dequeueReusableCell(), then fill it.
    virtual RecyclingGridItem* cellForRow(RecyclingView* recycler, size_t index) { return nullptr; }

    // The item at `index` was clicked (A, a tap, a click).
    virtual void onItemSelected(View* recycler, size_t index) { }

    // Forget the items (RecyclingView::clearData()).
    virtual void clearData() { }
};

// What a RecyclingGrid and an HRecyclerFrame share: the cells, their queue,
// the data source, where each item goes and the focus between them. Items
// are laid out in lines of `spanCount` along the scrolling axis: rows of
// columns in a RecyclingGrid, columns of rows in an HRecyclerFrame. Every
// item has the same size.
class RecyclingView
{
  public:
    virtual ~RecyclingView();

    // Registers how to build a cell for `identifier`.
    void registerCell(std::string identifier, std::function<RecyclingGridItem*()> allocation);

    // A queued cell for `identifier`, or a new one; prepareForReuse() is
    // called on it either way.
    RecyclingGridItem* dequeueReusableCell(std::string identifier);

    // The data source; with `deleteDataSource`, the recycler deletes it when
    // it is replaced or when the recycler goes.
    void setDataSource(RecyclingGridDataSource* source, bool deleteDataSource = true);
    RecyclingGridDataSource* getDataSource() const { return this->dataSource; }

    // Builds the cells again from the data source, scrolled to the default
    // cell (setDefaultCellFocus()); the focus, if it was in the recycler,
    // goes to that cell.
    void reloadData();

    // Items were added at the end: the content grows, the cells and the
    // scrolling stay, and the end-reached callback may fire again. Fewer
    // items than before reload everything.
    void notifyDataChanged();

    // Calls the data source's clearData(), then reloads.
    void clearData();

    // Called once when the last item comes into view (with preFetchLine
    // lines ahead): load the next page there, then notifyDataChanged().
    // It fires again after notifyDataChanged(), reloadData(),
    // setDataSource() or forceRequestNextPage().
    void onEndReached(std::function<void()> callback) { this->endReachedCallback = callback; }
    void onNextPage(std::function<void()> callback) { this->onEndReached(callback); }
    void forceRequestNextPage() { this->endRequested = false; }

    // Scrolls to the item at `index` and makes it the one focused when the
    // focus enters the recycler.
    void selectRowAt(size_t index, bool animated);

    // The cell showing the item at `index`, if it is on screen. Only valid
    // until the next frame: cells are reused.
    RecyclingGridItem* getGridItemByIndex(size_t index);

    void setDefaultCellFocus(size_t index) { this->defaultCellFocus = index; }
    size_t getDefaultCellFocus() const { return this->defaultCellFocus; }

    size_t getItemCount();

    // Lines of items across the scrolling axis.
    int getSpanCount() const { return this->span; }

    // The frame: the RecyclingGrid or HRecyclerFrame itself.
    virtual Box* getFrameBox() = 0;

    // Internal, for the content box.
    View* getNextCellFocus(FocusDirection direction, View* currentView);

  protected:
    RecyclingView(Axis scrollAxis);

    Box* contentBox = nullptr;

    virtual float getScrollOffset()                           = 0;
    virtual void setScrollOffset(float offset, bool animated) = 0;

    // Places and recycles the cells for the current scrolling.
    void recyclingLoop();

    // The frame's size changed: the items are laid out again before the
    // next frame (not from onLayout(), which yoga calls in the middle of its
    // layout: building cells there moved the frame itself).
    void layoutChanged();

    // Applies a pending layoutChanged().
    void flushLayout();

    // onThemeChanged() on the cells kept for reuse (the frame's own
    // onThemeChanged() reaches the cells shown).
    void queuedCellsThemeChanged();

    // The cell the focus enters the recycler on, if any.
    View* defaultCell();

    // Padding inside the scrolled content; reloadData() applies it.
    float paddingTop = 0, paddingRight = 0, paddingBottom = 0, paddingLeft = 0;

    // Lines across the scrolling axis: a fixed count, or as many as fit
    // lines `minimumLineSize` wide (0: the fixed count).
    int spanCount         = 1;
    float minimumLineSize = 0;

    // The size of an item along the scrolling axis, the space between two
    // items, and how many lines are kept ready beyond each end of the view.
    float itemSize   = 240;
    float itemSpace  = 20;
    int preFetchLine = 1;

  private:
    Axis axis;
    bool layouted      = false;
    bool layoutPending = false;
    int span           = 1;
    float crossSize    = 0; // an item's size across the scrolling axis

    RecyclingGridDataSource* dataSource = nullptr;
    bool deleteDataSource               = false;
    size_t defaultCellFocus             = 0;
    size_t knownCount                   = 0;

    std::function<void()> endReachedCallback;
    bool endRequested = false;

    std::map<std::string, std::vector<RecyclingGridItem*>> queueMap;
    std::map<std::string, std::function<RecyclingGridItem*()>> allocationMap;

    void computeGeometry();
    float pitch() const { return this->itemSize + this->itemSpace; }
    float lineStart(size_t line) const;
    float scrollTarget(size_t index);
    void placeCell(RecyclingGridItem* cell, size_t index);
    RecyclingGridItem* addCell(size_t index);
    void queueCell(RecyclingGridItem* cell);
    bool holdsFocus(View* view);
};

// Its cells' parent: hands the navigation between cells to the recycler.
class RecyclingGridContentBox : public Box
{
  public:
    RecyclingGridContentBox(RecyclingView* recycler);
    View* getNextFocus(FocusDirection direction, View* currentView) override;
    RecyclingView* getRecycler() const { return this->recycler; }

  private:
    RecyclingView* recycler;
};

// A grid that scrolls vertically and only keeps the cells on screen.
// `spanCount` columns, or as many `columnWidth` wide as fit (then they share
// what is left); every row `itemHeight` tall, `itemSpace` between items.
// The focus moves in both directions; it leaves the grid past its edges.
//
// XML: spanCount, columnWidth, itemHeight, itemSpace, preFetchLine, and the
// padding attributes (padding inside the scrolled content).
class RecyclingGrid : public ScrollingFrame, public RecyclingView
{
  public:
    RecyclingGrid();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onLayout() override;
    void onThemeChanged() override;
    View* getDefaultFocus() override;

    void setSpanCount(int count);
    void setColumnWidth(float width);
    void setItemHeight(float height);
    void setItemSpace(float space);
    void setPreFetchLine(int lines);

    void setPadding(float padding) override;
    void setPadding(float top, float right, float bottom, float left) override;
    void setPaddingTop(float top) override;
    void setPaddingRight(float right) override;
    void setPaddingBottom(float bottom) override;
    void setPaddingLeft(float left) override;

    static View* create();

    Box* getFrameBox() override { return this; }

  protected:
    float getScrollOffset() override { return this->getContentOffsetY(); }
    void setScrollOffset(float offset, bool animated) override { this->setContentOffsetY(offset, animated); }

  private:
    float oldWidth = -1;
};

// A row that scrolls horizontally and only keeps the cells on screen:
// items `itemWidth` wide, `itemSpace` apart, as tall as the row (or
// `spanCount` rows sharing its height).
//
// XML: itemWidth, itemSpace, spanCount, preFetchLine, and the padding
// attributes (padding inside the scrolled content).
class HRecyclerFrame : public HScrollingFrame, public RecyclingView
{
  public:
    HRecyclerFrame();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onLayout() override;
    void onThemeChanged() override;
    View* getDefaultFocus() override;

    void setSpanCount(int count);
    void setItemWidth(float width);
    void setItemSpace(float space);
    void setPreFetchLine(int lines);

    void setPadding(float padding) override;
    void setPadding(float top, float right, float bottom, float left) override;
    void setPaddingTop(float top) override;
    void setPaddingRight(float right) override;
    void setPaddingBottom(float bottom) override;
    void setPaddingLeft(float left) override;

    static View* create();

    Box* getFrameBox() override { return this; }

  protected:
    float getScrollOffset() override { return this->getContentOffsetX(); }
    void setScrollOffset(float offset, bool animated) override { this->setContentOffsetX(offset, animated); }

  private:
    float oldHeight = -1;
};

} // namespace brls
