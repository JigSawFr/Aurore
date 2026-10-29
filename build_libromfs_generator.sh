# SPDX-FileCopyrightText: borealis contributors
# SPDX-License-Identifier: Apache-2.0

set -e

# ======== Manually building libromfs generator ========
# This project supports using libromfs to package the main program and resource files together to avoid additional file dependencies
# The function of libromfs-generator is to convert resource files into cpp source code and link them into the main program
# There are two solutions for cross-platform compilation:
# 1. Pre-compile libromfs-generator in the format of the host machine.
# 2. Set CMAKE_CROSSCOMPILING_EMULATOR to simulate execution libromfs-generator on the host.
# This script is used to generate libromfs-generator in the format of the host machine.

echo "Build libromfs-generator"

# absolute, so the paths below do not depend on where the script is called from
PROJECT_PATH=$(cd -- "$(dirname -- "$0")" && pwd)
LIBROMFS_PATH="${PROJECT_PATH}/library/lib/extern/libromfs/generator"
BUILD_DIR="${PROJECT_PATH}/build_libromfs_generator"
# Ninja when it is around, the default generator otherwise
GENERATOR=()
command -v ninja > /dev/null 2>&1 && GENERATOR=(-G Ninja)

cd "${PROJECT_PATH}"

# build libromfs-generator: the source directory is spelled out, as a plain
# path after -B is ignored by current cmake versions
cmake -S "${LIBROMFS_PATH}" -B "${BUILD_DIR}" ${GENERATOR[@]+"${GENERATOR[@]}"}
cmake --build "${BUILD_DIR}"

# put libromfs-generator under the jni folder
cp "${BUILD_DIR}/libromfs-generator" "${PROJECT_PATH}"
echo "Build libromfs-generator: ${PROJECT_PATH}/libromfs-generator"

# remove build folder
rm -rf "${BUILD_DIR}"
echo "Remove temp build dir: ${BUILD_DIR}"
