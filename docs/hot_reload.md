# Hot reload

On a desktop, the screen shown is built again when one of its XML files, or the theme file, is saved: no rebuild, no restart, no walking back to the screen.

## Turning it on

It is off unless asked for:

- in code, after `init()`: `brls::Application::setHotReload(true);` or `setHotReload(true, "/path/to/my_app/resources");`
- for one run: `BRLS_HOT_RELOAD=1 ./my_app`, or `BRLS_HOT_RELOAD=/path/to/my_app/resources ./my_app`, in a build with the debugging aids (below).

The environment variables of the debugging aids (`BRLS_HOT_RELOAD`, `BRLS_INSPECTOR`, `BRLS_REDRAW_DEBUG`, `BRLS_PROFILE` and Cadran's `BRLS_` flags) are read only in a build made with `BRLS_DEV_TOOLS`. It is `AUTO` by default: on, except in `Release` and `MinSizeRel` builds, so an app given to its users does not read its screens from a folder an environment variable names. `-DBRLS_DEV_TOOLS=ON` keeps them in a release build (a simulator built in `Release`), `OFF` leaves them out of every build. The functions (`setHotReload`, `setInspectorEnabled`...) work in every build.

A theme file is watched when it is read by `Theme::loadFile()` after hot reload is on (with the environment variable, it is on from `init()`), with the files it `@include`s and `inherit`s (not the presets: they have no file). An `@res/…` theme is read from hot reload's resources folder when the file is there, as the XML files are (a name with `..` is not looked for there).

Each file watched is looked at about three times a second, and read again only when its time or size has changed; one larger than 4 MB, or that is not a regular file, is not read.

### Where the XML files are read from

- **A build that reads its resources from disk** (without `AURORE_LIBROMFS`) reads them from the `resources/` folder copied next to the app when it was built. Edit the files there, or better, give hot reload the sources' own folder (`BRLS_HOT_RELOAD=/path/to/my_app/resources`): the XML files are then read from it.
- **A build with its resources built in** (`AURORE_LIBROMFS`, the default on the consoles and phones) has no files to watch: give hot reload the sources' resources folder, and the XML files found there are read from it in place of the ones built in (`@res/…` in `brls:View`, `createFromXMLResource()`, `inflateFromXMLRes()`). The images, fonts and translations still come from the built-in ones.
- A path given to `createFromXMLFile()` or `inflateFromXMLFile()` is watched as it is.

It does nothing on the Switch, the PS4, the PS Vita, Android and iOS: their resources are not read from a folder of the developer's machine.

## What happens on a save

The files are looked at about three times a second; a file seen changed is acted on once it has stopped changing (an editor writing it in several goes).

- **An XML file** used by the activity on top (read while it was made, or while it was on top: a tab made when it is opened counts) clears its cached document, and the activity is built again: its `createContentView()` makes a new content, `onContentAvailable()` runs again, and the tabs shown and the focus are put back where they were when the new tree has a view at the same place. A file used only by an activity below waits until that one is made again.
- **The theme file, or a file it includes or inherits**: the theme file in use is read again (`Theme::loadFile`), and the activity on top is built again for the metrics read when views are made. A colour or metric taken out of the file goes back to its value without the file.

`Application::rebuildActivity()` does the same on demand.

## Limits

- An XML file with a mistake (a tag not closed, an unknown attribute, a `brls:View` that includes the file it is in: elements nest at most 64 levels deep) is logged and shown in a notification (`file:line: message`), and the screen stays as it was, the focus where it was, until the next save. This holds for the files of the tabs shown too: the new content is kept only once it has made them.
- An activity given its view in its constructor (`new Activity(view)`) cannot be made again; one with `CONTENT_FROM_XML_RES` or its own `createContentView()` can.
- What the activity set up outside its content view stays as it was: a subscription made in `onContentAvailable()` is made a second time, unless the activity takes it back in its destructor or before. Views bound with `BRLS_BIND` in the activity are looked up again in the new content.
- A view's state not in its XML (a scroll position, a text typed) starts again.
