# The Switch's toolchain at devkitA64 r30 (GCC 16.1, binutils 2.46), which
# devkitPro published as pacman packages on 2026-06-10 but not yet as a
# Docker image: the image the CI pins (r29.2, GCC 15.2, libnx 4.12.0) with
# r30's packages installed over it, each checked against the SHA-256 it had
# when pinned (SHA256SUMS). libnx and the portlibs stay those of the pinned image:
# only the compiler moves. By hand:
#   docker build -t aurore-devkita64-r30 switch/scripts
# Built by the CI's `switch (gcc 16)` job.
FROM devkitpro/devkita64:20260219@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282

# ccache for the CI's builds
RUN apt-get update && \
    apt-get install -y --no-install-recommends ccache && \
    rm -rf /var/lib/apt/lists/*

# r30's packages from devkitPro's repository, through dkp-pacman (which
# checks devkitPro's signatures; pkg.devkitpro.org serves pacman, not curl),
# then each checked against the SHA-256 it had when pinned: a newer build
# fails here, and moving to it is a change of this file
COPY SHA256SUMS /tmp/r30/SHA256SUMS
RUN set -e && \
    cd /tmp/r30 && \
    dkp-pacman -Sy && \
    dkp-pacman -Sw --noconfirm --cachedir /tmp/r30 devkitA64 devkita64-gcc devkita64-binutils && \
    sha256sum *.pkg.tar.zst && \
    sha256sum -c SHA256SUMS && \
    test "$(ls *.pkg.tar.zst | wc -l)" = "$(wc -l < SHA256SUMS)" && \
    dkp-pacman -U --noconfirm $(cut -d' ' -f3 SHA256SUMS) && \
    cd / && rm -rf /tmp/r30 && \
    /opt/devkitpro/devkitA64/bin/aarch64-none-elf-gcc --version | head -1
