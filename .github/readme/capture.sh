#!/bin/bash
# The README's screenshots and tour.gif, from the demo's desktop build:
#   cmake -B build_pc -DAURORE_PLATFORM=DESKTOP && cmake --build build_pc
#   xvfb-run -a -s "-screen 0 1280x720x24" .github/readme/capture.sh
# Needs Xvfb, xdotool, ImageMagick, ffmpeg and dbus. The demo starts on its
# Cadran tab, in the dark theme; light.png with BOREALIS_THEME=LIGHT.
set -e
cd "$(dirname "$0")/../.."
OUT=.github/readme
APP=./build_pc/aurore_demo

start() {
    dbus-run-session -- $APP $APP_ARGS > /dev/null 2>&1 &
    sleep 6
}
stop() { pkill -x aurore_demo || true; sleep 1; }
key() { xdotool key "$1"; sleep "${2:-1}"; }

# name, then the keys from the first tab
shot() {
    local name=$1; shift
    start
    for k in "$@"; do key "$k"; done
    sleep 2.5
    import -window root "$OUT/$name.png"
    stop
}

shot cadran
shot sheet Right Down Right Right Return
shot picker Right Down Right Right Right Return
shot checklist Right Down Right Right Right Right Return
shot markdown Right Down Down Down Down Down Down Down Down
shot components Down Right
export BOREALIS_THEME=LIGHT
shot light
unset BOREALIS_THEME
# In French, from Language, theme and text size (the screens built again)
shot french Right Down Down Right Right Right Right Return Return Down Down Return
# The demo's theme folders (resources/themes): pictures, logo, decorations; particles held still
export BRLS_STILL_FOCUS=1
APP_ARGS="-t winter" shot winter
APP_ARGS="-t prism" shot prism
BOREALIS_THEME=LIGHT APP_ARGS="-t aurora" shot aurora
unset BRLS_STILL_FOCUS
# palettes.png is taken by hand: the header's theme button, then Palette

# The tour: the Cadran tab and its dialogs, then the other tabs.
start
ffmpeg -y -loglevel error -f x11grab -framerate 25 -video_size 1280x720 -i "$DISPLAY" -t 24 \
    -c:v libx264 -preset veryfast -crf 18 /tmp/aurore_tour.mp4 &
FF=$!
sleep 2
key Right 1.2; key Down 1.2
key Right 0.6; key Right 1; key Return 2.4; key Escape 1.2
key Right 1; key Return 2.4; key Escape 1.2
key Left 0.6; key Left 0.6; key Left 0.6; key Left 1.2
key Down 1.6; key Down 1.6; key Down 1.4; key Down 1.4; key Down 1.2; key Down 1.6
wait $FF
stop
ffmpeg -y -loglevel error -i /tmp/aurore_tour.mp4 -vf "fps=12,scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle" "$OUT/tour.gif"
rm /tmp/aurore_tour.mp4
