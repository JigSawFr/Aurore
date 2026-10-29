# Themes

Every colour Aurore draws with, the core views' and Cadran's, is a **theme key** (`aurore/text`, `cadran/teal`…) with a value for the light variant and one for the dark variant. A **theme file** changes any of them, and the look changes at once: no restart, no screen built again. Aurore ships presets, an app lists the user's own files in a folder, and a theme can be three lines long:

```ini
name = Mint
inherit = dark
accent = #3ddc97
```

## The format

A theme file is text, `*.theme` by convention: `key = value` lines, `[sections]`, `#` comments (a `#` at the start of a line or after a space; `#3ddc97` right after `=` is a colour, and so is one after a comma, in a list of colours). A theme with pictures is a folder holding its file and its pictures (below, [Theme folders and pictures](#theme-folders-and-pictures)).

```ini
# Metadata: shown by a picker (Theme::listThemes)
name = Ocean
author = Aurore
version = 1
description = Deep blue, a sky-blue accent.
# The motif a picker shows it with: a Cadran icon's name
icon = wave

# What it builds on: light, dark, another file (next to this one, or
# "builtin:<id>" for a preset). Without it: dark.
inherit = dark

# For a file that styles one variant: the file of the other one (next to
# this one, or a preset's id), which a picker switches to when that other
# variant is asked for
counterpart = ocean-day.theme

# The palette: a few colours, every key derived from them
background = #0a1a2f
surface = #10263f
text = #eaf2fb
accent = #3fa7ff
warning = #ffb000
danger = #ff6b6b

# A picture behind every screen, in the theme's folder; how it fills the
# window, the veil over it, the surfaces' opacity over it (below)
background_image = night.jpg
background_fit = cover
background_dim = 0.35
surface_opacity = 1

# Another file's lines, here
@include shared.inc

[colors]            # any key, for every variant the file styles
aurore/sidebar/background = #0d1b2a

[light]             # for the light variant only (palette, keys, background)
accent = #1f5fc4
background_image = day.jpg

[dark]              # for the dark variant only
cadran/glow = rgba(63, 167, 255, 0.15)

[metrics]           # style metrics (Application::getStyle)
aurore/highlight/stroke_width = 6
```

- **Colours**: `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(r, g, b)`, `rgba(r, g, b, a)` (r, g and b from 0 to 255, a from 0 to 1; a value out of its range, `nan` or `inf` is not a colour).
- **Variants**: a file styles the variant it inherits (`inherit = light` or `dark`, dark by default), or both when it has `[light]` and `[dark]` sections; it then follows the system's mode (or the app's light/dark setting). The variant a file does not style stays as it is without a file.
- **`counterpart`**, for a file that styles one variant only: the file (or preset) with the other variant, the same look in light or dark. A picker asked for the variant this file lacks switches to it (Cadran's does, below); `--check-themes` reports a counterpart that cannot be read or lacks that variant.
- **Background** (`background_image`, `background_fit`, `background_dim`, `surface_opacity`): for every variant the file styles, or in `[light]`/`[dark]` for one; below.
- **Decorations** (`backdrop.*`, `decor.*`, `particles*`): the same way; [Decorations](#decorations).
- **`icon`**: the Cadran icon (`cadran/view/cadran_icon.hpp`) a picker shows the file with: Cadran's palette picker draws it in the file's accent on a swatch of its background and surface. The motifs drawn for palettes: `snowflake`, `heart`, `lantern`, `blossom`, `sun`, `wave`, `leaf`, `pumpkin`, `tree`, `firework`, `gem`, `dune`, `contrast`, `moon`, `bolt`, `sparkle`; any other icon of the list works too. Inherited, and an `@include`d file's counts, as `background_image`. Without one (or with a name Cadran does not know), `sparkle`.
- **Logos and images** (`logo`, `icon.<name>`, `image.<slot>`, `preview`): pictures in the theme's folder for the app's logo, Cadran's icons, the app's image slots and the theme's thumbnail in a picker, for every variant or in `[light]`/`[dark]`; below, [Logos and images](#logos-and-images).
- **`inherit`** with a file or a preset takes everything from it first (its colours, metrics, variants, background), then this file's lines over them; for the background, a line of this file for every variant also beats the inherited file's `[light]`/`[dark]` one. `@include other.inc` reads another file's lines at that place (its `name` and `inherit` are ignored). Both look next to the including file first, then in the presets (`ocean`, or `builtin:ocean` for the preset only). The file they name, and a `counterpart`, is confined to the theme's folder as its pictures are ([below](#theme-folders-and-pictures)): no absolute path, no `..` out of the folder, no link out of it, `@res/` for a resource theme only; a refused one is an error (`line 3: @include /etc/passwd: an absolute path is not allowed: a theme's files are in its folder`), its file is not read. Limits, each an error with its file and line: a file that includes or inherits itself, directly or not (`parts/sky.inc:2: @include sky.inc includes itself (a loop)`), more than 8 levels, more than 64 files read for one theme (with what it includes and inherits); a file larger than 256 KB, or that is not a regular file (a folder, a pipe, a device), cannot be read.
- **Later lines win**; palette values are derived first, then `[colors]`, then `[light]`/`[dark]`, so an explicit key always beats the derived one.
- **Errors never stop the file**: a line that cannot be read (a bad colour, an unknown section, a missing include) is skipped and reported, with its file and line number. An unknown key is kept (an app's own keys are fine) and reported as a warning, in case it is a typo. The first 100 errors are kept, then `and N more errors`; an error quoting a long line is cut at 200 bytes, and a control character of the file is written `\x1B` in errors, warnings and the log.
- **Contrast**: after a file is read, the pairs the core and Cadran promise to keep readable are checked (`Theme::checkContrast`): `aurore/text on aurore/background reads badly: 3.1:1, under 4.5:1`. Text needs 4.5:1 (the inks, a cell's value, secondary text, the toasts), shapes that carry meaning 3:1: the focus ring on the screen and on a card, a tile's icon, a chart's bars (`cadran/bar`, `cadran/bar_today`) (WCAG 1.4.3, 1.4.11). A theme that sets `cadran/text3` over a picture sets `aurore/text_secondary` with it. Warnings are logged, and listed in `ThemeInfo::warnings`.

### The palette

The palette keys (`accent`, `background`, `surface`, `text`, `warning`, `danger`) set every key that follows from them; the ones a file does not give keep the variant's own colours. Inks used as text are darkened or lightened until they read at 4.5:1 on the background and both surfaces.

| Palette key | Keys it sets |
| --- | --- |
| `background` | `aurore/background`, `aurore/clear`, `aurore/sidebar/background` |
| `surface` | `cadran/surf`, `cadran/surf2` (5 % toward the text), `aurore/highlight/background`, `aurore/button/default_*_background` |
| `text` (and `background`) | `aurore/text`, `cadran/text`, `cadran/text2` and `text3` (30 % and 45 % toward the background, at 4.5:1), `aurore/text_disabled`, `aurore/text_secondary` (as `text3`), `aurore/header/subtitle`, `aurore/header/rectangle`, `aurore/list/listItem_value_color`, every separator and line (`aurore/sidebar/separator`, `aurore/header/border`, `aurore/applet_frame/separator`, `cadran/line`, `cadran/track`, `cadran/border`, `cadran/well`), button text and borders, `aurore/slider/line_empty`, `aurore/spinner/bar_color`, `cadran/key_bg`, `cadran/key_fg` |
| `accent` | `aurore/accent` and the inks of the active and chosen things (`aurore/sidebar/active_item`, `aurore/button/highlight_*_text`, `cadran/teal`, `cadran/pill_ok_ink`), the fills (`aurore/button/primary_*`, `aurore/slider/line_filled`, `cadran/teal_fill`, `cadran/teal_soft`, `cadran/pill_ok`, `cadran/rail_active`), the focus ring (`aurore/highlight/color1` at 3:1, `color2` lighter, `cadran/hl1`, `cadran/hl2`), `aurore/click_pulse`, `cadran/glow`, `cadran/glow_strong`, and the state shades below |
| `warning` | `cadran/amber`, `cadran/amber_fill`, `cadran/amber_surf`, `cadran/amber_line` |
| `danger` | `cadran/coral`, `cadran/coral_fill`, `cadran/coral_line`, `cadran/coral_soft` |

The label on the fills (`cadran/navy`: a confirmation's, a primary button's) is the one of the text, the background, black or white that reads best on the accent and danger fills; a fill the file does not give is darkened or lightened until it reads at 4.5:1.

**State shades**: `aurore/accent/hover`, `aurore/accent/pressed`, `aurore/accent/disabled` and `aurore/accent/ink` (the text on the accent) exist in every theme, derived from `aurore/accent` (from `accent` in a file), for an app's state styles: `pressed:backgroundColor="@theme/aurore/accent/pressed"` (docs/styles.md).

<a id="theme-folders-and-pictures"></a>
## Theme folders and pictures

A theme stays a single `.theme` file, or becomes a folder when it has pictures: the folder is named after the theme (its id), its file is `theme.theme`, and its pictures are next to it or in subfolders.

```
themes/                      # the folder an app lists (Theme::listThemes)
├── mint.theme               # a single file: id "mint"
└── aurora/                  # a theme folder: id "aurora"
    ├── theme.theme
    ├── night.jpg
    ├── day.jpg
    └── parts/sky.inc        # an @include, not listed
```

`Theme::listThemes(folder)` lists the folder's `*.theme` files and its subfolders that have a `theme.theme` (a subfolder without one is skipped); `Theme::loadFile("themes/aurora/theme.theme")` uses one.

**Pictures stay in the theme's folder.** A picture a theme names (`background_image`, and any picture a later key names) is read from the folder of the file that names it, and must stay in the theme's folder: the theme folder, or the folder of a single `.theme` file. An absolute path (`/…`, `sdmc:/…`, `C:/…`), a `..` going out of the folder, a backslash, or a link in the folder to a file out of it (or to a folder out of it, for a file not there yet; a link whose end cannot be told is refused too) is refused: the line is skipped and reported in `ThemeInfo::errors` (`line 7: background_image ../x.png: goes out of the theme's folder`), never a crash. An included file in a subfolder of the theme keeps the theme's folder (`parts/sky.inc` may name `../night.jpg`); an inherited theme's pictures are confined to its own folder. A preset's pictures are the app's resources (`@res/…`), and so may be those of a theme that is itself a resource (`@res/themes/…`); no other theme may name `@res/`. One function does this, `Theme::resolveAsset`, for every picture of a theme, and for the files it includes and inherits. Where it cannot help: a hard link is a file like any other (only who can write to the folder can make one), and a picture is checked when the theme is read and again just before it is decoded (`Theme::recheckAsset`), so a link put in its place in between is refused, but not one put there in the instant between that second look and the read; the consoles' cards (Switch, PS Vita, PS4) have no links, and are not checked for them. A picture that is not there is an error of `inspect()` and `listThemes()` (`background_image: cannot read …`); one that is there but cannot be decoded is an error only of an explicit check (`inspect(path, true)`, `listThemes(folder, true)`, `--check-themes`), since a listing decodes nothing. Used anyway, it is logged once and nothing is drawn.

### The background

| Key | Values | Default | What |
| --- | --- | --- | --- |
| `background_image` | a file in the theme's folder, or `none` | none | A picture behind every screen: PNG or JPEG (not WebP). `none` takes away an inherited one |
| `background_fit` | `cover`, `contain`, `tile`, `center` | `cover` | `cover` fills the window, centred, cut at the edges; `contain` shows it whole, centred, `aurore/clear` around it; `tile` repeats it from the top left; `center` puts it in the middle as is. `tile` and `center` draw one pixel of the picture per unit of the 1280×720 layout |
| `background_dim` | 0 to 1 | 0.35 | A veil of `aurore/background` over the picture, so that the text drawn on it reads: 0 shows it as is, 1 hides it |
| `surface_opacity` | 0 to 1 | 1 | The opacity of the surfaces over the picture: cards and chips (`cadran/surf`, `cadran/surf2`, and the tinted ones: `cadran/amber_surf`, `cadran/teal_soft`, `cadran/coral_soft`, `cadran/pill_ok`), the sidebar (`aurore/sidebar/background`), the focus's fill (`aurore/highlight/background`), the default buttons. 1 keeps them opaque; without a picture it does nothing |

Each key is read at the top (every variant the file styles) or in `[light]`/`[dark]` (one variant), as the colours: a theme can have a night picture and a day one, or a picture in one variant only. A variant the file does not style has no picture.

What shows and what does not:

- The screens have no fill of their own: their background is the window's, `aurore/clear`, then the picture, then its veil. The main screen, a tab frame's content, applet frames and Cadran's screens show it as they are.
- The surfaces keep their colours, at `surface_opacity`. Cadran's rail, when it opens over the tab in a translucent colour, draws the picture again under itself (`Application::drawThemeBackground`), so that the tab does not show through it.
- Dialogs keep their own surfaces, over `aurore/backdrop`: the core's panel is `aurore/background`, opaque; Cadran's dialogs, sheets and pickers are drawn in `cadran/surf`/`surf2`, at `surface_opacity`. Under 1, such a panel draws the picture again under itself, dimmed by `aurore/backdrop` as around it (`View::setThemeArea(ThemeArea::DIALOG)`), so it shows the picture faintly, not the screen behind it.
- `aurore/background` stays opaque: views use it to cut a shape out of what is under them (a badge's ring, the fade at the bottom of a list).

Why 0.35: over a picture as it comes (a photo, a gradient with bright parts), text keyed to the theme's own background colour needs that colour back under it; 0.35 keeps two thirds of the picture's colour, enough to see it, and pulls its brightest and darkest parts toward the background, which in most pictures brings the text's contrast back over its minimum. A busy or bright picture wants more (0.5 to 0.7); a dark, quiet one under a dark theme reads at 0.

**Contrast**: the picture's average colour is worked out once, where it is decoded, then the veil over it: that is what the text on the screens is checked on (`aurore/text on aurore/background over the background image reads badly: 3.2:1, under 4.5:1`), and the translucent surfaces over it. The palette derives its inks to read on `background`; over a picture darker or lighter than that, give the keys the check names (the demo's Aurora does, in `[light]`). The average does not see a bright spot in a dark picture: look at the screens too.

When the check runs: a picture is decoded off the UI thread when a theme is used (its colour recorded then, the check logged once it is loaded, `Theme::checkBackgroundContrast`), or by an explicit check on the thread that asks (`inspect(path, true)`, `listThemes(folder, true)`, `aurore_demo --check-themes`, which a picker must not call from the UI thread). A plain `listThemes()`, the one a picker calls, decodes nothing and reads colours only: its `ThemeInfo::warnings` have the picture's check only for a picture whose colour is already known (loaded, or checked, in this run).

**Live**: a theme with a picture is used like any other, at once, without building the screens again; going back to a theme without one (or to none) gives every colour and every pixel back. A picture still loading leaves the one before on screen until it is ready.

A complete theme folder, the demo's Aurora (`resources/themes/aurora`, its pictures drawn by `tools/make_aurora_background.py`):

```ini
# resources/themes/aurora/theme.theme
name = Aurora
author = Aurore demo
version = 1.0
description = Northern lights behind every screen, at night or at dawn.
icon = sparkle

background_fit = cover
surface_opacity = 0.85

[light]
background = #eef2fa
surface = #ffffff
text = #1b2433
accent = #15604f
background_image = day.jpg
background_dim = 0.5
cadran/text3 = #56606e
aurore/header/subtitle = #56606e
aurore/highlight/color2 = #2a8f7a

[dark]
background = #0a1426
surface = #13233a
text = #eaf4f4
accent = #3ee8a5
background_image = night.jpg
```

To see it: `aurore_demo -t @res/themes/aurora/theme.theme` (`AURORE_THEME=LIGHT` for the dawn), or the demo's Themes tab.

### Pictures in memory

A theme's pictures are loaded by `ThemeImages` (`aurore/core/theme_image.hpp`), the one loader for them: decoded on a thread of their own (`Threading::asyncDecode`, so a large one does not hold back the app's `aurore::async` tasks), made smaller when larger than 1920×1080 (the ratio kept, an average of the pixels each one covers), the texture made on the UI thread (`aurore::sync`), and freed when nothing holds it any more: the background's when the theme or the variant changes. Only the variant on screen has its picture loaded: a 1920×1080 picture is 8 MB of texture (RGBA), so at most 8 MB for the background, plus the picture being replaced while the next one loads. A JPEG that size takes a few tens of milliseconds to decode on a desktop, a few hundred on a Switch, on that thread. A picture larger than 4096×4096 pixels, or than 16384 on a side, is refused from its header without being decoded (`NVG_MAX_IMAGE_PIXELS`, `NVG_MAX_IMAGE_SIDE` in `nanovg.h`, the same for `aurore::Image`): the theme reports it as a picture it cannot read.


<a id="logos-and-images"></a>
## Logos and images

Besides its background, a theme folder can bring pictures of its own: the app's logo, some of Cadran's icons, the pictures an app lets themes replace (its image slots), and a thumbnail of itself for a picker. Each key names a file in the theme's folder, confined like `background_image` (above: no absolute path, no `..` out of the folder, no link out of it), PNG or JPEG.

| Key | Value | What |
| --- | --- | --- |
| `logo` | a file, or `none` | The app's logo wherever Cadran draws its mark (`CadranIcon` "mark": the header, the about screen), instead of the app's own drawing (`ui::hooks`' `mark`) or Aurore's. Fitted in the mark's box, its ratio kept, centred, drawn as it is |
| `icon.<name>` | a file, or `none` | Cadran's icon `<name>` (`settings`, `info`, `gamepad`…: the names of `cadran/view/cadran_icon.hpp`), wherever it is drawn (`CadranIcon`, `CadranIcon::glyph`: the rail, the rows and sheets, the pills, the palettes' motifs; not the header's language, theme and text size buttons, which are drawn otherwise). Fitted in the icon's box, its ratio kept |
| `icon.<name>.tint` | `true` or `false` | `true`: the picture is a mask drawn in the icon's colour (below); `false` (the default): drawn as it is |
| `image.<slot>` | a file, or `none` | The picture of an image slot the app declares (`playguard/loading/art`, below) |
| `preview` | a file, or `none` | The theme's thumbnail in a picker: Cadran's palette picker draws it in the row's square, instead of the swatch of its colours |

Each is read at the top (every variant the file styles) or in `[light]`/`[dark]` (one variant): a dark logo and a light one, an icon replaced in one variant only. Inherited and included as the background is: a file's own line for every variant beats the inherited file's `[light]`/`[dark]` one, and `none` takes away an inherited picture. A variant the file does not style has none of them. Without these keys nothing changes, pixel for pixel.

**The tint**: a monochrome icon follows the theme as Cadran's drawings do when it is a mask: `icon.settings.tint = true` draws the picture in the icon's colour of the moment (the rail's ink, the focus's colour on the focused row, the accent of a checked line, a disabled grey), keeping only its shape: each pixel's alpha, its colour ignored. A picture with no transparent pixel at all (a black drawing on white, a JPEG) is read by its darkness instead: black is the ink, white is nothing. Without the tint the picture is drawn in its own colours, at the opacity of the icon's colour (a faded icon fades), the same in every state: for a coloured icon, or one that must stay as it is. Draw tinted icons white (or black) on transparent, the shape filling its 24-unit box as Cadran's do, with no margin of its own beyond theirs.

**What happens when**: the pictures are decoded on the decode thread (`ThemeImages`) when something first draws them, the built-in logo or icon drawn until they are ready (a frame or two). A file that is not there is an error of `inspect()` and `listThemes()` (`icon.settings: cannot read …`; a picker leaves the theme out); one that cannot be decoded is an error of an explicit check only (`inspect(path, true)`, `--check-themes`). Used anyway, it is logged once and the built-in logo, icon or slot picture is drawn. An `icon.<name>` that no icon has (`icon.cog`), or an `image.<slot>` the app does not declare, is a warning (`image.myapp/art: the app has no such image slot (kept); its slots: …`), kept in case another version of the app has it.

**Sizes and memory**: a logo is made at most 256×256 (`ThemeImages::LOGO_SIZE`), an icon and a preview 128×128 (`ICON_SIZE`, `PREVIEW_SIZE`), at load, the ratio kept: a larger file costs its decoding time only. An image slot's pictures are made at most what the app declared for it (`maxSide`), else 1920×1080. Only the variant on screen has its pictures loaded, and only those drawn; they are let go of when the theme or the variant changes (a picture still drawn by a view, until it takes the new one), the picker's previews when it closes. A logo and a dozen icons are about 1 MB of textures.

### Image slots: pictures an app lets themes replace

An app declares, once at start-up and before it loads the user's theme, the pictures a theme may replace, with its own as the fallback:

```cpp
// PlayGuard: the loading screen's art; PlayShelf: the placeholder of a game
// without a cover, made at most 512 px
aurore::Theme::addImageSlot("playguard/loading/art", "@res/img/loading_art.png");
aurore::Theme::addImageSlot("playshelf/cover/placeholder", "@res/img/no_cover.png", 512);
```

A theme then replaces it:

```ini
image.playguard/loading/art = loading.png
[light]
image.playguard/loading/art = loading-day.png
```

and the app shows whichever is in use, live:

```xml
<!-- Follows the theme: the theme's picture, else the app's -->
<aurore:Image themeImage="playshelf/cover/placeholder" scalingType="fit"/>
```

```cpp
image->setThemeImage("playguard/loading/art");   // the same, in code

// Drawn by the app's own view: the picture now (the theme's for the
// variant on screen, else the fallback; nullptr for neither), maybe still
// loading
std::shared_ptr<aurore::ThemeImage> art = aurore::Theme::image("playguard/loading/art");
if (art && art->ready())
    art->draw(vg, x, y, width, height, aurore::BackgroundFit::COVER);
// ...asked again when the theme changes, and drawn again once it is loaded
aurore::Application::getThemeChangedEvent()->subscribe([](aurore::ThemeVariant) { /* Theme::image(key) again */ });
aurore::ThemeImages::getLoadedEvent()->subscribe([](const aurore::ThemeImage* image) { /* redraw */ });
```

Slot keys are the app's: prefix them with its name (`myapp/…`). A slot without a fallback (`addImageSlot("myapp/banner")`) shows nothing unless a theme gives it a picture: `Theme::image` is `nullptr`, `setThemeImage` clears the image. `aurore::Image::setThemeImage` keeps the picture it shows until the next one is loaded, takes the fallback back when the theme's cannot be read or the theme goes, and stops following the slot when the image is set another way (`setImageFromFile`, `clear`). `Theme::getImageSlots()` lists them, for a theme author's documentation. Keep a transparent edge around a slot's picture that a `aurore::Image` draws larger than it is: the picture's edge pixels are stretched over the rest of the view (borealis' images do this).

Apps that draw their logo themselves (a loading screen of their own, a splash) ask for the theme's the same way: `aurore::Theme::logo()` (`nullptr` without one), drawn with `logo->draw(vg, x, y, size, size, aurore::BackgroundFit::CONTAIN)` when `ready()`, their own otherwise. `aurore::Theme::icon(name, &tint)` is an icon's picture, for a view that draws Cadran's icons its own way, and `aurore::Theme::getPictures(variant)` the paths of all of them.

### The preview in a picker

`preview` is shown by Cadran's palette picker in the theme's row (`ui::pick_palette`), in the swatch's rounded square, filling it (cut at its edges, centred), with its hairline. Listing the themes decodes nothing: the picker asks for each preview when it opens, they are decoded on the decode thread, and each row shows the swatch of its colours until its preview is ready. A square picture of a screen of the theme, or its emblem on its background, at least 72×72 (the square is 36 units wide, 54 pixels on a 1080p screen), made 128×128 at most when loaded. `ThemeInfo::lightPictures`/`darkPictures` (`pictures(variant)`) have it for an app's own picker: `aurore::ThemeImages::load(info.pictures(variant).preview, options)` with `options.maxSide = aurore::ThemeImages::PREVIEW_SIZE`.

### The demo's Prism

A theme folder with a logo per variant, three icons (two tinted masks, a coloured one), a preview, and the picture of the demo's image slot (`demo/about/art`, the About tab's picture), all drawn by `tools/make_prism_pictures.py`:

```ini
# resources/themes/prism/theme.theme
name = Prism
author = Aurore demo
version = 1.0
description = Light split into its colours: a logo, icons and a picture of its own.
icon = gem

preview = preview.png
icon.settings = icons/settings.png
icon.settings.tint = true
icon.info = icons/info.png
icon.info.tint = true
icon.gamepad = icons/gamepad.png
image.demo/about/art = about.png

[light]
background = #f6f4fb
surface = #ffffff
text = #221d33
accent = #6a3fd1
logo = logo-light.png

[dark]
background = #14111f
surface = #1e1a2e
text = #f1eefa
accent = #b48cff
logo = logo-dark.png
```

The demo declares its slot in `demo/src/demo_app.cpp` (`Theme::addImageSlot("demo/about/art", "@res/img/aurore_256.png", 512)`), and the picture of its About tab follows it (`image->setThemeImage("demo/about/art")`, `demo/src/tab/about_tab.cpp`; `themeImage="demo/about/art"` in XML). To see it: `aurore_demo -t @res/themes/prism/theme.theme`, or the palette picker.

<a id="decorations"></a>
## Decorations

Over the palette and the background, a theme can decorate the screens: a picture over the surface of the header, the sidebar or the dialogs (a **backdrop**), a strip along the top or the bottom edge of the screen (a garland, icicles, a snowdrift), and **particles** (snow, leaves, petals...) drifting behind the screens. Without these keys nothing is drawn and nothing changes: an app's screens stay pixel for pixel what they were.

```ini
# resources/themes/winter/theme.theme (the demo's)
inherit = builtin:winter
decor.top_fit = tile
decor.bottom_fit = tile
backdrop.sidebar_fit = tile
backdrop.dialog_fit = tile
backdrop.dialog_opacity = 0.6
particles = snow
particles_density = medium

[light]
decor.top = icicles-day.png
decor.bottom = drift-day.png
backdrop.sidebar = frost-day.png
backdrop.dialog = frost-day.png

[dark]
decor.top = icicles-night.png
decor.bottom = drift-night.png
backdrop.sidebar = frost-night.png
backdrop.dialog = frost-night.png
```

To see it: `aurore_demo -t @res/themes/winter/theme.theme` (`AURORE_THEME=LIGHT` for the day). Its pictures are drawn by `tools/make_winter_decor.py` (a few KB each).

### Backdrops and strips

| Key | Values | Default | What |
| --- | --- | --- | --- |
| `backdrop.header` | a file in the theme's folder, or `none` | none | Over a screen's header, its whole width (an `AppletFrame`'s, not a dialog's), under its title |
| `backdrop.sidebar` | the same | none | Over the sidebar (Cadran's rail), under its items |
| `backdrop.dialog` | the same | none | Over the panel of dialogs, dropdowns and Cadran's sheets and pickers, in their round corners, under their content |
| `decor.top`, `decor.bottom` | the same | none | A strip along the top (bottom) edge of the screen, the screen's width, behind the screens: over the background and its picture, under every view |
| `<key>_fit` | `cover`, `contain`, `tile`, `center` | `cover` | As `background_fit`, in the area. A strip's `cover` is the picture at the screen's width (its height from its ratio); its `tile` repeats it sideways, at its own height or `_height` |
| `<key>_opacity` | 0 to 1 | 1 | The picture's opacity |
| `decor.top_height`, `decor.bottom_height` | 1 to 720 (layout units) | the picture's | The strip's height |

Each key is read at the top or in `[light]`/`[dark]`, inherited and taken away (`none`) like `background_image`; the pictures are confined to the theme's folder the same way (`Theme::resolveAsset`), reported in `ThemeInfo::errors` when they are refused or missing (`backdrop.header: cannot read …`), loaded by `ThemeImages`, and only the variant on screen's are in memory. A picture still loading leaves the one before.

What to draw: a backdrop is under text, so keep it quiet (a texture, a faint pattern, a gradient) or lower its opacity; the screen's text is not checked on it. A strip is behind the screens: the header's title and the footer's hints are drawn over it, so a top strip stays within the header's first 30 units or so (icicles, a garland), a bottom strip under about 20 (the footer's text is above that), or stays faint. A header backdrop covers the top strip: use one or the other. PNG with transparency is the usual form of a strip.

Which views: the core's AppletFrame header, `Sidebar`, `Dialog`, `Dropdown` and `EditTextDialog`, and Cadran's sheets; an app's own panel opts in with `view->setThemeArea(aurore::ThemeArea::DIALOG)` (or `HEADER`, `SIDEBAR`).

### Particles

| Key | Values | Default | What |
| --- | --- | --- | --- |
| `particles` | `none`, `snow`, `leaves`, `petals`, `embers`, `confetti`, `sparkles`, `hearts`, `bubbles` | `none` | What drifts: snowflakes falling, leaves and cherry petals tumbling down, embers rising and dying out, confetti fluttering down, four-pointed sparkles twinkling, hearts and bubbles rising |
| `particles_density` | `low`, `medium`, `high`, or 0 to 1 | `low` (0.3) | How many, of 60 at most (`ThemeDecorations::MAX_PARTICLES`); each kind takes its share (a leaf is larger than a flake). `medium` is 0.6, `high` 1 |
| `particles_color` | up to 5 colours, comma-separated (`#fff, rgba(200, 220, 255, 0.8)`), or `auto` | from the palette | Each particle takes one. From the palette: snow and bubbles a pale ink on a dark background, the accent's tint on a light one; leaves the accent, amber and coral; petals the accent and a lighter one; embers amber and the accent; confetti the accent, pink, blue, amber, violet; sparkles the accent and a pale ink (amber on a light background); hearts the accent and pink |
| `particles_speed` | 0.25 to 4 | 1 | Faster or slower, sway and spin included |
| `particles_layer` | `back`, `front` | `back` | `back`: behind the screens (over the background, its picture and the strips, under every view), so they show between the cards and never cross a text or the focus. `front`: over the screens (under the toasts), at 55 % of their opacity, for a theme that wants them seen everywhere; they then cross text |

Read like the others: at the top or per variant, inherited. They are drawn with nanovg in three depths (the far ones smaller, slower, fainter), a few fills per frame (one per depth and colour; a gradient each for the glow of embers and of the nearest flakes): no picture, no allocation once made.

**Motion, the battery and the user** (`ThemeDecorations`, `aurore/core/theme_decor.hpp`):

- They move only when motion is allowed: `Application::getReduceMotion()` false, so not with the app's reduced motion, nor, with Cadran, with Preferences › Visual effects at *Reduced*, nor *Automatic* in hand (the lighter effects), nor with `AURORE_STILL_FOCUS` (screenshot tests). Otherwise **nothing** of them is drawn, not even a frozen frame: the plain palette.
- They move at 30 frames a second at most (`PARTICLE_FPS`): their places are a function of the time, taken at a 30 Hz tick. With redraw on demand, an otherwise still screen is drawn 30 times a second while they run, instead of once a second.
- After 60 seconds without input (`IDLE_SECONDS`: a button, a key, a touch, the mouse) they fade out over 3 seconds and stop: no frame is asked for any more. The next input brings them back, fading in. A theme being used counts as an input.
- They are not views: they never take the focus, an input or a touch, and the layout does not know them.
- What they cost (measured on the desktop, the demo's main screen, Mesa's software renderer, every frame drawn): about 0.1 to 0.3 ms more of nanovg per frame to build their paths and 0.5 to 1.3 ms more to rasterise them (`low` to `high` snow), the whole process 6 to 9 % more CPU. On a GPU the rasterising is close to free; the paths stay.

### The presets' particles

The seasons and feasts drift gently (`low`), each in its own colours, under the rules above: a user who reduces motion, or a console in hand on *Automatic*, gets the plain palette.

| Preset | Particles |
| --- | --- |
| `winter`, `christmas` | `snow` |
| `spring` | `petals` |
| `autumn` | `leaves` |
| `halloween` | `embers` |
| `new-year` | `confetti` |
| `valentine` | `hearts` |
| `lunar-new-year` | `sparkles`, in gold |
| `summer` | none |

A file built on one keeps them (`inherit = builtin:winter`), or says `particles = none`.

### The API

```cpp
aurore::Theme::getDecor(aurore::ThemeVariant::DARK);   // pictures by area, particles
info.decor(variant);                                // the same, from ThemeInfo (lightDecor, darkDecor)
view->setThemeArea(aurore::ThemeArea::DIALOG);        // an app's panel takes backdrop.dialog
aurore::ThemeDecorations::particlesRunning();         // shown and moving now
aurore::ThemeDecorations::hasPicture(aurore::ThemeArea::TOP);
aurore::Application::getLastInputTime();              // what the particles' pause counts from
```

## Every key

Without a theme file, each variant has the core's defaults, then Cadran's palettes over them (`cadran/ui/cadran_palette.hpp`, the single place they are defined).

**Core** (`library/lib/core/theme.cpp`):

| Key | What |
| --- | --- |
| `aurore/clear` | The window's colour, under everything |
| `aurore/background` | Screens, dialogs and pickers |
| `aurore/text`, `aurore/text_disabled` | Labels, and what cannot be used now |
| `aurore/text_secondary` | Secondary text at 4.5:1: a switch cell's Off, a placeholder, a counter (Cadran: `cadran/text3`) |
| `aurore/accent`, `aurore/accent/hover`, `…/pressed`, `…/disabled`, `…/ink` | The accent (a dialog's buttons) and its state shades |
| `aurore/backdrop` | Behind a dialog or a picker |
| `aurore/notification/background`, `aurore/notification/text` | Toasts |
| `aurore/click_pulse` | A press's flash |
| `aurore/highlight/background`, `aurore/highlight/color1`, `aurore/highlight/color2`, `aurore/highlight/shadow` | The focus: its fill, its ring pulsing between the two colours, its shadow |
| `aurore/shadow` | A view's shadow (`shadowType`) |
| `aurore/applet_frame/separator` | The header's and footer's lines |
| `aurore/sidebar/background`, `aurore/sidebar/active_item`, `aurore/sidebar/separator` | The sidebar |
| `aurore/header/border`, `aurore/header/rectangle`, `aurore/header/subtitle` | A list's header |
| `aurore/button/{primary,default}_{enabled,disabled}_{background,text}`, `aurore/button/highlight_{enabled,disabled}_text`, `aurore/button/{enabled,disabled}_border_color` | Buttons, by style and state |
| `aurore/list/listItem_value_color` | A cell's value |
| `aurore/slider/pointer_color`, `…/pointer_border_color`, `…/line_filled`, `…/line_empty` | Sliders |
| `aurore/spinner/bar_color` | The progress spinner |
| `aurore/battery/level`, `aurore/battery/charging` | The battery widget |

**Cadran** (`cadran/ui/cadran_palette.hpp`; it also sets the core keys above to its palettes):

| Key | What |
| --- | --- |
| `cadran/text`, `text2`, `text3` | Text, quieter text, the quietest (captions, notes) |
| `cadran/text_off` | What the app cannot do now (`ui::show_writable`) |
| `cadran/surf`, `surf2` | Cards, and the second surface (chips, code, icons' squares) |
| `cadran/line`, `cadran/border`, `cadran/track`, `cadran/well` | Hairlines, a button's border, a switch's track off, a bar's empty part |
| `cadran/teal`, `teal_fill`, `teal_soft` | The accent: as an ink, as a fill, faint |
| `cadran/amber`, `amber_fill`, `amber_surf`, `amber_line` | Warnings |
| `cadran/coral`, `coral_fill`, `coral_line`, `coral_soft` | Removals and failures |
| `cadran/blue`, `violet`, `pink` | A release note's alerts, a build's tags, the tiles' inks |
| `cadran/pill_ok`, `pill_ok_ink` | The header's pill |
| `cadran/rail_active`, `cadran/rail_shadow` | The rail's active pill, its shadow over the tab |
| `cadran/glow`, `glow_strong` | The screens' background light |
| `cadran/hl1`, `hl2` | The focus ring's two colours |
| `cadran/key_bg`, `key_fg` | A controller key's bubble |
| `cadran/navy` | The label on a fill |
| `cadran/knob`, `knob_line` | A switch's knob |
| `cadran/bar`, `bar_today` | A chart's bars |
| `cadran/avatar0` … `avatar3` | The accounts' avatars |
| `cadran/hold_fill` | A held button filling up |
| `cadran/brand/track`, `brand/fill`, `brand/glow` | The loading screen's bar, alike in both variants |
| `cadran/none` | Transparent |

Not colour keys, on purpose: the app's logo (`CadranIcon` "mark", the app's own drawing through `ui::hooks`' `mark`; a theme replaces it with a picture, `logo`, below), the debug layer and the layout wireframe (`-v`), and a `Rectangle`'s default colour before the app sets one.

**Metrics** (`[metrics]`): numbers from -10000 to 10000 (`nan`, `inf` and larger ones are errors), any of `library/lib/core/style.cpp` and Cadran's (`cadran/radius/*`, `aurore/tab_frame/sidebar_width`…). The focus highlight's look is there: `aurore/highlight/stroke_width`, `corner_radius`, `shadow_width`, `shadow_offset`, `shadow_feather`, `shadow_opacity`. Lengths are written at density 1 and follow the [density](styles.md#density-of-the-lengths). Metrics read when drawing (the highlight's) change at once; the ones read when a view is made (paddings, sizes) apply to the views made afterwards.

## Presets

Built into the library (`library/lib/core/theme_presets.cpp`), every app has them: `builtin:<id>`, listed by `Theme::builtinThemes()`, their names translated (`resources/i18n/*/themes.json`). Each passes the contrast check.

| Id | Variants | Motif (`icon`) | What |
| --- | --- | --- | --- |
| `high-contrast-dark` | dark | `contrast` | White on black, a yellow focus ring, a thicker ring; counterpart `high-contrast-light` |
| `high-contrast-light` | light | `contrast` | Black on white, a deep blue focus ring, a thicker ring; counterpart `high-contrast-dark` |
| `ocean` | dark | `wave` | Deep blue, a sky-blue accent |
| `amethyst` | light and dark | `gem` | Violet, following the system's mode |
| `sand` | light | `dune` | Warm paper, a terracotta accent |
| `winter`, `valentine`, `lunar-new-year`, `spring`, `summer`, `autumn`, `halloween`, `christmas`, `new-year` | light and dark | `snowflake`, `heart`, `lantern`, `blossom`, `sun`, `leaf`, `pumpkin`, `tree`, `firework` | The seasons and the feasts of the year, below |
| `seasonal` | light and dark | the one of the day's | The one of the day, by the calendar, below |

A preset is also a starting point: `inherit = builtin:ocean`, then the keys to change.

### The seasons

Each season has both variants and keeps the roles of every theme (the accent for what is chosen and the primary buttons, amber for warnings, red for removals), so that an app reads the same all year; the season is in the surfaces, the focus ring (it pulses between its two colours), the screens' glow, the loading bar, the charts' bars, the tiles' inks (`cadran/blue`, `violet`, `pink`) and the avatars. To see one: `aurore_demo -t builtin:halloween` (`AURORE_THEME=LIGHT` for the light variant).

| Id | Dark | Light |
| --- | --- | --- |
| `winter` | Frost and silver: a cold slate night (`#0f1620`), an ice-blue accent (`#9cc9ff`), the focus pulsing from ice to white | Frost (`#edf3f8`), a glacier-blue accent (`#2a6496`) |
| `valentine` | Wine (`#1f0a14`), a rose accent (`#ff5c8a`), the focus from rose to blush | Blush (`#fbeff2`), a crimson accent (`#c1121f`) |
| `lunar-new-year` | Red lacquer (`#1f0a0b`), a gold accent (`#f6c445`), the focus from gold to lantern red, a red glow; jade for the details | Rice paper (`#fbf2e4`), an imperial red accent (`#b5121b`), the focus from red to gold |
| `spring` | Cherry blossoms at night (`#1c1320`), a petal-pink accent (`#ff9cc4`) | Blush (`#fbf0f3`), a cherry accent (`#b8487a`), the focus from cherry to leaf green |
| `summer` | A warm night by the sea (`#0d2327`), a sunset-coral accent (`#ff8f5a`), the focus from coral to sun | Sun-bleached sand (`#fdf7ea`), a lagoon accent (`#00798c`), the focus from tangerine to lagoon |
| `autumn` | Dark bark (`#1a1310`), a maple accent (`#e8743b`), the focus from maple to amber | Linen (`#f6efe4`), a rust accent (`#a83f12`), the focus from rust to olive |
| `halloween` | A violet night (`#150d1f`), bone text (`#f3ead8`), a pumpkin accent (`#ff8a1f`), the focus flickering to candlelight with a warm shadow, a violet moonlight glow, slime green and violet inks | Moonlit lilac (`#f1edf6`), a pumpkin accent (`#c2520a`), the focus from violet to pumpkin |
| `christmas` | Pine (`#0d1f17`), snow text, a gold accent (`#e8c15a`), the focus twinkling from gold to cranberry like the tree's lights, a golden shadow; a cranberry loading bar | Snow (`#f4f6f1`), a cranberry accent (`#b3162f`), the focus from pine to gold |
| `new-year` | Midnight (`#0a0c18`), a champagne accent (`#e9c46a`), the focus sparkling from gold to silver | Silver (`#f3f2ef`), champagne-gold fills (`#c9a227`), the focus from gold to midnight blue |

#### Seasonal

`builtin:seasonal` is the preset of the day: a picker lists it like the others (*Seasonal*, Cadran's palette picker included), and an app that saves it gets Halloween at the end of October and Christmas in December without doing anything. It reads the local date, and the main loop looks again once a minute (`Theme::followCalendar()`): the day a season begins, the screens change at midnight, or as soon as the console wakes up. Every season has both variants, so the variant stays as it is. `Theme::seasonalTheme(time)` says which preset a date has (`"builtin:halloween"`), for a tile or a test. It inherits that preset, so its `ThemeInfo` (its `icon`, its previews) is the season's: its row in Cadran's palette picker shows a snowflake in January and a pumpkin at Halloween.

The days are in one table (`SEASONS`, `library/lib/core/theme_presets.cpp`), both ends included, the first range a date falls in winning:

| Preset | Days |
| --- | --- |
| `new-year` | 29 December to 3 January |
| `christmas` | 1 to 28 December |
| `valentine` | 10 to 15 February |
| `lunar-new-year` | The eve of the Lunar New Year to the Lantern Festival, 14 days after it (the dates are tabled from 2025 to 2036; a year out of the table has none) |
| `halloween` | 17 October to 2 November |
| `spring` | 20 March to 20 June |
| `summer` | 21 June to 22 September |
| `autumn` | 23 September to 30 November |
| `winter` | The rest: 4 January to 19 March, but for the feasts |

The seasons are the northern hemisphere's.

## The API

```cpp
#include <aurore.hpp>

// Use a file (or a preset), replacing the one in use; the screens take it at
// their next frame. `variant`: the one it styles (or the one in use when it
// styles both), for setThemeVariant.
aurore::ThemeVariant variant;
if (aurore::Theme::loadFile("sdmc:/config/myapp/themes/mint.theme", &variant))
    aurore::Application::getPlatform()->setThemeVariant(variant);
aurore::Theme::loadFile("builtin:high-contrast-dark");
aurore::Theme::reset();                       // back to the look without a file

// A picker: metadata, a preview, errors and warnings per file, never a crash
for (const aurore::ThemeInfo& info : aurore::Theme::listThemes("sdmc:/config/myapp/themes"))
    if (info.ok())
        addRow(info.name, info.author, info.dark ? info.darkPreview : info.lightPreview, info.path);
std::vector<aurore::ThemeInfo> presets = aurore::Theme::builtinThemes();

aurore::Theme::getCurrentPath();              // the file in use, "" without one
aurore::Theme::styles(aurore::ThemeVariant::LIGHT);   // whether it styles that variant
aurore::Theme::inspect(path);                 // one file, read without using it
aurore::Theme::checkContrast(variant);        // the pairs under their minimum
aurore::Theme::addContrastCheck("myapp/ink", "aurore/background", 4.5f);
```

`ThemeInfo` has `path`, `id` (a theme folder's name for its `theme.theme`), `name`, `author`, `version`, `description`, `icon` (its `icon`, "" without one), `light`/`dark` (the variants it styles, `styles(variant)`), `counterpart` (its path, as `loadFile` takes it), `lightPreview`/`darkPreview` (`background`, `surface`, `text`, `accent`, `highlight`), `lightBackground`/`darkBackground` (`ThemeBackground`: `image`, `fit`, `dim`, `surfaceOpacity`; `background(variant)`), `lightPictures`/`darkPictures` (`ThemePictures`: `logo`, `preview`, `icons`, `images`; `pictures(variant)`), `backgroundImage` (the dark variant's picture, else the light one's), `errors`, `warnings`, `ok()` and `variant()`. `listThemes` takes a folder or a resource folder (`@res/themes`), and lists its `*.theme` files and theme folders, cheaply (no picture decoded unless asked, above); a file's `@include`s are best named `*.inc`, so that they are not listed.

For what a theme's pictures need (the background, and what later keys draw):

```cpp
aurore::Theme::getBackground(aurore::ThemeVariant::DARK);   // image, fit, dim, surfaceOpacity
aurore::Theme::resolveAsset(name, directory, root, &path, &error); // a theme's picture, confined
aurore::Theme::addSurfaceKey("myapp/card");   // an app's surface, translucent over a picture
aurore::Theme::checkBackgroundContrast(variant); // the text on the picture, once it is loaded

// The loader: shared by path while held, freed with its last holder
std::shared_ptr<aurore::ThemeImage> image = aurore::ThemeImages::load(path);
aurore::ThemeImages::getLoadedEvent()->subscribe([](const aurore::ThemeImage* image) { /* ready or failed */ });
if (image->ready()) nvgImagePattern(vg, x, y, image->width, image->height, 0, image->texture, 1.0f);
aurore::ThemeImages::meanColor(path, &color);  // its average colour, remembered
aurore::ThemeImages::decoding();               // pictures on the decode thread (a test waits for 0)

aurore::Theme::addImageSlot("myapp/art", "@res/img/art.png");   // a picture themes may replace
aurore::Theme::image("myapp/art");             // the picture now: the theme's, else the app's
image->setThemeImage("myapp/art");           // a aurore::Image that follows it
aurore::Theme::logo();                         // the theme's logo, nullptr without one
aurore::Theme::icon("settings", &tint);        // an icon's picture, nullptr without one
art->draw(vg, x, y, width, height, aurore::BackgroundFit::CONTAIN); // a ThemeImage, fitted

aurore::Application::hasThemeBackground();    // a picture is behind the screens
aurore::Application::drawThemeBackground(vg, x, y, width, height); // under a translucent panel
```

### Colours that follow the theme

A view drawn from a theme key follows every change. What the core and Cadran draw does; an app's own views do the same in one of three ways:

- **XML**: `textColor="@theme/aurore/text"`, `backgroundColor="@theme/cadran/surf"` (any colour property of the view: docs/styles.md).
- **Code**: `label->setThemeColor("textColor", "cadran/text2")`, `box->setThemeColor("backgroundColor", "cadran/surf")`, instead of `setTextColor(theme["cadran/text2"])`, which keeps the colour of the moment. A colour set afterwards by value (`setTextColor(…)`) replaces the key; `setThemeColor(property, "")` forgets it.
- **Drawing**: in `draw()`, `ctx->theme["cadran/teal"]` is always the theme in use.

What keeps colours another way (a cached `NVGcolor`, an image per variant) refreshes them in `View::onThemeChanged()` (call the base; a `Box` goes on to its children), or listens to `Application::getThemeChangedEvent()`, fired at most once per iteration of the main loop when the variant, the file or a colour changed, after every view of the screens took the change.

## A theme picker in an app

With Cadran (PlayGuard, PlayShelf), everything is there:

1. Keep `ui::Prefs::theme_file` in the app's settings, next to `theme` ("" for the boards' palettes, `builtin:<id>`, or a path), and hand the prefs over with `ui::use_prefs()` as today.
2. Say where the user's own files are: `ui::hooks::get().themes_dir = [] { return std::string("sdmc:/config/myapp/themes"); };` (on the desktop, a folder next to the settings). The folder may not exist: the picker then offers the presets only.
3. That is all: `ui::pick_theme()` (the header's button, Preferences) ends with a **Palette** row opening `ui::pick_palette()`: Default, the presets, then the folder's readable files (a broken one is logged and left out), each row with a swatch of its colours (its background, its surface as a ring, its `icon` in its accent; in the variant on screen when the palette styles it, else in the one it styles; Default's, the app's own colours and a palette), or the theme's `preview` once loaded. `ui::palette_name()` is the choice saved, for a tile. The choice is saved through `save_prefs` and applied at once (`live_prefs::apply_theme()`).

A palette that styles one variant only decides the variant, and the header's button always shows the variant on screen:

- Picking such a palette switches to its variant (`ui::Prefs::theme` becomes `light` or `dark`, whatever it was).
- Picking the other variant (Dark with *High contrast light*) switches the palette too: to the file's `counterpart` when it has that variant (*High contrast dark*), else back to Default, the boards' palettes, which have both. The theme picker says so beforehand: such a row shows the palette it will switch to.
- System keeps the palette, shown in its own variant whatever the system's mode.
- Preferences saved before this rule (Dark with a light-only palette) show the palette's variant, and `theme` is corrected when they are applied.

Without Cadran's pickers, the same in a few lines: list `Theme::builtinThemes()` and `Theme::listThemes(folder)` (the demo's Themes tab does it, `demo/src/tab/themes_tab.cpp`, a tile per theme with a swatch of its colours), call `Theme::loadFile(info.path, &variant)` and `setThemeVariant(variant)` on a click, save `info.path`, and load it again at start-up (after the app's own `addColor` calls).

Users write their files in the folder, starting from a preset (`inherit = builtin:ocean`) or from nothing; the demo's examples are in `resources/themes` (Mint, Rose, Boréale's night, and the folders Aurora, Prism and Winter). To check files without opening a screen: `aurore_demo --check-themes <folder>` prints each preset's and each file's errors and warnings and exits with 1 when there are some.
