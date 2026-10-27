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

Each entry is a property (or `all`), a duration (`150ms`, `0.2s`) and an easing: `linear`, `ease`, `ease-in`, `ease-out`, `ease-in-out` (cubic), or one of tweeny's by name (`quadraticOut`, `sinusoidalInOut`, `backOut`, `elasticOut`, `bounceOut`…; the default is `ease`). A change made while a transition runs starts from where it is. Colours mix channel by channel.

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

Every view has a role and a label, for what is said when it takes the focus:

```xml
<brls:Box focusable="true" accessibilityRole="button" accessibilityLabel="@i18n/app/play" />
```

The roles are `none`, `button`, `label`, `header`, `image`, `cell`, `toggle`, `slider`, `text_field`, `tab` and `list` (`brls::AccessibilityRole`). Without them a view says what it shows: a `Label` its text, a `Button` its text, a cell its title and its value (a `BooleanCell` is a `toggle`, a `SliderCell` a `slider`, an input cell a `text_field`), a sidebar's item its label (a `tab`).

`brls::Application::setFocusAnnouncer` is called with the focused view's description each time the focus moves: its label, then its role and `disabled` in the app's language (`resources/i18n/<language>/a11y.json`, English when the app has none), as in "Save, button, disabled". That is where a text-to-speech engine, a screen reader of the system or a log plugs in:

```cpp
brls::Application::setFocusAnnouncer([](const std::string& description) {
    my_tts::say(description);
});
```

`View::getAccessibilityDescription()` can be overridden for a view that says more (a state of its own).

## Reduced motion and text scale

Both are settings of the core, which Cadran's preferences feed:

- `brls::Application::setReduceMotion(true)`: the movements there only for show are left out: transitions, the focus highlight's glide, and Cadran's entrances, glides and breathing glow. borealis' own views follow it too: a list scrolls straight to the focused line, a text cut short stays cut (ending with "…") instead of scrolling, a toast and a dropdown fade in where they stay, a switch cell does not bounce. `getReduceMotion()`, which transitions read, is also true while Cadran's effects leave movement out (Preferences › Visual effects "Reduced", the lighter effects in hand, `BRLS_STILL_FOCUS`); the highlight's glide follows the app's setting only.
- `brls::Application::setTextScale(1.25f)`: every label drawn that much larger (`Label::setFontScale`), and every screen measured and laid out again at once. A view of the app that measures text of its own is told through `View::onTextScaleChanged()`. Cadran's Preferences › Text size goes through it.

High contrast is a theme: the presets `builtin:high-contrast-dark` and `builtin:high-contrast-light` (docs/themes.md), offered by Cadran's palette picker, or any theme file (`brls::Theme::loadFile`, which warns about colours that read badly).
