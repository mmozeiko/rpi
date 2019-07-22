#!/bin/bash

set -eu

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

LLVM_VERSION=8.0.1

mkdir -p "${TOOLCHAIN}"
mkdir -p "${ROOT}"/{llvm.src,llvm.build}
mkdir -p "${ROOT}"/llvm.src/tools/{clang,lld}

curl -Lf https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz | tar -xJ -C "${ROOT}"/llvm.src --strip-components=1
curl -Lf https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/cfe-${LLVM_VERSION}.src.tar.xz | tar -xJ -C "${ROOT}"/llvm.src/tools/clang --strip-components=1
curl -Lf https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/lld-${LLVM_VERSION}.src.tar.xz | tar -xJ -C "${ROOT}"/llvm.src/tools/lld --strip-components=1

# patching lld to work for armv6
# more info: https://reviews.llvm.org/D50076 & https://reviews.llvm.org/D50077
#curl -Lf https://reviews.llvm.org/D50076?download=true | patch -p0 -d "${ROOT}"/llvm.src/tools/lld
#curl -Lf https://reviews.llvm.org/D50077?download=true | patch -p2 -d "${ROOT}"/llvm.src/tools/lld

cd "${ROOT}"/llvm.build
cmake "${ROOT}"/llvm.src \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${TOOLCHAIN}" \
    -DLLVM_TARGETS_TO_BUILD="ARM;AArch64" \
    -DLLVM_ENABLE_ASSERTIONS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_BUILD_TESTS=OFF -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_ENABLE_WARNINGS=OFF -DLLVM_ENABLE_WERROR=OFF \
    -DLLVM_BUILD_DOCS=OFF -DLLVM_ENABLE_BINDINGS=OFF
cmake --build . -- -j`nproc`
cmake --build . --target install -- -j`nproc`

cd "${ROOT}"
rm -rf "${ROOT}"/{llvm.src,llvm.build}
