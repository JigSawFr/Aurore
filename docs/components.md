# Components

Views of Aurore beyond borealis' own. The demo shows them: the **Grid** tab (a grid and a row of pictures, a menu on each) and **Basic components** (icon buttons, a menu under a button).

## Recycled grid and row

`brls::RecyclingGrid` scrolls vertically, `brls::HRecyclerFrame` horizontally. Like `RecyclerFrame`, they keep only the cells on screen (and `preFetchLine` lines ahead on each side): the others go back to a queue and are given out again for other items. Every item has the same size. The focus moves between cells in both directions; past the grid's edges it leaves for the views around it.

```xml
<!-- As many 170 px columns as fit, rows 200 px tall -->
<brls:RecyclingGrid id="grid" grow="1" columnWidth="170" itemHeight="200" itemSpace="16" padding="6" />

<!-- One row of items 120 px wide -->
<brls:HRecyclerFrame id="row" height="150" itemWidth="120" itemSpace="16" />
```

| Attribute | View | Meaning |
|---|---|---|
| `spanCount` | both | Columns of a grid (rows of an `HRecyclerFrame`); default 1 |
| `columnWidth` | `RecyclingGrid` | Instead of `spanCount`: as many columns this wide as fit, sharing what is left |
| `itemHeight` | `RecyclingGrid` | A row's height |
| `itemWidth` | `HRecyclerFrame` | An item's width |
| `itemSpace` | both | Space between two items, both ways |
| `preFetchLine` | both | Lines kept ready beyond each end of the view; default 1 |
| `padding…` | both | Padding inside the scrolled content |

In code:

```cpp
class Cover : public brls::RecyclingGridItem
{
    void prepareForReuse() override { /* reset what an item set */ }
    void cacheForReuse() override { /* off screen: let go of a texture */ }
};

class Covers : public brls::RecyclingGridDataSource
{
    size_t getItemCount() override { return items.size(); }
    brls::RecyclingGridItem* cellForRow(brls::RecyclingView* recycler, size_t index) override
    {
        auto* cell = (Cover*)recycler->dequeueReusableCell("Cover");
        // fill the cell for items[index]
        return cell;
    }
    void onItemSelected(brls::View* recycler, size_t index) override { /* A on an item */ }
};

grid->registerCell("Cover", [] { return new Cover(); });
grid->onEndReached([&] { loadNextPage(); });  // then: grid->notifyDataChanged()
grid->setDataSource(new Covers());             // deleted by the grid
```

- `onEndReached()` fires once when the last item comes into view; it fires again after `notifyDataChanged()` brought more items, after `reloadData()`, `setDataSource()` or `forceRequestNextPage()`.
- `notifyDataChanged()`: items were added at the end; the cells and the scrolling stay. Fewer items than before reload everything.
- `reloadData()` builds the cells again, scrolled to `setDefaultCellFocus()`'s item; `selectRowAt(index, animated)` scrolls to an item.
- `getGridItemByIndex(index)` gives the cell of an item on screen, valid until the next frame.

They come from Switchfin's `recycling_grid` and `h_recycling` (Apache-2.0, see `NOTICE`), without its skeleton cells and its flow mode (items of different heights).
