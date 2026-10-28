# Views

The views Aurore registers for XML, what each one is for and where its class is. Every attribute of every tag, with its values, is in [xml-attributes.md](xml-attributes.md); [components.md](components.md) has the views Aurore added (grids, context menus, icon buttons) in more depth, and [styles.md](styles.md) the state styles, transitions and bindings every view takes.

## XML in short

```xml
<brls:Box axis="column" padding="20" gap="12" width="auto" height="auto">
    <brls:Label text="@i18n/my_app/title" fontSize="28" />
    <brls:Button id="ok" style="primary" text="OK" />
</brls:Box>
```

- A tag is a view, its children are added to it in order, its attributes are applied as it is made. A tag the app makes itself is registered with `Application::registerXMLView("MyView", MyView::create)` before the first XML that uses it ([getting-started.md](getting-started.md#4-a-view-of-your-own-3-minutes)).
- Values may refer to the resources: `@i18n/…` a string, `@theme/…` a theme colour (followed when the theme changes), `@style/…` a metric, `@res/…` a resource file. Sizes are in layout units (the screen is 1280 wide), or `auto`, or a percentage (`50%`).
- Every view takes the common attributes: `id`, the Yoga layout (`width`, `height`, `grow`, `shrink`, `margin…`, `position…`, `positionType`, `alignSelf`, `aspectRatio`, `boxSizing`…), its look (`backgroundColor`, `cornerRadius`, `border…`, `shadowType`, `alpha`, `visibility`), focus (`focusable`, `focusUp`… to route it, `hideHighlight`…), accessibility (`accessibilityLabel`…), `transition` and `bind`. `title` and `icon` are what an `AppletFrame` shows for its content.
- A screen's file is read with `CONTENT_FROM_XML_RES("activity/main.xml")` in an `Activity`, or `View::createFromXMLResource()`; a view of the app's fills itself with `inflateFromXMLRes()`.

## Layout

| Tag | Class (header) | What it is |
| --- | --- | --- |
| `brls:Box` | `Box` (`core/box.hpp`) | A flexbox: `axis` (`row`, `column`), `justifyContent`, `alignItems`, `direction`, `flexWrap`, `gap`, `rowGap`, `columnGap`, `padding…`. Any number of children |
| `brls:Padding` | `Padding` (`core/box.hpp`) | An empty view that grows: pushes the views after it to the end of its box |
| `brls:Rectangle` | `Rectangle` (`views/rectangle.hpp`) | A plain rectangle of `color` |
| `brls:ScrollingFrame` | `ScrollingFrame` (`views/scrolling_frame.hpp`) | Scrolls its one child vertically when it is taller; `scrollingBehavior` `natural` (scroll first) or `centered` (the focus in the middle) |
| `brls:HScrollingFrame` | `HScrollingFrame` (`views/h_scrolling_frame.hpp`) | The same, horizontally |

## Screens and frames

| Tag | Class (header) | What it is |
| --- | --- | --- |
| `brls:AppletFrame` | `AppletFrame` (`views/applet_frame.hpp`) | A screen with a header (its content's `title` and `icon`) and a footer of hints; one child, its content. `headerHidden`, `footerHidden`, `style` |
| `brls:TabFrame` | `TabFrame` (`views/tab_frame.hpp`) | A sidebar of tabs and the tab shown: children `<brls:Tab label="…">` (one view each, made when the tab is shown) and `<brls:Separator/>` |
| `brls:Sidebar` | `Sidebar` (`views/sidebar.hpp`) | The list of tabs of a `TabFrame`, usable alone |
| `brls:Header` | `Header` (`views/header.hpp`) | A section's header in a list: `title`, `subtitle`, a bar before and a line under |
| `brls:BottomBar` | `BottomBar` (`views/bottom_bar.hpp`) | The footer: the hints, the clock, the battery and the network when the platform has them |
| `brls:Hints` | `Hints` (`views/hint.hpp`) | The hints of the focused view's actions; Cadran puts its `FooterHints` in its place. `addBaseAction`, `forceShown`, `allowAButtonTouch` |

Dialogs, dropdowns, the text input dialog and context menus are made in code, not XML: `Dialog` (`views/dialog.hpp`), `Dropdown` (`views/dropdown.hpp`), `EditTextDialog` (`views/edit_text_dialog.hpp`), `ContextMenu` (`views/context_menu.hpp`). `DebugLayer` (`views/debug_layer.hpp`) is drawn over the app by `Application::enableDebuggingView(true)`.

## Content

| Tag | Class (header) | What it is |
| --- | --- | --- |
| `brls:Label` | `Label` (`views/label.hpp`) | Text: `text`, `fontSize`, `textColor`, `fontFamily`, `lineHeight`, `horizontalAlign`, `verticalAlign`, `singleLine`, `animated` (a text too long scrolls), `autoAnimate` |
| `brls:Image` | `Image` (`views/image.hpp`) | A picture: `image` (`@res/…` or a path), `themeImage` (a theme's image slot), `scalingType` (`fit`, `fill`, `stretch`, `center`), `imageAlign`, `interpolation` |
| `brls:ProgressSpinner` | `ProgressSpinner` (`views/progress_spinner.hpp`) | A spinner while something loads: `size`, `barColor` |
| `brls:Slider` | `Slider` (`views/slider.hpp`) | A value from 0 to 1 moved with the D-pad or a finger, in steps or not |

## Controls

| Tag | Class (header) | What it is |
| --- | --- | --- |
| `brls:Button` | `Button` (`views/button.hpp`) | A button: `text`, `style` (`default`, `primary`, `highlight`, `bordered`, `borderless`), `state` (`enabled`, `disabled`), and the label's text attributes |
| `brls:IconButton` | `IconButton` (`views/icon_button.hpp`) | A button with an icon before its text, or round with an icon alone: `icon`, `glyph` (a Material Icons glyph), `iconSize` ([components.md](components.md)) |
| `brls:CheckBox` | `CheckBox` (`views/cells/cell_radio.hpp`) | The check mark of a `RadioCell` |

## Lists and cells

| Tag | Class (header) | What it is |
| --- | --- | --- |
| `brls:RecyclerFrame` | `RecyclerFrame` (`views/recycler.hpp`) | A long vertical list that makes only the cells on screen; rows from a `RecyclerDataSource` in code |
| `brls:RecyclingGrid` | `RecyclingGrid` (`views/recycling_grid.hpp`) | The same as a grid: `spanCount` or `columnWidth`, `itemHeight`, `itemSpace`, `preFetchLine` |
| `brls:HRecyclerFrame` | `HRecyclerFrame` (`views/recycling_grid.hpp`) | A row of items, scrolled sideways: `itemWidth`, `itemSpace` |
| `brls:DetailCell` | `DetailCell` (`views/cells/cell_detail.hpp`) | A line with a `title` and a detail on the right; the base of the cells below |
| `brls:BooleanCell` | `BooleanCell` (`views/cells/cell_bool.hpp`) | A line with an on / off switch |
| `brls:RadioCell` | `RadioCell` (`views/cells/cell_radio.hpp`) | A line with a check mark, one of several |
| `brls:SelectorCell` | `SelectorCell` (`views/cells/cell_selector.hpp`) | A line that opens a `Dropdown` of values |
| `brls:InputCell` | `InputCell` (`views/cells/cell_input.hpp`) | A line that opens the keyboard for a text (`secure` hides it) |
| `brls:InputNumericCell` | `InputNumericCell` (`views/cells/cell_input.hpp`) | The same for a number |
| `brls:SliderCell` | `SliderCell` (`views/cells/cell_slider.hpp`) | A line with a `Slider` |

## Status widgets

| Tag | Class (header) | What it is |
| --- | --- | --- |
| `brls:Battery` | `BatteryWidget` (`views/widgets/battery.hpp`) | The battery's level and charge |
| `brls:Wireless` | `WirelessWidget` (`views/widgets/wireless.hpp`) | The network's signal |
| `brls:Account` | `AccountWidget` (`views/widgets/account.hpp`) | The user's account (the Switch's) |

## Cadran's views

[Cadran](cadran.md)'s views are `brls::View`s in `cadran/view/`, made in code. Those with a `create()` can be registered for XML under the name the app chooses, as the apps on Cadran do:

```cpp
brls::Application::registerXMLView("ScrollView", ScrollView::create);
brls::Application::registerXMLView("CadranIcon", CadranIcon::create);
brls::Application::registerXMLView("CadranDigits", CadranDigits::create);
brls::Application::registerXMLView("LoadingBar", LoadingBar::create);
```

| Class (header) | What it is |
| --- | --- |
| `ScrollView` (`cadran/view/scroll_view.hpp`) | Cadran's scrolling frame: each press moves to the next line and scrolls just enough to show it |
| `CadranIcon`, `CadranSwatch` (`cadran/view/cadran_icon.hpp`) | About 60 vector icons, which a theme may replace with pictures; a palette's swatch |
| `CadranDigits` (`cadran/view/cadran_digits.hpp`) | Text in the display face (Sora): figures, values |
| `LoadingBar` (`cadran/view/loading_bar.hpp`) | A progress bar that eases to each value |
| `QrView` (`cadran/view/qr_view.hpp`) | A QR code of a text |
| `markdown_view::fill` (`cadran/view/markdown_view.hpp`) | A Markdown text as labels |
| `TabBadge`, `VersionBadge`, `HeaderSetting`, `FooterHints` (`cadran/view/`) | The pieces of Cadran's frame, which Cadran installs itself |
