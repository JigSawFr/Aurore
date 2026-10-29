#!/usr/bin/env bash
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Builds SDL_shadercross, SDL's official shader translator, which
# tools/make_sdlgpu_shaders.py runs to turn nanovg's SPIR-V shaders into the
# MSL and DXIL of SDL_GPU's Metal and Direct3D 12 drivers. Linux x86_64.
#
#     tools/build_shadercross.sh [prefix]     default: build/shadercross
#
# Then <prefix>/bin/shadercross, which make_sdlgpu_shaders.py finds there
# (or through SHADERCROSS). The versions are pinned below, so the shaders
# come out the same on every run:
#   - SDL_shadercross and the SPIRV-Cross it pins, built from source;
#   - DirectXShaderCompiler's release (libdxcompiler and libdxil, which
#     signs the DXIL), checked against its SHA-512;
#   - SDL3: the submodule Aurore builds.
# Needs git, curl, CMake, Ninja and a C++ compiler.
set -euo pipefail

SHADERCROSS_COMMIT=1ff05bec573988a98ef9e0260b4da44f512b8367
SPIRV_CROSS_COMMIT=1a6169566c73d3da552748fc372fe2bbb856e46e
DXC_TAG=v1.9.2602.24
DXC_ARCHIVE=linux_dxc_2026_05_26.x86_64.tar.gz
DXC_SHA512=3da18b4b32899c65881276315411092fa1a076e25bbea20606bc247c5fa93c83f948785969e007115dc72afcd433ab76e54c392de6d37c11d93d8614a829e40a

ROOT=$(cd "$(dirname "$0")/.." && pwd)
PREFIX=$(mkdir -p "${1:-$ROOT/build/shadercross}" && cd "${1:-$ROOT/build/shadercross}" && pwd)
WORK="$PREFIX/work"
mkdir -p "$WORK"

# Already built at these versions (CI's cache)
STAMP="$SHADERCROSS_COMMIT $SPIRV_CROSS_COMMIT $DXC_TAG"
if [ -x "$PREFIX/bin/shadercross" ] && [ "$(cat "$PREFIX/bin/versions" 2> /dev/null)" = "$STAMP" ]; then
    echo "$PREFIX/bin/shadercross"
    exit 0
fi

fetch() { # url dir commit
    if [ ! -d "$2/.git" ]; then
        git init -q "$2"
        git -C "$2" remote add origin "$1"
    fi
    git -C "$2" fetch -q --depth 1 origin "$3"
    git -C "$2" checkout -q FETCH_HEAD
}

# DirectXShaderCompiler, as Microsoft releases it
if [ ! -f "$PREFIX/dxc/lib/libdxcompiler.so" ]; then
    curl -sSfL -o "$WORK/$DXC_ARCHIVE" \
        "https://github.com/microsoft/DirectXShaderCompiler/releases/download/$DXC_TAG/$DXC_ARCHIVE"
    echo "$DXC_SHA512  $WORK/$DXC_ARCHIVE" | sha512sum -c --quiet
    mkdir -p "$PREFIX/dxc"
    tar -xzf "$WORK/$DXC_ARCHIVE" -C "$PREFIX/dxc"
fi

# SDL3, from Aurore's submodule
if [ ! -f "$PREFIX/lib/cmake/SDL3/SDL3Config.cmake" ]; then
    cmake -S "$ROOT/library/lib/extern/SDL3" -B "$WORK/sdl3" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF \
        -DSDL_UNIX_CONSOLE_BUILD=ON > /dev/null
    cmake --build "$WORK/sdl3" > /dev/null
    cmake --install "$WORK/sdl3" > /dev/null
fi

# SPIRV-Cross, the commit SDL_shadercross pins, with its C API
if [ ! -f "$PREFIX/lib/libspirv-cross-c.a" ]; then
    fetch https://github.com/KhronosGroup/SPIRV-Cross.git "$WORK/SPIRV-Cross" "$SPIRV_CROSS_COMMIT"
    cmake -S "$WORK/SPIRV-Cross" -B "$WORK/spirv-cross-build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_STATIC=ON -DSPIRV_CROSS_CLI=OFF \
        -DSPIRV_CROSS_ENABLE_TESTS=OFF > /dev/null
    cmake --build "$WORK/spirv-cross-build" > /dev/null
    cmake --install "$WORK/spirv-cross-build" > /dev/null
fi

# SDL_shadercross, its command line
fetch https://github.com/libsdl-org/SDL_shadercross.git "$WORK/SDL_shadercross" "$SHADERCROSS_COMMIT"
# Where its build looks for DirectXShaderCompiler's release
ln -sfn "$PREFIX/dxc" "$WORK/SDL_shadercross/external/DirectXShaderCompiler-binaries"
cmake -S "$WORK/SDL_shadercross" -B "$WORK/shadercross-build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DSDLSHADERCROSS_VENDORED=OFF -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF \
    -DSDLSHADERCROSS_SHARED=OFF -DSDLSHADERCROSS_STATIC=ON -DSDLSHADERCROSS_CLI=ON \
    -DSDLSHADERCROSS_INSTALL=OFF > /dev/null
cmake --build "$WORK/shadercross-build" > /dev/null
mkdir -p "$PREFIX/bin"
cp "$WORK/shadercross-build/shadercross" "$PREFIX/bin/shadercross"
# The libraries it loads next to it (make_sdlgpu_shaders.py gives the
# folder to the loader), so that the folder alone is the tool
cp "$PREFIX/dxc/lib/libdxcompiler.so" "$PREFIX/dxc/lib/libdxil.so" "$PREFIX/bin/"
cp -P "$PREFIX"/lib/libSDL3.so* "$PREFIX/bin/"
echo "$STAMP" > "$PREFIX/bin/versions"
echo "$PREFIX/bin/shadercross"
