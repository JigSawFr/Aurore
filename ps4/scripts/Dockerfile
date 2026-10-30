# The PS4's toolchain: PacBrew's OpenOrbis, with LLVM 23's clang and runtimes
# (libc++, libc++abi, libunwind, compiler-rt's builtins) in place of its
# LLVM 12, for C++23 and its library. Built by the CI's `ps4` job; by hand:
#   docker build -t aurore-ps4 ps4/scripts
FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates curl git patch cmake make ninja-build \
        pkg-config python3 xz-utils zstd libarchive-tools libgpgme11 pacman-package-manager && \
    rm -rf /var/lib/apt/lists/*

# PacBrew's toolchain and portlibs, without its clang and libc++ (LLVM 12),
# nor fmt and tinyxml2, which Aurore brings in newer versions
RUN echo "[pacbrew]" >> /etc/pacman.conf && \
    echo "SigLevel = Optional TrustAll" >> /etc/pacman.conf && \
    echo "Server = https://pacman.mydedibox.fr/pacbrew/packages/" >> /etc/pacman.conf && \
    ln -s /proc/mounts /etc/mtab && \
    pacman -Sy --noconfirm && \
    pacman -S --noconfirm ps4-openorbis pacbrew-ps4-portlibs && \
    pacman -Rdd --noconfirm ps4-openorbis-clang ps4-openorbis-libcxx ps4-openorbis-libfmt ps4-openorbis-tinyxml2 && \
    yes | pacman -Scc

ARG LLVM_VERSION=23.1.4
ARG LLVM_RELEASE=https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}
ENV OPENORBIS=/opt/pacbrew/ps4/openorbis

# LLVM's own build of clang and lld, kept to what OpenOrbis calls
RUN set -e && \
    cd /tmp && \
    curl -fsSL -o llvm.tar.xz $LLVM_RELEASE/LLVM-$LLVM_VERSION-Linux-X64.tar.xz && \
    echo "25d814fda50d5fb0084b42be09605d8b1595d49d5e2e03a3345d7e112ff6f9f3  llvm.tar.xz" | sha256sum -c - && \
    mkdir -p /opt/llvm && \
    tar -xJf llvm.tar.xz -C /opt/llvm --strip-components=1 --wildcards \
        '*/bin/clang' '*/bin/clang++' '*/bin/clang-23' '*/bin/lld' '*/bin/ld.lld' \
        '*/bin/llvm-ar' '*/bin/llvm-ranlib' '*/bin/llvm-nm' '*/bin/llvm-objcopy' '*/bin/llvm-strip' \
        '*/lib/clang/23/include/*' && \
    rm llvm.tar.xz && \
    for tool in clang clang++ ld.lld llvm-ar llvm-ranlib llvm-nm llvm-objcopy llvm-strip; do \
        ln -sf /opt/llvm/bin/$tool $OPENORBIS/bin/$tool; \
    done

# The runtimes, built from LLVM's sources with libcxx-ps4.patch; and
# OpenOrbis' headers and flags for them (openorbis-ps4.patch)
COPY build-runtimes.sh libcxx-ps4.patch openorbis-ps4.patch /tmp/ps4/
RUN set -e && \
    cd /tmp/ps4 && \
    curl -fsSL -o llvm-project.tar.xz $LLVM_RELEASE/llvm-project-$LLVM_VERSION.src.tar.xz && \
    echo "819ecf7351f1a66f76b2d701d8942faaf5faeb324cb90c9d5b090de142434bb2  llvm-project.tar.xz" | sha256sum -c - && \
    tar -xJf llvm-project.tar.xz --strip-components=1 \
        llvm-project-$LLVM_VERSION.src/cmake llvm-project-$LLVM_VERSION.src/compiler-rt \
        llvm-project-$LLVM_VERSION.src/libc llvm-project-$LLVM_VERSION.src/libcxx \
        llvm-project-$LLVM_VERSION.src/libcxxabi llvm-project-$LLVM_VERSION.src/libunwind \
        llvm-project-$LLVM_VERSION.src/llvm/cmake llvm-project-$LLVM_VERSION.src/llvm/utils/llvm-lit \
        llvm-project-$LLVM_VERSION.src/runtimes && \
    patch -p1 < libcxx-ps4.patch && \
    patch -d $OPENORBIS -p1 < openorbis-ps4.patch && \
    bash build-runtimes.sh /tmp/ps4 && \
    cp $OPENORBIS/ps4vars.sh /root/.bash_aliases && \
    rm -rf /tmp/ps4

RUN mkdir /src/ && \
    printf '%s\n' '#!/bin/bash -i' 'set -e' 'cd /src' 'echo "$@"' 'bash -c "$@"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENV LANG=en_US.UTF-8
VOLUME /src/
WORKDIR /src/
SHELL ["/bin/bash", "-i", "-c"]
ENTRYPOINT ["/entrypoint.sh"]
