# Components

Views of Aurore beyond borealis' own. The demo shows them: the **Grid** tab (a grid and a row of pictures, a menu on each: X or a long press) and **Basic components** (icon buttons, a menu under a button).

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

## Context menu

`brls::ContextMenu` is a short list of actions in a popup next to a view: below it, or above when there is no room, kept on screen. The screen stays visible around it, lightly dimmed. B, a touch or a click outside it, or choosing a line closes it; a line's action runs once it is closed.

```cpp
auto* menu = new brls::ContextMenu(anchor);       // the view it opens next to
menu->setTitle("Bulbasaur");
menu->addItem("Open", [] { /* … */ }, "\uE89E");  // a Material Icons glyph
menu->addItem("Rename", [] { /* … */ }, "@res/img/rename.png");
menu->addSeparator();
menu->addItem("Delete", [] { /* … */ }, cadran::kit::icon("trash", 22, "cadran/coral")); // any view
menu->open();
```

`ContextMenu::attach(view, fill)` opens one with X on `view` (its hint reads "Options", or the text given) and with a long press on it; `fill` fills the menu each time it opens:

```cpp
brls::ContextMenu::attach(cell, [cell](brls::ContextMenu* menu) {
    menu->addItem("Open", [cell] { open(cell->getIndex()); });
});
```

The long press is a gesture recognizer of its own, `brls::LongPressGestureRecognizer(view, respond, duration = 500, slop = 16)`: a touch or the mouse's left button held still for `duration` ms. It takes over from the tap under it, which then does not click.

## Icon button

`brls::IconButton` is a `Button` with an icon before its text, or an icon alone in a round button when it has no text. Every `Button` attribute and style applies.

```xml
<brls:IconButton glyph="&#xE5D4;" />                                     <!-- round, a glyph -->
<brls:IconButton style="primary" glyph="&#xE037;" text="Play" />
<brls:IconButton style="bordered" icon="@res/img/logo.png" iconSize="28" text="Aurore" />
```

| Attribute | Meaning |
|---|---|
| `glyph` | A Material Icons glyph, drawn in the text's colour (it follows the style and the state) |
| `icon` | A picture: `@res/…` or a file path |
| `iconSize` | The icon's side; default 24 |

In code: `setGlyph()`, `setIcon()`, `setIconView(view)` (a view of the app's, a `CadranIcon`…), `setIconSize()`, `setText()`.
