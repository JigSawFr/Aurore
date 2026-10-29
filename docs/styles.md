# Styles, transitions, data binding and accessibility

What a view can do beyond its own attributes: change its look while it has the focus, is pressed, disabled or under the mouse; animate those changes; follow values of the app's data; say what it is to a screen reader. A view that uses none of it keeps nothing more than a null pointer. The demo's *Styles and data* tab (`resources/xml/tabs/styles.xml`, `demo/src/tab/styles_tab.cpp`) shows all of it.

## State styles

An attribute prefixed with a state applies while the view is in that state, and the property goes back to what it was when no state styles it any more:

```xml
<aurore:Box
    focusable="true"
    backgroundColor="@theme/aurore/button/default_enabled_background"
    focus:backgroundColor="@theme/aurore/accent"
    pressed:scale="0.94"
    hover:borderColor="#ffffff"
    disabled:alpha="0.4" />
```

| State | On while |
|---|---|
| `hover:` | the mouse pointer is over the view or one of its children (desktop; the nearest view with a `hover:` style takes it) |
| `focus:` | the view has the focus |
| `pressed:` | A is pressed on the focused view (the length of its click animation), or a touch or click is held on it |
| `disabled:` | the app says so (`view->setStyleState(aurore::StyleState::DISABLED, true)`); a `Button` in `ButtonState::DISABLED` is |

When several states style the same property, the later one in this table wins.

Values are written as for the property itself: colours as `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(…)`, `rgba(…)` or `@theme/<name>`; numbers as `4`, `4px`, `4dp` (at the density in use, below) or `@style/<name>`. A `@theme/` value is read each time the state comes on, and again when the theme changes, so it follows a theme loaded or switched since; a plain `textColor="@theme/…"` attribute does too (`View::setThemeColor`, docs/themes.md).

The properties are those that are drawn, never laid out (a size or a margin would move the screen around):

| View | Properties |
|---|---|
| every view | `backgroundColor`, `borderColor`, `lineColor`, `borderThickness`, `cornerRadius`, `alpha`, `scale` |
| `Label` | `textColor` |
| `Button` | `textColor` (its label's) |
| `Rectangle` | `color` |

In code: `view->setStateStyle(aurore::StyleState::FOCUS, "backgroundColor", "@theme/aurore/accent")`. A view of the app adds its own properties by overriding `getStyleProperty` and `setStyleProperty` (and calling the base for the others).

If the app changes a property itself while a state styles it, its value is the one the view goes back to.

## Transitions

`transition` animates the changes of the properties it names instead of applying them at once, the state styles' as well as those made in code (`setBackgroundColor`, `setAlpha`, `Label::setTextColor`…):

```xml
<aurore:Box
    focus:backgroundColor="@theme/aurore/accent"
    focus:cornerRadius="28"
    transition="backgroundColor 180ms ease-out, cornerRadius 300ms backOut" />
```

Each entry is a property (or `all`), a duration (`150ms`, `0.2s`, at most `60s`) and an easing: `linear`, `ease`, `ease-in`, `ease-out`, `ease-in-out` (cubic), or one of tweeny's by name (`quadraticOut`, `sinusoidalInOut`, `backOut`, `elasticOut`, `bounceOut`…; the default is `ease`). A change made while a transition runs starts from where it is. Colours mix channel by channel.

Transitions are skipped, the change applied at once, while `aurore::Application::getReduceMotion()` is true.

The size and the place of a view are not animated: they come from the layout of the whole screen (yoga), and moving one view frame by frame means laying the screen out again at each frame, scrolling frames and lists included. A view can still be moved without layout with `scale` and its translation.

## Data binding

`aurore::Observable<T>` (`aurore/core/observable.hpp`) is a value that tells its subscribers when it changes:

```cpp
aurore::Observable<std::string> name { "Ada" };
label->bindText(name);   // the label shows "Ada"
name = "Grace";          // then "Grace"

aurore::Subscription s = name.subscribe([](const std::string& value) { aurore::Logger::info("{}", value); });
```

`set()` (or `=`) calls the subscribers only when the value changes (when `T` can be compared); `notify()` calls them anyway. The subscription ends when the `Subscription` is destroyed; a view keeps those of its bindings (`View::keepSubscription`) and they end with it, so an observable never calls into a view that is gone. Set observables from the UI thread (`aurore::sync` from a worker).

For XML, observables are gathered in a `aurore::DataModel`, registered under a name on the application (every screen) or on an activity (its screen, looked up first):

```cpp
aurore::Observable<std::string> name { "Ada" };
aurore::Observable<bool> online { true };
aurore::Observable<NVGcolor> tint { nvgRGB(46, 196, 182) };

aurore::DataModel profile;          // outlives its registration
profile.add("name", name);
profile.add("online", online);
profile.add("tint", tint);
aurore::Application::registerModel("profile", &profile);   // or activity->registerModel(…)
```

```xml
<aurore:Label bind="text: profile.name" />
<aurore:Box bind="visible: profile.online, backgroundColor: profile.tint" />
```

`bind` takes `property: model.key` pairs separated by commas:

| Property | Observable | Views |
|---|---|---|
| `text` | `std::string`, `int` or `float` | `Label`, `Button`, the cells (their title) |
| `visible` | `bool` (`VISIBLE` or `GONE`) | every view |
| a style property (`backgroundColor`, `alpha`, `textColor`…) | `NVGcolor` or `float` | as above; a `transition` on it animates the change |

The binding is made when the view joins an activity (the XML is read before), against the activity's models then the application's; a model registered later is looked up then. A key the model does not have, or of the wrong type, is logged as an error.

## Accessibility

Every view has a role, a label, a value and a selected state, for what is said when it takes the focus:

```xml
<aurore:Box focusable="true" accessibilityRole="button" accessibilityLabel="@i18n/app/play" />
<aurore:Box focusable="true" accessibilityLabel="@i18n/app/battery" accessibilityValue="80%" />
```

The roles are `none`, `button`, `label`, `header`, `image`, `cell`, `toggle`, `slider`, `text_field`, `tab`, `list` and `dialog` (`aurore::AccessibilityRole`). Without them a view says what it shows:

| View | Role | Label | Value, state |
| --- | --- | --- | --- |
| `Label` | label | its text | |
| `Button`, `IconButton` | button | its text (give an icon alone an `accessibilityLabel`) | |
| `DetailCell`, `SelectorCell`, input cells | cell, text field | its title | its detail and the other texts in it (a "›" left out) |
| `BooleanCell` | toggle | its title | "on" or "off", its detail shown or not |
| `RadioCell` | cell | its title | "selected" when it is |
| `SliderCell` (its knob takes the focus) | slider | the cell's title | "40%" |
| sidebar item | tab | its label | "selected" for the tab shown |
| `Dialog` | dialog | its text | |
| any other focusable `Box` | none | the texts shown in it, a focusable view in it left out (it speaks for itself) | |

In code: `setAccessibilityRole`, `setAccessibilityLabel`, `setAccessibilityValue` and `setAccessibilitySelected` (XML `accessibilitySelected="true"`); a view of the app's overrides `getDefaultAccessibilityRole()`, `getDefaultAccessibilityLabel()`, `getDefaultAccessibilityValue()` or `getDefaultAccessibilitySelected()` to say what it shows as it changes.

`aurore::Application::setFocusAnnouncer` is called with the focused view's description each time the focus moves: its label, its role, its value, "selected", "disabled", then its place among its neighbours, in the app's language (`resources/i18n/<language>/a11y.json`, English when the app has none):

- "Wi-Fi, switch, on, 2 of 5", "Language, English, 3 of 5", "Save, button, disabled";
- the place ("2 of 5") is said for a button, a cell, a toggle, a slider, a text field or a tab: a recycler's cell its row in its section, any other one among the focusable views of its parent; nothing for a view alone;
- entering a dialog or a list (role `list` with a label) from outside, its description comes first: "Delete this profile?, dialog, Cancel, button, 1 of 2".

The same function is given what changes in place, outside of a focus move (`aurore::Application::announce`):

- a value changed on the focused view by the user (a toggle flipped: "on"; a slider let go: "60%"; a cell's detail changed: its new detail; a radio cell checked: "selected"), within 2 s of a press or a touch, so that a value changing on its own (a clock) is not read out at each tick (`View::announceAccessibilityValue()`, which an app's own view calls after its value changed);
- every toast (`Application::notify`), its text.

That is where a text-to-speech engine, a screen reader of the system or a log plugs in:

```cpp
aurore::Application::setFocusAnnouncer([](const std::string& text) {
    my_tts::say(text); // speech-dispatcher on Linux, SAPI on Windows, AVSpeechSynthesizer on iOS…
});
```

Aurore does not talk to a platform's accessibility API (AT-SPI, UI Automation, TalkBack, VoiceOver) itself: this function is the only way out. `Application::getLastAnnouncement()` is the last text given, announcer or not.

A focusable view with no label and no value (an `IconButton` without text, a picture) says its role at most, so it cannot be told apart from the next one: the first time each kind takes the focus, a warning names it (in a debug build, or whenever an announcer is set). The [inspector](inspector.md) shows each view's description and flags those; the test harness lists them (`Harness::silentFocusables()`, docs/testing.md).

`View::getAccessibilityDescription()` can be overridden for a view that says more.

## Density of the lengths

The density shrinks or widens the lengths of a screen (margins, paddings, heights, radii, the sidebar's width) without touching the text, for a small screen or a user who wants more on it:

| Density | Factor | For |
|---|---|---|
| `aurore::DENSITY_COMFORTABLE` | 1 | the default: the screens as designed |
| `aurore::DENSITY_COMPACT` | 0.85 | more lines on the screen |
| `aurore::DENSITY_DENSE` | 0.7 | a small screen |

```cpp
aurore::Application::setDensity(aurore::DENSITY_COMPACT); // before the first screen
```

What follows it:

- the metrics of kind length (`aurore::MetricKind::LENGTH`): those of `library/lib/core/style.cpp` but the durations, the font sizes, the opacities and the ratios, Cadran's corners (`cadran/radius/*`), and those an app adds with `Style::addLength`. A metric added with `addMetric` keeps the kind of the name it replaces, and is not a length when the name is new; a theme file's `[metrics]` are written at density 1 and follow it too. `StyleValues::getRawMetric` reads one as written;
- a number in `dp` in XML (`paddingLeft="16dp"`), state styles included, and `aurore::dp(16)` in code.

What does not: the text (its scale is `Application::setTextScale`, below), a plain number or one in `px`, durations. A screen whose sizes are all plain numbers looks the same at every density: the density reaches a screen through `dp` and the style, as it moves to them.

The views read their sizes when they are made: set the density before the first screen, or make the screens again after changing it. The value is kept between `DENSITY_MIN` (0.5) and `DENSITY_MAX` (1.5). On the desktop, `AURORE_DENSITY=compact` (or `dense`, or a number) sets it at start, to try a screen at another density.

## Screen width

A screen is compact, medium or expanded by its width in layout units (`Application::contentWidth`): `aurore::WidthClass::COMPACT` under 720, `MEDIUM` under 1080, `EXPANDED` from 1080 (the 1280-unit design width). `Application::getWidthClass()` gives it; `aurore::widthClassOf(width)` the class of any width.

The design width sets it: `Application::ORIGINAL_WINDOW_WIDTH = 480` (before `init()`) lays a 320-pixel screen out on 480 units, a compact one, its text 0.67 times as large as written. On the desktop, `AURORE_DESIGN_SIZE=480x360` does the same at start, to try a small screen's layout in a window.

A layout preset sets the design size, the density and the text scale in one go: `Application::usePreset(aurore::PRESET_SMALL_SCREEN)` (before `init()`) lays a small screen out on 480x360 units, dense, its text 1.25 times larger, 12 to 17 pixels on a 320x240 one; `PRESET_STANDARD` is the usual 1280x720. A platform whose screen is small gives its own (`Platform::getLayoutPreset`), the app's wins over it, and `AURORE_PRESET=small` (or `standard`) wins over both on the desktop. Cadran's text size setting multiplies the preset's text scale (`Application::getPresetTextScale`).

In XML, an attribute prefixed with a class applies on such a screen only, over the plain one whatever their order; on the other screens its name is checked, its value is not read:

```xml
<aurore:Box
    axis="row"
    compact:axis="column"
    width="400"
    compact:width="100%"
    expanded:marginLeft="40dp" />
```

A state may follow (`compact:focus:cornerRadius="4"`). The attributes are read when the view is made: a screen that changes class (a window resized past a threshold) is laid out for its new class when it is made again. A view laid out in code reads `getWidthClass()` the same way.

On a compact screen the library's own views make room for the content: a `Header` puts its subtitle under its title, on one line; the footer leaves out the clock, battery and Wi-Fi, its hints taking the row; dialogs take 94% of the width. With Cadran, the header is lower and without its language, theme and text size buttons (Preferences has them), the rail on its icons narrower, a tab's margins smaller, and action sheets and pickers take the screen's width.

## Reduced motion and text scale

Both are settings of the core, which Cadran's preferences feed:

- `aurore::Application::setReduceMotion(true)`: the movements there only for show are left out: transitions, the focus highlight's glide, and Cadran's entrances, glides and breathing glow. borealis' own views follow it too: a list scrolls straight to the focused line, a text cut short stays cut (ending with "…") instead of scrolling, a toast and a dropdown fade in where they stay, a switch cell does not bounce. `getReduceMotion()`, which transitions read, is also true while Cadran's effects leave movement out (Preferences › Visual effects "Reduced", the lighter effects in hand, `AURORE_STILL_FOCUS`); the highlight's glide follows the app's setting only.
- `aurore::Application::setTextScale(1.25f)`: every label drawn that much larger (`Label::setFontScale`), and every screen measured and laid out again at once. A view of the app that measures text of its own is told through `View::onTextScaleChanged()`. The cells (`DetailCell`, `RadioCell` and those made from them) grow with it: their height becomes a minimum height that many times larger, so the text keeps its padding. Cadran's Preferences › Text size (×1.25 or ×1.5) goes through it.

High contrast is a theme: the presets `builtin:high-contrast-dark` and `builtin:high-contrast-light` (docs/themes.md), offered by Cadran's palette picker, or any theme file (`aurore::Theme::loadFile`, which warns about colours that read badly).
