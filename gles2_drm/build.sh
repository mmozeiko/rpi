#!/bin/bash

set -eu

LIBDRM_VERSION=2.4.94
MESA_VERSION=18.2.0

curl -Lf https://dri.freedesktop.org/libdrm/libdrm-${LIBDRM_VERSION}.tar.bz2 | tar -xj
curl -Lf https://mesa.freedesktop.org/archive/mesa-${MESA_VERSION}.tar.xz | tar -xJ

cd libdrm-${LIBDRM_VERSION}
LDFLAGS="--sysroot=${SYSROOT} -Wl,-rpath=\\\$\$ORIGIN" \
./configure \
    --host=${TARGET} --enable-vc4 \
    --disable-intel --disable-radeon --disable-amdgpu \
    --disable-nouveau --disable-vmwgfx --disable-freedreno \
    --disable-libkms --disable-cairo-tests --disable-valgrind \
    --disable-manpages
make -j`nproc`
make -j`nproc` install DESTDIR="${SYSROOT}"
cd ..

# mesa seems to want to always use inline ARM NEON code when compiling for
# any ARM target, but armv6 does not support NEON
# we want to disable inline ARM NEON code unless NEON is allowed (rpi2 & rpi3)
if [[ `cat ${CC}` = *"neon"* ]] ||  [[ `cat ${CC}` = *"armv8-a"* ]]; then
    DISABLE_ASM=
else
    DISABLE_ASM=--disable-asm
fi

cd mesa-${MESA_VERSION}
CFLAGS="-O2 -fPIC -Wall -fno-math-errno -fno-trapping-math -DDEFAULT_DRIVER_DIR=\\\"\\\$\$ORIGIN\\\"" \
CXXFLAGS="${CFLAGS}" \
LDFLAGS="--sysroot=${SYSROOT} -Wl,-rpath=\\\$\$ORIGIN" \
./configure \
    --host=${TARGET} ${DISABLE_ASM} \
    --with-platforms=drm --with-gallium-drivers=vc4 --without-dri-drivers \
    --enable-gbm --enable-egl --enable-gles2 --disable-gles1 --disable-glx \
    --disable-osmesa --disable-libunwind --disable-dri3 --disable-llvm \
    --disable-valgrind --disable-xa --disable-xvmc --disable-vdpau --disable-va
make -j`nproc`
make -j`nproc` install DESTDIR="${SYSROOT}"
cd ..

rm -rf libdrm-${LIBDRM_VERSION} mesa-${MESA_VERSION}

for f in libgbm.so libglapi.so libEGL.so libGLESv2.so dri/vc4_dri.so;
do
  "${TOOLCHAIN}"/bin/llvm-objcopy -strip-unneeded `realpath "${SYSROOT}"/usr/local/lib/"${f}"`
  chrpath -r '$ORIGIN' "${SYSROOT}"/usr/local/lib/"${f}"
done
