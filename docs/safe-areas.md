# Safe areas

A phone's screen has a notch or a punch hole, rounded corners, a status bar and a home indicator; a TV may crop the edges of the picture. The **safe area** is the part of the window clear of all of them. Aurore draws edge to edge: backgrounds fill the whole window, and the views that hold text and controls keep clear of the edges the safe area takes. Nothing to do in an app built on `AppletFrame` (or `TabFrame`, Cadran's frames): the frame does it.

## What reports it

| Platform | The safe area |
| --- | --- |
| Android | The system bars, the display cutout and the rounded corners over the SDL surface (`PlatformUtils.getSafeAreaInsets`, through JNI): `WindowInsets` of the bars and the cutout from Android 11, the system window insets and `DisplayCutout` before, nothing before Android 6. A rounded corner of radius *r* keeps *r* (1 − 1/√2) from both its edges (a rectangle's corner set in that far stays inside it). The keyboard is not part of it |
| iOS, tvOS | `UIWindow.safeAreaInsets` (under SDL3, through `SDL_GetWindowSafeArea`, which reads it): the notch, the rounded corners, the home indicator; on tvOS, the TV's safe area |
| Desktops, Switch, PS4, PS Vita, New 3DS, a browser | Nothing: zero, unless set (below) |

The platform says it in the unit of the window's size (points on iOS, pixels on Android) through `Platform::getSafeAreaInsets()`. `Application::getSafeArea()` has it in layout units, whole ones, never more than a third of the window from an edge, and never less than the overscan margin.

It is read again when the window changes size, and on Android and iOS when the window is shown or comes back, and when the display turns (a turn of 180 degrees keeps the size and moves the notch to the other side), and under SDL3 when SDL says it changed: then once more 300 ms later, when the system has laid the window out. A change lays the views out again and fires `Application::getSafeAreaChangedEvent()` with the new insets. A platform of an app's own calls `Application::updateSafeArea()` when its safe area may have changed.

## On a desktop, and a TV

`AURORE_SAFE_AREA` shows a phone's safe area on the desktop, in a build with `AURORE_DEV_TOOLS` ([environment.md](environment.md)): `AURORE_SAFE_AREA=47,0,34,0` (an iPhone held upright: top, right, bottom, left), `AURORE_SAFE_AREA=0,48` (a notch on either side), `AURORE_SAFE_AREA=30` (every edge), in the window's unit as a CSS padding is written. In code, `Application::setSafeAreaOverride(&insets)` does the same (`nullptr` goes back to the platform's).

`Application::setOverscan(fraction)` keeps every edge at least that fraction of the window clear: `0.05` is a TV's title safe area (5 %), for a setting of an app on a console or a TV box plugged into a set that crops the picture. 0 by default, at most 0.25.

## In the layout

`Box::setSafeArea(edges)`, `safeArea="…"` in XML, says which edges of a box keep clear of the safe area: there, the safe area is added to the box's padding, so the box's children move in while its background still reaches the window's edge.

| Value | Edges |
| --- | --- |
| `auto` (the default) | none, but all of them for the content view of an activity |
| `none` | none |
| `top`, `right`, `bottom`, `left` | that one; joined with `\|` (`top\|bottom`), commas or spaces |
| `vertical`, `horizontal`, `all` | top and bottom, left and right, every edge |

```xml
<aurore:Box axis="column" safeArea="none" backgroundColor="@theme/aurore/background">
    <!-- a bar at the top: its colour up to the window's edge, its text below the notch -->
    <aurore:Box height="60" boxSizing="contentBox" safeArea="top|horizontal"
        backgroundColor="@theme/aurore/accent">
        <aurore:Label text="Title"/>
    </aurore:Box>
    <aurore:ScrollingFrame grow="1" safeArea="horizontal|bottom">
        …
    </aurore:ScrollingFrame>
</aurore:Box>
```

The rules:

- **Once on each path.** A box that keeps clear of an edge does it for the views in it: they take the safe area no more. A box that keeps clear of none (`none`, or `auto` below the content view) lets them. So the content view of an activity keeps clear of every edge by default (`auto`), and the views in it do nothing; give it `safeArea="none"` to go edge to edge, and set the edges on the views at the window's edges.
- **The window's safe area.** The inset added is the window's, whatever the box's place: set the edges of boxes that touch those edges of the window.
- **Fixed sizes.** A box with a fixed height is that high with the safe area in it, its content squeezed. `boxSizing="contentBox"` makes it grow by the safe area instead, as a header does.
- **Your own padding.** `getPaddingTop()` and the others give the box's own padding, and `setPadding…()` set it: the safe area stays added to it. `getSafeAreaPadding()` gives what is added now.
- **Absolute views.** A view positioned absolutely (`positionType="absolute"`) ignores its parent's padding, the safe area's too: place it from `Application::getSafeArea()`.

Since nothing focusable lies in a box's padding, nothing can be focused or tapped under the insets, unless placed there absolutely.

## The frames

An `AppletFrame` (and so a `TabFrame`, a Cadran screen) keeps clear of the safe area itself, edge to edge:

- the frame keeps clear of the left and the right: its header, content and footer move in, the app's background behind them;
- the header (content-box) grows by the top inset: its backdrop reaches the top of the window, its title and tools sit below the notch;
- the footer (content-box) grows by the bottom inset, its hints above the home indicator;
- with the header or the footer hidden (`headerHidden`, `setHeaderVisibility`), the frame keeps clear of that edge for the content;
- the content takes nothing more: a frame inside it (a `TabFrame` in an `AppletFrame`) does nothing again.

`setSafeArea()` on a frame (or `safeArea` on its XML) replaces this: the frame then keeps clear of the edges it is given, its header and footer of nothing more.

The toasts (`Application::notify`) keep clear of the top and the right; a dialog, a dropdown or an action sheet is the content view of its activity: all its edges.

## Testing it

`Harness::setSafeArea(insets)` has the harness' platform report a safe area (framebuffer pixels) and lays the screen out again; `tests/demo_tests.cpp` checks the demo's frame, a dialog and XML layouts with one, and their screenshots (`safe_area_main_screen`, `safe_area_xml`).

On a device, the safe area is logged when it changes (`Safe area: top …`).
