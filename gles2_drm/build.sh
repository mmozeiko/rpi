#!/bin/bash

set -eu

curl -Lf https://dri.freedesktop.org/libdrm/libdrm-2.4.94.tar.bz2 | tar -xj
curl -Lf https://mesa.freedesktop.org/archive/mesa-18.2.0-rc5.tar.xz | tar -xJ

cd libdrm-2.4.94
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

# mesa seems want try to use inline NEON code when compiling for ARM
# allow inline ARM code only when NEON is allowed (rpi2 & rpi3)
if [[ `cat ${CC}` = *"neon"* ]] ||  [[ `cat ${CC}` = *"armv8-a"* ]]; then
    DISABLE_ASM=
else
    DISABLE_ASM=--disable-asm
fi

cd mesa-18.2.0-rc5
CFLAGS="-O2 -fPIC -Wall -fno-math-errno -fno-trapping-math -ffast-math -DDEFAULT_DRIVER_DIR=\\\"\\\$\$ORIGIN\\\"" \
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

rm -rf libdrm-2.4.94 mesa-18.2.0-rc5

for f in libgbm.so libglapi.so libEGL.so libGLESv2.so dri/vc4_dri.so;
do
  "${TOOLCHAIN}"/bin/llvm-objcopy -strip-unneeded `realpath "${SYSROOT}"/usr/local/lib/"${f}"`
  chrpath -r '$ORIGIN' "${SYSROOT}"/usr/local/lib/"${f}"
done
