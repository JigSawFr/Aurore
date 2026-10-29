#!/bin/bash
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

# The README's screenshots (the website's gallery too) and tour.gif, drawn by
# the test harness without a window or a display (aurore-tour, tests/tour.cpp):
#   cmake --preset tests && cmake --build --preset tests
#   .github/readme/capture.sh
# Needs ffmpeg for the GIF. The pictures are the same at every run.
set -e
cd "$(dirname "$0")/../.."
OUT=.github/readme
TOUR=./build/tests/tests/aurore-tour
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# name, then aurore-tour's arguments: its options, then the scene
shot() {
    local name=$1; shift
    "$TOUR" -o "$TMP/$name" "$@" > /dev/null
    cp "$TMP/$name/${@: -1}.png" "$OUT/$name.png"
}

shot cadran readme-cadran
shot sheet readme-sheet
shot picker readme-picker
shot checklist readme-checklist
shot markdown readme-markdown
shot components readme-components
shot palettes readme-palettes
shot light --theme light readme-cadran
shot french --lang fr readme-cadran
# The demo's theme folders (resources/themes): pictures, logo, decorations
shot winter --theme @res/themes/winter/theme.theme readme-cadran
shot prism --theme @res/themes/prism/theme.theme readme-cadran
shot aurora --variant light --theme @res/themes/aurora/theme.theme readme-cadran
# The demo on each simulated console (the website's "On every screen")
for console in switch vita ps4 3ds phone; do
    shot "console-$console" "readme-console-$console"
done

# The tour: through the tabs, into a game and back, at 12 pictures a second
"$TOUR" -o "$TMP/tour" --film "$TMP/film" readme-tour > /dev/null
ffmpeg -y -loglevel error -framerate 12 -i "$TMP/film/%04d.png" \
    -vf "scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle" \
    "$OUT/tour.gif"
