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

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <aurore/core/activity.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/touch/tap_gesture.hpp>
#include <aurore/views/recycling_grid.hpp>

namespace brls
{

// RecyclingGridItem

RecyclingGridItem::RecyclingGridItem()
{
    this->setFocusable(true);

    this->registerClickAction([this](View* view)
        {
        auto* content = dynamic_cast<RecyclingGridContentBox*>(this->getParent());
        if (!content)
            return false;
        RecyclingView* recycler = content->getRecycler();
        if (recycler->getDataSource())
            recycler->getDataSource()->onItemSelected(recycler->getFrameBox(), this->index);
        return true; });

    this->addGestureRecognizer(new TapGestureRecognizer(this));
}

// RecyclingGridContentBox

RecyclingGridContentBox::RecyclingGridContentBox(RecyclingView* recycler)
    : Box(Axis::ROW)
    , recycler(recycler)
{
}

View* RecyclingGridContentBox::getNextFocus(FocusDirection direction, View* currentView)
{
    return this->recycler->getNextCellFocus(direction, currentView);
}

// RecyclingView

RecyclingView::RecyclingView(Axis scrollAxis)
    : axis(scrollAxis)
{
    this->contentBox = new RecyclingGridContentBox(this);
}

RecyclingView::~RecyclingView()
{
    if (this->dataSource && this->deleteDataSource)
        delete this->dataSource;

    // The cells on screen are the content box's children, freed with it
    for (auto& [identifier, queue] : this->queueMap)
    {
        for (RecyclingGridItem* cell : queue)
        {
            cell->setParent(nullptr);
            if (cell->isPtrLocked())
                cell->freeView();
            else
                delete cell;
        }
    }
}

void RecyclingView::registerCell(std::string identifier, std::function<RecyclingGridItem*()> allocation)
{
    this->queueMap[identifier];
    this->allocationMap[identifier] = allocation;
}

RecyclingGridItem* RecyclingView::dequeueReusableCell(std::string identifier)
{
    RecyclingGridItem* cell = nullptr;

    auto queue = this->queueMap.find(identifier);
    if (queue != this->queueMap.end() && !queue->second.empty())
    {
        cell = queue->second.back();
        queue->second.pop_back();
    }
    else
    {
        auto allocation = this->allocationMap.find(identifier);
        if (allocation == this->allocationMap.end())
        {
            Logger::error("RecyclingView: no cell registered as \"{}\"", identifier);
            return nullptr;
        }
        cell                  = allocation->second();
        cell->reuseIdentifier = identifier;
        cell->detach();
    }

    cell->prepareForReuse();
    return cell;
}

void RecyclingView::setDataSource(RecyclingGridDataSource* source, bool deleteDataSource)
{
    if (this->dataSource && this->deleteDataSource && this->dataSource != source)
        delete this->dataSource;

    this->dataSource       = source;
    this->deleteDataSource = deleteDataSource;
    this->reloadData();
}

size_t RecyclingView::getItemCount()
{
    return this->dataSource ? this->dataSource->getItemCount() : 0;
}

void RecyclingView::computeGeometry()
{
    Box* frame    = this->getFrameBox();
    bool vertical = this->axis == Axis::COLUMN;
    float width   = frame->getWidth();
    float height  = frame->getHeight();
    float across  = vertical ? width - this->paddingLeft - this->paddingRight
                             : height - this->paddingTop - this->paddingBottom;

    if (this->minimumLineSize > 0)
        this->span = std::max(1, (int)std::floor((across + this->itemSpace) / (this->minimumLineSize + this->itemSpace)));
    else
        this->span = std::max(1, this->spanCount);

    this->crossSize = std::max(0.0f, (across - this->itemSpace * (this->span - 1)) / this->span);

    size_t lines = (this->getItemCount() + this->span - 1) / this->span;
    float length = lines > 0 ? lines * this->pitch() - this->itemSpace : 0;

    if (vertical)
    {
        this->contentBox->setWidth(width);
        this->contentBox->setHeight(this->paddingTop + length + this->paddingBottom);
    }
    else
    {
        this->contentBox->setHeight(height);
        this->contentBox->setWidth(this->paddingLeft + length + this->paddingRight);
    }
}

float RecyclingView::lineStart(size_t line) const
{
    float start = this->axis == Axis::COLUMN ? this->paddingTop : this->paddingLeft;
    return start + line * this->pitch();
}

void RecyclingView::placeCell(RecyclingGridItem* cell, size_t index)
{
    size_t line  = index / this->span;
    size_t place = index % this->span;
    float along  = this->lineStart(line);
    float across = place * (this->crossSize + this->itemSpace);

    if (this->axis == Axis::COLUMN)
    {
        cell->setDimensions(this->crossSize, this->itemSize);
        cell->setDetachedPosition(this->paddingLeft + across, along);
    }
    else
    {
        cell->setDimensions(this->itemSize, this->crossSize);
        cell->setDetachedPosition(along, this->paddingTop + across);
    }
}

RecyclingGridItem* RecyclingView::addCell(size_t index)
{
    if (!this->dataSource || index >= this->getItemCount())
        return nullptr;

    RecyclingGridItem* cell = this->dataSource->cellForRow(this, index);
    if (!cell)
        return nullptr;

    cell->setIndex(index);
    this->placeCell(cell, index);

    this->contentBox->getChildren().push_back(cell);

    // The same parent userdata as Box::addView(): the position in the parent
    size_t* userdata = (size_t*)malloc(sizeof(size_t));
    *userdata        = this->contentBox->getChildren().size() - 1;
    cell->setParent(this->contentBox, userdata);

    this->contentBox->invalidate();
    cell->View::willAppear();
    return cell;
}

// The cells kept for reuse: they come back in the theme drawn now
void RecyclingView::queuedCellsThemeChanged()
{
    for (auto& [identifier, queue] : this->queueMap)
        for (RecyclingGridItem* cell : queue)
            cell->onThemeChanged();
}

void RecyclingView::queueCell(RecyclingGridItem* cell)
{
    auto& children = this->contentBox->getChildren();
    auto it        = std::find(children.begin(), children.end(), cell);
    if (it == children.end())
        return;

    children.erase(it);
    for (size_t i = 0; i < children.size(); i++)
        if (auto* position = (size_t*)children[i]->getParentUserData())
            *position = i;

    if (this->contentBox->getLastFocusedView() == cell)
        this->contentBox->setLastFocusedView(nullptr);

    cell->willDisappear(true);
    cell->cacheForReuse();
    this->queueMap[cell->reuseIdentifier].push_back(cell);
    this->contentBox->invalidate();
}

bool RecyclingView::holdsFocus(View* view)
{
    for (View* v = Application::getCurrentFocus(); v; v = v->getParent())
        if (v == view)
            return true;
    return false;
}

RecyclingGridItem* RecyclingView::getGridItemByIndex(size_t index)
{
    for (View* child : this->contentBox->getChildren())
    {
        auto* cell = (RecyclingGridItem*)child;
        if (cell->getIndex() == index)
            return cell;
    }
    return nullptr;
}

void RecyclingView::recyclingLoop()
{
    this->flushLayout();
    if (!this->layouted || !this->dataSource)
        return;

    Box* frame     = this->getFrameBox();
    size_t count   = this->getItemCount();
    float viewport = this->axis == Axis::COLUMN ? frame->getHeight() : frame->getWidth();
    float offset   = this->getScrollOffset();
    float ahead    = this->preFetchLine * this->pitch();
    float origin   = this->lineStart(0);

    float start = offset - ahead - origin;
    float end   = offset + viewport + ahead - origin;

    size_t first = 0, last = 0; // items [first, last) are kept
    if (end >= 0 && count > 0)
    {
        size_t firstLine = start > 0 ? (size_t)(start / this->pitch()) : 0;
        size_t lastLine  = (size_t)(end / this->pitch());
        first            = std::min(count, firstLine * this->span);
        last             = std::min(count, (lastLine + 1) * this->span);
    }

    // The focused cell stays, wherever the touch scrolled it: the focus
    // must not follow a cell to another item
    std::vector<View*> children = this->contentBox->getChildren();
    for (View* child : children)
    {
        auto* cell = (RecyclingGridItem*)child;
        if ((cell->getIndex() < first || cell->getIndex() >= last) && !this->holdsFocus(cell))
            this->queueCell(cell);
    }

    if (last > first)
    {
        std::vector<bool> shown(last - first, false);
        for (View* child : this->contentBox->getChildren())
        {
            size_t index = ((RecyclingGridItem*)child)->getIndex();
            if (index >= first && index < last)
                shown[index - first] = true;
        }
        for (size_t index = first; index < last; index++)
            if (!shown[index - first])
                this->addCell(index);
    }

    if (count > 0 && last >= count && !this->endRequested && this->endReachedCallback)
    {
        this->endRequested = true;
        this->endReachedCallback();
    }
}

float RecyclingView::scrollTarget(size_t index)
{
    Box* frame     = this->getFrameBox();
    bool vertical  = this->axis == Axis::COLUMN;
    float viewport = vertical ? frame->getHeight() : frame->getWidth();
    float content  = vertical ? this->contentBox->getHeight() : this->contentBox->getWidth();

    float target = this->lineStart(index / this->span) + this->itemSize / 2 - viewport / 2;
    return std::max(0.0f, std::min(target, content - viewport));
}

void RecyclingView::selectRowAt(size_t index, bool animated)
{
    this->defaultCellFocus = index;
    if (!this->layouted || this->getItemCount() == 0)
        return;

    index = std::min(index, this->getItemCount() - 1);
    this->setScrollOffset(this->scrollTarget(index), animated);
    this->recyclingLoop();

    RecyclingGridItem* cell = this->getGridItemByIndex(index);
    if (!cell)
        cell = this->addCell(index);
    this->contentBox->setLastFocusedView(cell);
}

void RecyclingView::reloadData()
{
    if (!this->layouted)
        return;

    bool focusHere = this->holdsFocus(this->contentBox);

    // Queued, not freed: given back to show the new items
    std::vector<View*> children = this->contentBox->getChildren();
    for (View* child : children)
        this->queueCell((RecyclingGridItem*)child);
    this->contentBox->setLastFocusedView(nullptr);

    this->knownCount   = this->getItemCount();
    this->endRequested = false;
    this->computeGeometry();
    this->setScrollOffset(0, false);

    if (this->knownCount == 0)
    {
        // The cell that had the focus is queued now: the screen takes it
        Activity* activity = this->getFrameBox()->getParentActivity();
        if (focusHere && activity)
            Application::giveFocus(activity->getContentView());
        return;
    }

    this->selectRowAt(std::min(this->defaultCellFocus, this->knownCount - 1), false);

    if (focusHere)
    {
        if (View* cell = this->contentBox->getLastFocusedView())
            Application::giveFocus(cell);
    }
}

void RecyclingView::notifyDataChanged()
{
    if (!this->layouted)
        return;

    size_t count = this->getItemCount();
    if (count < this->knownCount)
    {
        this->reloadData();
        return;
    }

    // Only more items let the end-reached callback fire again: a last page
    // that brought nothing would otherwise ask again on every frame
    if (count > this->knownCount)
        this->endRequested = false;
    this->knownCount = count;

    this->computeGeometry();
    this->contentBox->invalidate();
    Application::requestRedraw();
}

void RecyclingView::clearData()
{
    if (!this->dataSource)
        return;
    this->dataSource->clearData();
    this->reloadData();
}

void RecyclingView::layoutChanged()
{
    this->layoutPending = true;
    Application::requestRedraw();
}

void RecyclingView::flushLayout()
{
    if (!this->layoutPending)
        return;
    this->layoutPending = false;
    this->layouted      = true;
    this->reloadData();
}

View* RecyclingView::defaultCell()
{
    this->flushLayout();
    auto& children = this->contentBox->getChildren();
    if (children.empty())
        return nullptr;

    View* last = this->contentBox->getLastFocusedView();
    if (last && std::find(children.begin(), children.end(), last) != children.end())
        return last;

    if (RecyclingGridItem* cell = this->getGridItemByIndex(this->defaultCellFocus))
        return cell;

    // The first one on screen
    float offset   = this->getScrollOffset();
    View* best     = nullptr;
    size_t bestIdx = SIZE_MAX;
    for (View* child : children)
    {
        auto* cell     = (RecyclingGridItem*)child;
        Point position = cell->getDetachedPosition();
        float along    = this->axis == Axis::COLUMN ? position.y : position.x;
        if (along + this->itemSize > offset && cell->getIndex() < bestIdx)
        {
            best    = cell;
            bestIdx = cell->getIndex();
        }
    }
    return best;
}

View* RecyclingView::getNextCellFocus(FocusDirection direction, View* currentView)
{
    this->flushLayout();
    Box* frame = this->getFrameBox();
    View* next = nullptr;

    auto* cell   = dynamic_cast<RecyclingGridItem*>(currentView);
    size_t count = this->getItemCount();
    if (cell && cell->getIndex() < count)
    {
        size_t index   = cell->getIndex();
        size_t span    = this->span;
        bool vertical  = this->axis == Axis::COLUMN;
        bool alongAxis = vertical ? (direction == FocusDirection::UP || direction == FocusDirection::DOWN)
                                  : (direction == FocusDirection::LEFT || direction == FocusDirection::RIGHT);
        bool forward   = direction == FocusDirection::DOWN || direction == FocusDirection::RIGHT;
        bool found     = false;
        size_t target  = 0;

        if (alongAxis)
        {
            // To the next line, or its last item when it is shorter
            if (!forward && index >= span)
            {
                target = index - span;
                found  = true;
            }
            else if (forward && (index / span + 1) * span < count)
            {
                target = std::min(index + span, count - 1);
                found  = true;
            }
        }
        else
        {
            // Within the line: its ends lead out of the recycler
            size_t place = index % span;
            if (!forward && place > 0)
            {
                target = index - 1;
                found  = true;
            }
            else if (forward && place + 1 < span && index + 1 < count)
            {
                target = index + 1;
                found  = true;
            }
        }

        if (found)
        {
            RecyclingGridItem* targetCell = this->getGridItemByIndex(target);
            if (!targetCell)
                targetCell = this->addCell(target);
            if (targetCell)
                next = targetCell->getDefaultFocus();
        }
    }

    next = frame->getParentNavigationDecision(frame, next, direction);
    if (!next && frame->hasParent())
        next = frame->getParent()->getNextFocus(direction, frame);
    return next;
}

// RecyclingGrid

RecyclingGrid::RecyclingGrid()
    : RecyclingView(Axis::COLUMN)
{
    this->setFocusable(false);
    this->setScrollingBehavior(ScrollingBehavior::CENTERED);
    this->setContentView(this->contentBox);

    this->registerFloatXMLAttribute("spanCount", [this](float value)
        { this->setSpanCount((int)value); });
    this->registerFloatXMLAttribute("columnWidth", [this](float value)
        { this->setColumnWidth(value); });
    this->registerFloatXMLAttribute("itemHeight", [this](float value)
        { this->setItemHeight(value); });
    this->registerFloatXMLAttribute("itemSpace", [this](float value)
        { this->setItemSpace(value); });
    this->registerFloatXMLAttribute("preFetchLine", [this](float value)
        { this->setPreFetchLine((int)value); });
}

void RecyclingGrid::setSpanCount(int count)
{
    this->spanCount       = std::max(1, count);
    this->minimumLineSize = 0;
    this->reloadData();
}

void RecyclingGrid::setColumnWidth(float width)
{
    this->minimumLineSize = std::max(0.0f, width);
    this->reloadData();
}

void RecyclingGrid::setItemHeight(float height)
{
    this->itemSize = height;
    this->reloadData();
}

void RecyclingGrid::setItemSpace(float space)
{
    this->itemSpace = space;
    this->reloadData();
}

void RecyclingGrid::setPreFetchLine(int lines)
{
    this->preFetchLine = std::max(0, lines);
}

void RecyclingGrid::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    this->recyclingLoop();
    ScrollingFrame::draw(vg, x, y, width, height, style, ctx);
}

void RecyclingGrid::onThemeChanged()
{
    ScrollingFrame::onThemeChanged();
    this->queuedCellsThemeChanged();
}

void RecyclingGrid::onLayout()
{
    ScrollingFrame::onLayout();

    float width = this->getWidth();
    if (std::isnan(width) || width <= 0 || (int)width == (int)this->oldWidth)
        return;
    this->oldWidth = width;
    this->layoutChanged();
}

View* RecyclingGrid::getDefaultFocus()
{
    View* cell = this->defaultCell();
    return cell ? cell->getDefaultFocus() : nullptr;
}

void RecyclingGrid::setPadding(float padding)
{
    this->setPadding(padding, padding, padding, padding);
}

void RecyclingGrid::setPadding(float top, float right, float bottom, float left)
{
    this->paddingTop    = top;
    this->paddingRight  = right;
    this->paddingBottom = bottom;
    this->paddingLeft   = left;
    this->reloadData();
}

void RecyclingGrid::setPaddingTop(float top)
{
    this->paddingTop = top;
    this->reloadData();
}

void RecyclingGrid::setPaddingRight(float right)
{
    this->paddingRight = right;
    this->reloadData();
}

void RecyclingGrid::setPaddingBottom(float bottom)
{
    this->paddingBottom = bottom;
    this->reloadData();
}

void RecyclingGrid::setPaddingLeft(float left)
{
    this->paddingLeft = left;
    this->reloadData();
}

View* RecyclingGrid::create()
{
    return new RecyclingGrid();
}

// HRecyclerFrame

HRecyclerFrame::HRecyclerFrame()
    : RecyclingView(Axis::ROW)
{
    this->setFocusable(false);
    this->setScrollingBehavior(ScrollingBehavior::CENTERED);
    this->setContentView(this->contentBox);

    this->registerFloatXMLAttribute("spanCount", [this](float value)
        { this->setSpanCount((int)value); });
    this->registerFloatXMLAttribute("itemWidth", [this](float value)
        { this->setItemWidth(value); });
    this->registerFloatXMLAttribute("itemSpace", [this](float value)
        { this->setItemSpace(value); });
    this->registerFloatXMLAttribute("preFetchLine", [this](float value)
        { this->setPreFetchLine((int)value); });
}

void HRecyclerFrame::setSpanCount(int count)
{
    this->spanCount = std::max(1, count);
    this->reloadData();
}

void HRecyclerFrame::setItemWidth(float width)
{
    this->itemSize = width;
    this->reloadData();
}

void HRecyclerFrame::setItemSpace(float space)
{
    this->itemSpace = space;
    this->reloadData();
}

void HRecyclerFrame::setPreFetchLine(int lines)
{
    this->preFetchLine = std::max(0, lines);
}

void HRecyclerFrame::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    this->recyclingLoop();
    HScrollingFrame::draw(vg, x, y, width, height, style, ctx);
}

void HRecyclerFrame::onThemeChanged()
{
    HScrollingFrame::onThemeChanged();
    this->queuedCellsThemeChanged();
}

void HRecyclerFrame::onLayout()
{
    HScrollingFrame::onLayout();

    float height = this->getHeight();
    if (std::isnan(height) || height <= 0 || (int)height == (int)this->oldHeight)
        return;
    this->oldHeight = height;
    this->layoutChanged();
}

View* HRecyclerFrame::getDefaultFocus()
{
    View* cell = this->defaultCell();
    return cell ? cell->getDefaultFocus() : nullptr;
}

void HRecyclerFrame::setPadding(float padding)
{
    this->setPadding(padding, padding, padding, padding);
}

void HRecyclerFrame::setPadding(float top, float right, float bottom, float left)
{
    this->paddingTop    = top;
    this->paddingRight  = right;
    this->paddingBottom = bottom;
    this->paddingLeft   = left;
    this->reloadData();
}

void HRecyclerFrame::setPaddingTop(float top)
{
    this->paddingTop = top;
    this->reloadData();
}

void HRecyclerFrame::setPaddingRight(float right)
{
    this->paddingRight = right;
    this->reloadData();
}

void HRecyclerFrame::setPaddingBottom(float bottom)
{
    this->paddingBottom = bottom;
    this->reloadData();
}

void HRecyclerFrame::setPaddingLeft(float left)
{
    this->paddingLeft = left;
    this->reloadData();
}

View* HRecyclerFrame::create()
{
    return new HRecyclerFrame();
}

} // namespace brls
