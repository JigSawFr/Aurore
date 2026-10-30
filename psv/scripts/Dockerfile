# The Vita's OpenGL ES 2 toolchain: VitaSDK, PVR_PSP2 and SDL2 built with
# VIDEO_VITA_PVR. Built by the CI's `psv (gles2)` job; by hand:
#   docker build -t aurore-vita-gles2 psv/scripts
# The same VitaSDK as the GXM job (Ubuntu 24.04, GCC 15.2).
FROM vitasdk/vitasdk@sha256:61c173463b3d10d71464b6fc591f1fe1018736bf2e321e6852db06612eb05b07

# git, wget and bsdtar (for the zips) come with the image
RUN git config --global --add safe.directory '*'

# PVR_PSP2 (GLES): its headers at the tag's commit, its stubs and modules
# checked against their hashes
ARG PVR_PSP2_VERSION=3.9
ARG PVR_PSP2_COMMIT=bb441056082bc7d51085c98b8c99e74d9c94de4f
RUN set -e && \
    cd /tmp && \
    git clone --depth 1 --branch v$PVR_PSP2_VERSION https://github.com/GrapheneCt/PVR_PSP2 pvr && \
    test "$(git -C pvr rev-parse HEAD)" = "$PVR_PSP2_COMMIT" && \
    release=https://github.com/GrapheneCt/PVR_PSP2/releases/download/v$PVR_PSP2_VERSION && \
    wget -q $release/vitasdk_stubs.zip $release/PSVita_Release.zip && \
    echo "7ee2498b58cb97871fcb0e3e134ce1045acf2c22ce4873b1844a391b5da4fe47  vitasdk_stubs.zip" | sha256sum -c - && \
    echo "ed69be89f21c4894e8009a8c3567c89b1778c8db0beb3c2f4ea134adab4c494f  PSVita_Release.zip" | sha256sum -c - && \
    target=$VITASDK/arm-vita-eabi && \
    mkdir -p $target/include $target/lib $target/suprx && \
    cp -r pvr/include/* $target/include && \
    sed -i -e s/__drvkhrplatform_h_/__khrplatform_h_/ $target/include/KHR/khrplatform.h && \
    mkdir pvr_stubs && bsdtar -xf vitasdk_stubs.zip -C pvr_stubs && \
    find pvr_stubs -type f -name "*.a" -exec cp {} $target/lib \; && \
    mkdir pvr_modules && bsdtar -xf PSVita_Release.zip -C pvr_modules && \
    rm pvr_modules/libGLESv1_CM.suprx pvr_modules/libpvr2d.suprx && \
    mv pvr_modules/*.suprx $target/suprx/ && \
    rm -rf /tmp/*

# SDL2 with OpenGL ES 2 in place of VitaSDK's (sdl2_vitagl, which its other SDL
# packages keep depending on); makepkg refuses root
ADD sdl2 /vdpm/sdl2
RUN useradd --create-home builder && \
    chown -R builder:builder /vdpm && \
    su builder -c "cd /vdpm/sdl2 && VITASDK=$VITASDK PATH=$PATH vita-makepkg" && \
    vdpm pacman -- --remove --nodeps --nodeps --noconfirm sdl2_vitagl && \
    vdpm pacman -- --upgrade --noconfirm /vdpm/sdl2/sdl2-*-vita.pkg.tar.xz && \
    rm -rf /vdpm

# Runs its arguments in /src, after copying the PVR_PSP2 modules to
# psv/module/, which the GLES2 .vpk packs
RUN mkdir /src/ && \
    printf '%s\n' '#!/bin/bash' 'set -e' \
        'mkdir -p /src/psv/module/' \
        "cp ${VITASDK}/arm-vita-eabi/suprx/*.suprx /src/psv/module/" \
        'cd /src' 'echo "$@"' 'bash -c "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

VOLUME /src/
WORKDIR /src/
ENTRYPOINT ["/entrypoint.sh"]
