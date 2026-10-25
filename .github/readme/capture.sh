#!/bin/bash
# The README's screenshots and tour.gif, from the demo's desktop build:
#   cmake -B build_pc -DPLATFORM_DESKTOP=ON && cmake --build build_pc
#   xvfb-run -a -s "-screen 0 1280x720x24" .github/readme/capture.sh
# Needs Xvfb, xdotool, ImageMagick, ffmpeg and dbus. The demo forces the dark
# theme; light.png is taken from a build that does not, demo/src/main.cpp
# being put back afterwards.
set -e
cd "$(dirname "$0")/../.."
OUT=.github/readme
APP=./build_pc/borealis_demo

start() {
    dbus-run-session -- $APP > /dev/null 2>&1 &
    sleep 6
}
stop() { pkill -x borealis_demo || true; sleep 1; }
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

shot components
shot layout Down
shot pokedex Down Down Down Down Down
shot settings Down Down Down Down Down Down

trap 'git checkout demo/src/main.cpp; cmake --build build_pc > /dev/null' EXIT
sed -i 's/ThemeVariant::DARK)/ThemeVariant::LIGHT)/' demo/src/main.cpp
cmake --build build_pc > /dev/null
shot light Down Down Down Down Down Down
git checkout demo/src/main.cpp
cmake --build build_pc > /dev/null
trap - EXIT

# The tour: every tab, the Pokédex scrolled, About.
start
ffmpeg -y -loglevel error -f x11grab -framerate 25 -video_size 1280x720 -i "$DISPLAY" -t 20 \
    -c:v libx264 -preset veryfast -crf 18 /tmp/borealis_tour.mp4 &
FF=$!
sleep 2.5
key Down 1.8; key Down 1.8; key Down 1.6; key Down 1.4; key Down 1.2
key Right 0.8
for i in 1 2 3 4 5 6; do key Down 0.55; done
sleep 0.4; key Left 0.8; key Down 1.6; key Down 2.5
wait $FF
stop
ffmpeg -y -loglevel error -i /tmp/borealis_tour.mp4 -vf "fps=12,scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle" "$OUT/tour.gif"
rm /tmp/borealis_tour.mp4
