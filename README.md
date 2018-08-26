# What is this?

Various demos for RaspberryPi.

# Compiling demos

You need to get gcc for OS you are running on your RaspberryPi.

If you use raspbian you can use compiler from https://github.com/raspberrypi/tools

    git clone --depth=1 https://github.com/raspberrypi/tools
    export PATH=`pwd`/tools/arm-bcm2708/arm-linux-gnueabihf/bin:"${PATH}"

    # compiler settings for rpi0 & rpi1
    export CFLAGS="-mcpu=arm1176jzf-s -mfpu=vfp"
    export CXXFLAGS="${CFLAGS}"

    # compiler settings for rpi2
    # export CFLAGS="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mthumb"
    # export CXXFLAGS="${CFLAGS}"

    # compiler settings for rpi3
    # export CFLAGS="-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mthumb"
    # export CXXFLAGS="${CFLAGS}"

# demo_bcm.c

Shows how to use GLESv2 with RaspberyPi closed-source OpenGL driver.

Make sure you don't have `dtoverlay=vc4-...` in your `/boot/config.txt`.

First you need to get RaspberryPI vc includes and libraries on your host.
For raspbian you can download them from https://github.com/raspberrypi/firmware

    git clone -b stable --depth=1 https://github.com/raspberrypi/firmware
    export FIRMWARE=`pwd`/firmware/hardfp
    export PKG_CONFIG_SYSROOT_DIR="${FIRMWARE}"
    export PKG_CONFIG_PATH="${FIRMWARE}"/opt/vc/lib/pkgconfig

Now you can compile the demo:

    arm-linux-gnueabihf-gcc -o demo_bcm demo_bcm.c ${CFLAGS} -std=gnu99 -O2 -Wall \
      -lm `pkg-config --cflags --libs brcmegl brcmglesv2`

Then upload & run the binary:

    scp demo_bcm pi@raspberrypi.local:
    ssh -t pi@raspberrypi.local ./demo_bcm

You should see spinning triangle (rendering code is in `render.h` file).

By default it enables vsync, to disable it change `ENABLE_VSYNC` value in demo_bcm.c file.

# demo_drm.c

Shows how to use GLESv2 with vc4 open-source driver & mesa/gmb/kms/drm libraries.

Make sure you enable vc4 driver in `/boot/config.txt`, put `dtoverlay=vc4-fkms-v3d` in config file.

You'll need to get up to date mesa library that has vc4 driver backend. Raspbian does not
have it enabled by default. Here are instructions how to compile mesa on your host machine:

    # where to place binaries, unset sysroot dir in case it was set from demo_bcm section
    export PREFIX=`pwd`/prefix
    export PKG_CONFIG_PATH="${PREFIX}"/lib/pkgconfig
    unset PKG_CONFIG_SYSROOT_DIR
    
    # download source code
    curl -Lf https://zlib.net/zlib-1.2.11.tar.xz | tar -xJ
    curl -Lf https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-2.2.6.tar.bz2 | tar -xj
    curl -Lf https://dri.freedesktop.org/libdrm/libdrm-2.4.93.tar.bz2 | tar -xj
    curl -Lf https://mesa.freedesktop.org/archive/mesa-18.2.0-rc4.tar.xz | tar -xJ
    
    # zlib
    cd zlib-1.2.11
    LDFLAGS='-Wl,-rpath=\$$ORIGIN' CHOST=arm-linux-gnueabihf ./configure --prefix="${PREFIX}"
    make install -j`nproc`
    cd ..

    # expat
    cd expat-2.2.6
    LDFLAGS='-Wl,-rpath=\$$ORIGIN' \
    ./configure --prefix="${PREFIX}" --host=arm-linux-gnueabihf --without-xmlwf
    make install -j`nproc`
    cd ..

    # libdrm
    cd libdrm-2.4.93
    LDFLAGS='-Wl,-rpath=\$$ORIGIN' \
    ./configure --prefix="${PREFIX}" --host=arm-linux-gnueabihf --enable-libkms --enable-vc4 \
      --disable-intel --disable-radeon --disable-amdgpu --disable-nouveau --disable-vmwgfx \
      --disable-freedreno --disable-cairo-tests --disable-valgrind --disable-manpages
    make install -j`nproc`
    cd ..

    # mesa, remove "--disable-asm" for rpi2/rpi3
    cd mesa-18.2.0-rc4
    LDFLAGS='-Wl,-rpath=\$$ORIGIN' CFLAGS="${CFLAGS} -DDEFAULT_DRIVER_DIR=\\\"\\\$\$ORIGIN\\\"" \
    ./configure --prefix="${PREFIX}" --host=arm-linux-gnueabihf --disable-asm \
      --with-platforms=drm --with-gallium-drivers=vc4 --without-dri-drivers \
      --enable-gbm --enable-egl --enable-gles2 --disable-gles1 --disable-glx \
      --disable-osmesa --disable-libunwind --disable-dri3 --disable-llvm \
      --disable-valgrind --disable-xa --disable-xvmc --disable-vdpau --disable-va
    make install -j`nproc`
    cd ..

    # remove debugging informaton to have smaller binaries
    arm-linux-gnueabihf-strip --strip-unneeded "${PREFIX}"/lib/*.so "${PREFIX}"/lib/dri/*.so

Now you'll have bunch of binaries in `${PREFIX}` folder. Copy them to your RaspberryPi:

    scp "${PREFIX}"/lib/{dri/vc4_dri.so,libEGL.so.1,libGLESv2.so.2,libgbm.so.1,libglapi.so.0,libdrm.so.2,libexpat.so.1,libz.so.1} pi@raspberrypi.local:

All .so files need to be in same folder where executable will be placed for it to work.

Then you can compile `demo_drm.c` file:

    arm-linux-gnueabihf-gcc -o demo_drm demo_drm.c ${CFLAGS} -std=gnu99 -O2 -Wall \
      -lm `pkg-config --cflags --libs egl glesv2 libdrm gbm` \
      -Wl,-rpath,'$ORIGIN',--unresolved-symbols=ignore-in-shared-libs

Then upload & run the binary:

    scp demo_drm pi@raspberrypi.local:
    ssh -t pi@raspberrypi.local ./demo_drm

You should see spinning triangle (rendering code is in `render.h` file).

By default it enables vsync, to disable it change `ENABLE_VSYNC` value in demo_drm.c file.
