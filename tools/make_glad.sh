#!/usr/bin/env bash
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Makes the OpenGL loader the GL drivers use, with glad 2.0.8: OpenGL 4.3
# (its compatibility profile, which the GL2 driver needs; the GL3 one asks
# for a core context) and OpenGL ES 3.2 in one header, no extensions,
# Aurore's code giving the loader (gladLoadGL / gladLoadGLES2 with GLFW's,
# SDL's or EGL's).
#
#   library/include/aurore/extern/glad/gl.h
#   library/include/aurore/extern/KHR/khrplatform.h
#   library/lib/extern/glad/gl.c
#
#     tools/make_glad.sh
#
# Needs Python 3 and the network (pip). --reproducible reads the
# specification glad 2.0.8 ships, so the output is the same on every run.
set -euo pipefail

root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT

python3 -m venv "${work}/venv"
"${work}/venv/bin/pip" install --quiet glad2==2.0.8
"${work}/venv/bin/glad" --quiet --reproducible --merge --api 'gl:compatibility=4.3,gles2=3.2' --extensions '' \
    --out-path "${work}/out" c

install -D -m 644 "${work}/out/include/glad/gl.h" "${root}/library/include/aurore/extern/glad/gl.h"
install -D -m 644 "${work}/out/include/KHR/khrplatform.h" "${root}/library/include/aurore/extern/KHR/khrplatform.h"
install -D -m 644 "${work}/out/src/gl.c" "${root}/library/lib/extern/glad/gl.c"
