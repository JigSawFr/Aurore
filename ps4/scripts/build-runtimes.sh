#!/bin/bash
# Builds LLVM's runtimes for the PS4 against OpenOrbis' musl, as PacBrew's
# ps4-openorbis-libcxx does for LLVM 12: compiler-rt's builtins, then
# libunwind, libc++abi and libc++, merged into libc++.a (ps4.cmake links -lc++).
# Built for x86_64-unknown-linux-gnu like PacBrew's, with __PS4__; the apps'
# FreeBSD target reads the same headers (libcxx-ps4.patch).
#   build-runtimes.sh <llvm-project source>
set -e
SRC=$1
O=${OPENORBIS:-/opt/pacbrew/ps4/openorbis}
B=$(mktemp -d)

common=(
  -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_C_COMPILER=$O/bin/clang -DCMAKE_CXX_COMPILER=$O/bin/clang++ -DCMAKE_ASM_COMPILER=$O/bin/clang
  -DCMAKE_AR=$O/bin/llvm-ar -DCMAKE_RANLIB=$O/bin/llvm-ranlib -DCMAKE_NM=$O/bin/llvm-nm
  -DCMAKE_SYSROOT=$O -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
)

cmake "${common[@]}" -S $SRC/compiler-rt/lib/builtins -B $B/builtins \
  -DCMAKE_C_COMPILER_TARGET=x86_64-unknown-linux-gnu -DCMAKE_ASM_COMPILER_TARGET=x86_64-unknown-linux-gnu \
  -DCMAKE_C_FLAGS="-fPIC -D__PS4__" -DCMAKE_ASM_FLAGS="-fPIC -D__PS4__" \
  -DCOMPILER_RT_BAREMETAL_BUILD=ON -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
  -DLLVM_CMAKE_DIR=$SRC/llvm/cmake/modules
ninja -C $B/builtins
cp $B/builtins/lib/linux/libclang_rt.builtins-x86_64.a $O/lib/

flags="-fPIC -D__PS4__ -D_LIBUNWIND_IS_BAREMETAL=1 -I$O/usr/include"
cmake "${common[@]}" -S $SRC/runtimes -B $B/runtimes \
  -DLLVM_ENABLE_RUNTIMES="libunwind;libcxxabi;libcxx" \
  -DCMAKE_C_FLAGS="$flags" -DCMAKE_CXX_FLAGS="$flags" \
  -DCMAKE_INSTALL_PREFIX=$O \
  -DLIBUNWIND_ENABLE_SHARED=OFF -DLIBUNWIND_USE_COMPILER_RT=ON -DLIBUNWIND_INCLUDE_TESTS=OFF \
  -DLIBCXXABI_ENABLE_SHARED=OFF -DLIBCXXABI_USE_COMPILER_RT=ON -DLIBCXXABI_INCLUDE_TESTS=OFF \
  -DLIBCXXABI_USE_LLVM_UNWINDER=ON -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
  -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_USE_COMPILER_RT=ON \
  -DLIBCXX_HAS_MUSL_LIBC=ON -DLIBCXX_HAS_PTHREAD_API=ON \
  -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
  -DLIBCXX_INCLUDE_BENCHMARKS=OFF -DLIBCXX_INCLUDE_TESTS=OFF
ninja -C $B/runtimes install-cxx install-cxxabi install-unwind

rm -rf $B
