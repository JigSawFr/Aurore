# Inspector

A panel drawn over the app, for finding out why a view sits where it sits, which colours it was drawn with, where the focus is, and what a frame costs. It works on every platform, the Switch included.

## Turning it on

It is off unless asked for, and costs nothing then (one key and two buttons read per frame):

- in code, at any time: `brls::Application::setInspectorEnabled(true);`
- in a debug build: `-DBRLS_INSPECTOR=ON` at configure time;
- on a desktop, for one run: `BRLS_INSPECTOR=1 ./my_app` (`BRLS_INSPECTOR=0` turns it off again, even in a build that has it on), in a build with the debugging aids: not a `Release` or `MinSizeRel` one unless `-DBRLS_DEV_TOOLS=ON` ([hot reload](hot_reload.md#turning-it-on)).

The demo turns it on.

## Opening it

**F12** on a keyboard, or **L3 + R3** (both sticks pressed in together) on a controller. The Switch's own combinations (Home, Capture, L + R on the controller screens) are left alone, and the two stick buttons are taken from the app while the combination is held. Each press goes one step further:

1. **Live**: the panel follows the focus. The app keeps every input: move around it, and the panel shows the view focused.
2. **Browse**: the inspector takes the controller, and the keyboard keys mapped to it, to walk the tree; the app gets none of it (touch and mouse still reach it).
3. Closed again.

In browse mode:

| Button | Key | |
| --- | --- | --- |
| Up, Down | arrows | previous, next view (held: repeats) |
| L, R | | a page up, down |
| Left | left arrow | the parent |
| Right | right arrow | the first child |
| A | Enter | give the focus to the view |
| Y | | select the view focused |
| X | | move the panel to the other side |
| B | Escape | back to live |

## What it shows

**The frame**, averaged over the last 30 frames, as `brls::Profiler` measured them (it measures while the inspector is open, [testing.md](testing.md#the-bench) has the rest of what it records):

- `frame`: the time from one frame to the next (and the longest), and the frames per second;
- `views`: drawing the activities, the highlight and the notifications (nanovg's calls, before anything reaches the GPU);
- `layout`: yoga's layouts since the frame before, and how many there were;
- `flush`: `nvgEndFrame`, where nanovg hands its work to the GPU; `present`: the swap, which waits for the screen when vsync is on (the inspector's own drawing included);
- `nanovg`: the draw calls and triangles of the app's drawing, the inspector's left out;
- a graph of the last 120 frames, the line at 60 fps, the slow frames in red.

While the inspector is open every frame is drawn, redraw on demand or not, so that its figures move.

**The tree** of every activity, the one on top first: each view by its class and `#id`, and its size. The view focused is marked `>` and the views it is in `|` (the focus chain); a view that is not visible is greyed; a view that takes the focus with no accessibility label nor value is flagged `no a11y` in red. The selected view is drawn over the screen as browsers do: its margin in orange, border in yellow, padding in green and content in blue, with its name and size on a tag; the view focused, when another one is selected, is outlined in blue.

**The selected view**: its position and size, the width and height asked of yoga (`auto`, points or `%`), its margin, border and padding (top, right, bottom, left, or one value when they are all the same), its flex grow, shrink, basis and direction, its visibility and alpha, whether it can take the focus, its accessibility role and what the focus announcer says of it (in red when it takes the focus with no label: docs/styles.md), its background, border and corner radius, and for a label its font size, colour and text; a colour that follows a theme key (`View::setThemeColor`, `@theme/…` in XML) shows its key.

**What it looked up in the theme** while it drew itself (not its children): each colour (`Theme::getColor`) with its value and a swatch, each metric (`Style`). Values read once when the view was made, or given by key, are not there: the keys show next to the colours above.

**The focus chain**, from the root down to the view focused, at the bottom.

## For the curious

`library/lib/core/inspector.cpp`. The panel is drawn with nanovg directly, not with views: it neither changes the tree it shows nor takes the focus. The timings and nanovg's counters (read through `nvgDebugGetStats()`, added to Aurore's nanovg) are the `FrameProfile`s of `library/lib/core/profiler.cpp`, which also has the time the panel itself took (`inspector`).
