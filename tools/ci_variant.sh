#!/usr/bin/env bash
# Builds one of the configurations the other CI jobs do not, on Linux (the CI
# job "linux (variants)"; also by hand, from the root of the checkout):
#
#   tools/ci_variant.sh <variant>
#
#   presets    the desktop-sdl preset
#   install    the library and its harness installed (AURORE_INSTALL), the
#              prefix moved, and tests/consumer built against it
#              (find_package(Aurore 0.1)) and run
#   fetch      AURORE_FETCH_DEPS: fmt, tinyxml2 and GLFW downloaded
#   fonts      FreeType and FriBidi
#   unity      the unity build, the whole library in one batch, with the tests
#   template   template/, the app to copy
#   legacy     borealis' option names, still read (with a deprecation warning)
#
# Build directories go to $VARIANT_DIR (default: build-variants).
set -euo pipefail

variant=${1:?"usage: $0 presets|install|fetch|fonts|unity|template|legacy"}
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
out=${VARIANT_DIR:-${root}/build-variants}
mkdir -p "${out}"
out=$(cd -- "${out}" && pwd)
jobs=${JOBS:-$(nproc)}
desktop=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP)

cd "${root}"
case "${variant}" in
presets)
    cmake --preset desktop-sdl
    cmake --build --preset desktop-sdl -j "${jobs}"
    ;;
install)
    build=${out}/install
    cmake -S . -B "${build}" "${desktop[@]}" -DAURORE_INSTALL=ON -DAURORE_BUILD_TESTS=ON \
        -DCMAKE_INSTALL_PREFIX="${out}/prefix"
    cmake --build "${build}" -j "${jobs}"
    rm -rf "${out}/prefix" "${out}/moved"
    cmake --install "${build}"
    # Relocatable: nothing in the package points into the prefix it was installed to
    if grep -rn "${out}/prefix" "${out}/prefix/lib/cmake"; then
        echo "the installed package names its install prefix" >&2
        exit 1
    fi
    mv "${out}/prefix" "${out}/moved"
    cmake -S tests/consumer -B "${out}/consumer" -G Ninja -DCMAKE_PREFIX_PATH="${out}/moved"
    cmake --build "${out}/consumer" -j "${jobs}"
    "${out}/consumer/aurore_consumer"
    ctest --test-dir "${out}/consumer" --output-on-failure
    ;;
fetch)
    cmake -S . -B "${out}/fetch" "${desktop[@]}" -DAURORE_WINDOW=GLFW -DAURORE_FETCH_DEPS=ON
    cmake --build "${out}/fetch" -j "${jobs}"
    ;;
fonts)
    cmake -S . -B "${out}/fonts" "${desktop[@]}" -DAURORE_LIBROMFS=ON -DAURORE_FREETYPE=ON -DAURORE_FRIBIDI=ON
    cmake --build "${out}/fonts" -j "${jobs}"
    ;;
unity)
    cmake -S . -B "${out}/unity" "${desktop[@]}" -DAURORE_LIBROMFS=ON -DAURORE_UNITY_BUILD=ON \
        -DCMAKE_UNITY_BUILD_BATCH_SIZE=0 -DAURORE_BUILD_TESTS=ON
    cmake --build "${out}/unity" -j "${jobs}"
    ;;
template)
    cmake -S template -B "${out}/template" "${desktop[@]}"
    cmake --build "${out}/template" -j "${jobs}"
    test -f "${out}/template/resources/i18n/en-US/app.json"
    test ! -e "${out}/template/resources/themes"
    ;;
legacy)
    # legacy-names: begin
    # What an app that has not moved to the AURORE_ names passes: borealis'
    # options, its BRLS_ ones, its headers and names (tests/legacy_names_tests.cpp)
    cmake -S . -B "${out}/legacy" -G Ninja -DCMAKE_BUILD_TYPE=Release -DPLATFORM_DESKTOP=ON -DUSE_SDL2=ON \
        -DUSE_LIBROMFS=ON -DBRLS_BUILD_TESTS=ON -DBRLS_DEV_TOOLS=ON -DBRLS_RESOURCES_DIR=. 2>&1 | tee "${out}/legacy.log"
    for name in PLATFORM_DESKTOP BRLS_BUILD_TESTS BRLS_DEV_TOOLS BRLS_RESOURCES_DIR "brls_add_tests()"; do
        grep -q "${name} is deprecated" "${out}/legacy.log"
    done
    grep -q "^AURORE_WINDOW:STRING=SDL2$" "${out}/legacy/CMakeCache.txt"
    grep -q "^AURORE_LIBROMFS:BOOL=ON$" "${out}/legacy/CMakeCache.txt"
    grep -q "^AURORE_BUILD_TESTS:BOOL=ON$" "${out}/legacy/CMakeCache.txt"
    grep -q "^AURORE_DEV_TOOLS:STRING=ON$" "${out}/legacy/CMakeCache.txt"
    cmake --build "${out}/legacy" -j "${jobs}" 2>&1 | tee "${out}/legacy-build.log"
    grep -q "<borealis.hpp> is deprecated: include <aurore.hpp>" "${out}/legacy-build.log"
    ctest --test-dir "${out}/legacy" --output-on-failure -R "aurore_(legacy_names|dev_tools)_tests"
    # legacy-names: end
    ;;
*)
    echo "unknown variant: ${variant}" >&2
    exit 2
    ;;
esac
