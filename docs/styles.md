# Styles, transitions, data binding and accessibility

What a view can do beyond its own attributes: change its look while it has the focus, is pressed, disabled or under the mouse; animate those changes; follow values of the app's data; say what it is to a screen reader. A view that uses none of it keeps nothing more than a null pointer. The demo's *Styles and data* tab (`resources/xml/tabs/styles.xml`, `demo/src/tab/styles_tab.cpp`) shows all of it.

## State styles

An attribute prefixed with a state applies while the view is in that state, and the property goes back to what it was when no state styles it any more:

```xml
<brls:Box
    focusable="true"
    backgroundColor="@theme/brls/button/default_enabled_background"
    focus:backgroundColor="@theme/brls/accent"
    pressed:scale="0.94"
    hover:borderColor="#ffffff"
    disabled:alpha="0.4" />
```

| State | On while |
|---|---|
| `hover:` | the mouse pointer is over the view or one of its children (desktop; the nearest view with a `hover:` style takes it) |
| `focus:` | the view has the focus |
| `pressed:` | A is pressed on the focused view (the length of its click animation), or a touch or click is held on it |
| `disabled:` | the app says so (`view->setStyleState(brls::StyleState::DISABLED, true)`); a `Button` in `ButtonState::DISABLED` is |

When several states style the same property, the later one in this table wins.

Values are written as for the property itself: colours as `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(…)`, `rgba(…)` or `@theme/<name>`; numbers as `4`, `4px` or `@style/<name>`. A `@theme/` value is read each time the state comes on, and again when the theme changes, so it follows a theme loaded or switched since; a plain `textColor="@theme/…"` attribute does too (`View::setThemeColor`, docs/themes.md).

The properties are those that are drawn, never laid out (a size or a margin would move the screen around):

| View | Properties |
|---|---|
| every view | `backgroundColor`, `borderColor`, `lineColor`, `borderThickness`, `cornerRadius`, `alpha`, `scale` |
| `Label` | `textColor` |
| `Button` | `textColor` (its label's) |
| `Rectangle` | `color` |

In code: `view->setStateStyle(brls::StyleState::FOCUS, "backgroundColor", "@theme/brls/accent")`. A view of the app adds its own properties by overriding `getStyleProperty` and `setStyleProperty` (and calling the base for the others).

If the app changes a property itself while a state styles it, its value is the one the view goes back to.

## Transitions

`transition` animates the changes of the properties it names instead of applying them at once, the state styles' as well as those made in code (`setBackgroundColor`, `setAlpha`, `Label::setTextColor`…):

```xml
<brls:Box
    focus:backgroundColor="@theme/brls/accent"
    focus:cornerRadius="28"
    transition="backgroundColor 180ms ease-out, cornerRadius 300ms backOut" />
```

Each entry is a property (or `all`), a duration (`150ms`, `0.2s`, at most `60s`) and an easing: `linear`, `ease`, `ease-in`, `ease-out`, `ease-in-out` (cubic), or one of tweeny's by name (`quadraticOut`, `sinusoidalInOut`, `backOut`, `elasticOut`, `bounceOut`…; the default is `ease`). A change made while a transition runs starts from where it is. Colours mix channel by channel.

Transitions are skipped, the change applied at once, while `brls::Application::getReduceMotion()` is true.

The size and the place of a view are not animated: they come from the layout of the whole screen (yoga), and moving one view frame by frame means laying the screen out again at each frame, scrolling frames and lists included. A view can still be moved without layout with `scale` and its translation.

## Data binding

`brls::Observable<T>` (`borealis/core/observable.hpp`) is a value that tells its subscribers when it changes:

```cpp
brls::Observable<std::string> name { "Ada" };
label->bindText(name);   // the label shows "Ada"
name = "Grace";          // then "Grace"

brls::Subscription s = name.subscribe([](const std::string& value) { brls::Logger::info("{}", value); });
```

`set()` (or `=`) calls the subscribers only when the value changes (when `T` can be compared); `notify()` calls them anyway. The subscription ends when the `Subscription` is destroyed; a view keeps those of its bindings (`View::keepSubscription`) and they end with it, so an observable never calls into a view that is gone. Set observables from the UI thread (`brls::sync` from a worker).

For XML, observables are gathered in a `brls::DataModel`, registered under a name on the application (every screen) or on an activity (its screen, looked up first):

```cpp
brls::Observable<std::string> name { "Ada" };
brls::Observable<bool> online { true };
brls::Observable<NVGcolor> tint { nvgRGB(46, 196, 182) };

brls::DataModel profile;          // outlives its registration
profile.add("name", name);
profile.add("online", online);
profile.add("tint", tint);
brls::Application::registerModel("profile", &profile);   // or activity->registerModel(…)
```

```xml
<brls:Label bind="text: profile.name" />
<brls:Box bind="visible: profile.online, backgroundColor: profile.tint" />
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
<brls:Box focusable="true" accessibilityRole="button" accessibilityLabel="@i18n/app/play" />
<brls:Box focusable="true" accessibilityLabel="@i18n/app/battery" accessibilityValue="80%" />
```

The roles are `none`, `button`, `label`, `header`, `image`, `cell`, `toggle`, `slider`, `text_field`, `tab`, `list` and `dialog` (`brls::AccessibilityRole`). Without them a view says what it shows:

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

`brls::Application::setFocusAnnouncer` is called with the focused view's description each time the focus moves: its label, its role, its value, "selected", "disabled", then its place among its neighbours, in the app's language (`resources/i18n/<language>/a11y.json`, English when the app has none):

- "Wi-Fi, switch, on, 2 of 5", "Language, English, 3 of 5", "Save, button, disabled";
- the place ("2 of 5") is said for a button, a cell, a toggle, a slider, a text field or a tab: a recycler's cell its row in its section, any other one among the focusable views of its parent; nothing for a view alone;
- entering a dialog or a list (role `list` with a label) from outside, its description comes first: "Delete this profile?, dialog, Cancel, button, 1 of 2".

The same function is given what changes in place, outside of a focus move (`brls::Application::announce`):

- a value changed on the focused view by the user (a toggle flipped: "on"; a slider let go: "60%"; a cell's detail changed: its new detail; a radio cell checked: "selected"), within 2 s of a press or a touch, so that a value changing on its own (a clock) is not read out at each tick (`View::announceAccessibilityValue()`, which an app's own view calls after its value changed);
- every toast (`Application::notify`), its text.

That is where a text-to-speech engine, a screen reader of the system or a log plugs in:

```cpp
brls::Application::setFocusAnnouncer([](const std::string& text) {
    my_tts::say(text); // speech-dispatcher on Linux, SAPI on Windows, AVSpeechSynthesizer on iOS…
});
```

Aurore does not talk to a platform's accessibility API (AT-SPI, UI Automation, TalkBack, VoiceOver) itself: this function is the only way out. `Application::getLastAnnouncement()` is the last text given, announcer or not.

A focusable view with no label and no value (an `IconButton` without text, a picture) says its role at most, so it cannot be told apart from the next one: the first time each kind takes the focus, a warning names it (in a debug build, or whenever an announcer is set). The [inspector](inspector.md) shows each view's description and flags those; the test harness lists them (`Harness::silentFocusables()`, docs/testing.md).

`View::getAccessibilityDescription()` can be overridden for a view that says more.

## Reduced motion and text scale

Both are settings of the core, which Cadran's preferences feed:

- `brls::Application::setReduceMotion(true)`: the movements there only for show are left out: transitions, the focus highlight's glide, and Cadran's entrances, glides and breathing glow. `getReduceMotion()`, which transitions read, is also true while Cadran's effects leave movement out (Preferences › Visual effects "Reduced", the lighter effects in hand, `BRLS_STILL_FOCUS`); the highlight's glide follows the app's setting only.
- `brls::Application::setTextScale(1.25f)`: every label drawn that much larger (`Label::setFontScale`), and every screen measured and laid out again at once. A view of the app that measures text of its own is told through `View::onTextScaleChanged()`. Cadran's Preferences › Text size goes through it.

High contrast is a theme: the presets `builtin:high-contrast-dark` and `builtin:high-contrast-light` (docs/themes.md), offered by Cadran's palette picker, or any theme file (`brls::Theme::loadFile`, which warns about colours that read badly).
