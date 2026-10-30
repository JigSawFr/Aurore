# The Switch's toolchain at devkitA64 r30 (GCC 16.1, binutils 2.46), which
# devkitPro published as pacman packages on 2026-06-10 but not yet as a
# Docker image: the image the CI pins (r29.2, GCC 15.2, libnx 4.12.0) with
# r30's three packages installed over it, each checked against the SHA-256
# it had when pinned. libnx and the portlibs stay those of the pinned image:
# only the compiler moves. By hand:
#   docker build -t aurore-devkita64-r30 switch/scripts
# Built by the CI's `switch (gcc 16)` job.
FROM devkitpro/devkita64:20260219@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282

ARG PACKAGES=https://pkg.devkitpro.org/packages

# curl and the CA certificates to fetch them, ccache for the CI's builds
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates curl ccache && \
    rm -rf /var/lib/apt/lists/*

RUN set -e && \
    mkdir /tmp/r30 && cd /tmp/r30 && \
    printf '%s  %s\n' \
        66f100490dafe506495ee083bc932aa2c2f231b5dafaa2b203d1832389b700e9 devkitA64-r30-1-any.pkg.tar.zst \
        0d61ad4946eb1080c6e645c41f0cfaac0a2eba5fec88426fef8f03363e933b5a devkita64-binutils-2.46.0-1-linux_x86_64.pkg.tar.zst \
        8192b899f0a5fcfe10d11fa63a631b45310c0c8f0b98d88c7c283c16e9d6b0fa devkita64-gcc-16.1.0-1-linux_x86_64.pkg.tar.zst \
        > SHA256SUMS && \
    for f in $(cut -d' ' -f3 SHA256SUMS); do curl -fsSL --retry 3 -o "$f" "$PACKAGES/$f"; done && \
    sha256sum -c SHA256SUMS && \
    dkp-pacman -U --noconfirm ./*.pkg.tar.zst && \
    cd / && rm -rf /tmp/r30 && \
    /opt/devkitpro/devkitA64/bin/aarch64-none-elf-gcc --version | head -1
