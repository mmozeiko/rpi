#!/bin/bash

set -eu

LIBDRM_VERSION=2.4.99
MESA_VERSION=19.1.2

curl -Lf https://dri.freedesktop.org/libdrm/libdrm-${LIBDRM_VERSION}.tar.bz2 | tar -xj
curl -Lf https://mesa.freedesktop.org/archive/mesa-${MESA_VERSION}.tar.xz | tar -xJ

cd libdrm-${LIBDRM_VERSION}

meson setup build \
  --cross-file "${TOOLCHAIN}"/meson-cross.txt \
  --buildtype=release \
  -Dlibkms=false -Dintel=false -Dradeon=false -Damdgpu=false -Dnouveau=false \
  -Dvmwgfx=false -Domap=false -Dexynos=false -Dfreedreno=false -Dtegra=false \
  -Dvc4=true -Detnaviv=false -Dcairo-tests=false -Dman-pages=false \
  -Dvalgrind=false -Dfreedreno-kgsl=false
ninja -C build
DESTDIR="${SYSROOT}" ninja -C build install

cd ..
cd mesa-${MESA_VERSION}

meson setup build \
  --cross-file "${TOOLCHAIN}"/meson-cross.txt \
  --buildtype=release -Db_ndebug=true \
  -Dplatforms=drm -Ddri-drivers= -Ddri-search-path='$ORIGIN' \
  -Dgallium-drivers=kmsro,v3d,vc4 -Dgallium-vdpau=false -Dgallium-xvmc=false \
  -Dgallium-omx=disabled -Dgallium-va=false -Dgallium-xa=false -Dvulkan-drivers= \
  -Dshared-glapi=true -Dgles1=false -Dgles2=true -Dopengl=true -Dgbm=true \
  -Dglx=disabled -Degl=true -Dasm=false -Dllvm=false -Dvalgrind=false \
  -Dlibunwind=false -Dlmsensors=false
ninja -C build
DESTDIR="${SYSROOT}" ninja -C build install

cd ..

rm -rf libdrm-${LIBDRM_VERSION} mesa-${MESA_VERSION}

for f in libgbm.so libglapi.so libEGL.so libGLESv2.so dri/v3d_dri.so dri/vc4_dri.so;
do
  "${TOOLCHAIN}"/bin/llvm-objcopy -strip-unneeded `realpath "${SYSROOT}"/usr/local/lib/"${f}"`
  patchelf --set-rpath '$ORIGIN' "${SYSROOT}"/usr/local/lib/"${f}"
done
