# gles2_drm

Example how to do GLES2 rendering with open-source [vc4] OpenGL driver by using [libdrm][libdrm] and [mesa][mesa].

This is only way how to do hardware accelerated OpenGL rendering on 64-bit Arch Linux ARM.

# Preparations

We will build our own mesa library, to have the latest version and to not depend on X11 libraries. Raspbian mesa
package does not even have vc4 backend.

First install necessary development files for libdrm and mesa in sysroot - they require zlib and expat libraries.

For Raspbian do:

    python3 ../sysroot.py --distro raspbian --sysroot "${SYSROOT}" zlib1g-dev libexpat1-dev

For Arch Linux ARM:

    python3 ../sysroot.py --distro alarm --sysroot "${SYSROOT}" --target "${TARGET}" zlib expat

Remember to install `zlib1g` and `libexpat1` for Raspbian or `zlib` and `expat` packages for Arch Linux ARM on
your device.

Now execute `./build.sh` script. This will download and build libdrm and mesa libraries in `${SYSROOT}/usr/local`.
This script requires **chrpath** so it can modify rpath for newly built libraries.

When it finishes, upload them to your Raspberry Pi, in this example we will put them in home folder:

    scp "${SYSROOT}"/usr/local/lib/{libdrm.so.2,libgbm.so.1,libglapi.so.0,libEGL.so.1,libGLESv2.so.2,dri/vc4_dri.so} pi@raspberrypi.local:

The libraries need to be placed in same folder as main executable. Then there will be no need to set LD_LIBRARY_PATH.

Make sure you load open-source vc4 OpenGL driver on your Raspberry Pi. Check `/boot/config.txt` file,
it should have `dtoverlay=vc4-fkms-v3d` line. This is not needed for 64-bit Arch Linux ARM, it loads vc4
driver automatically.

# Building & running

Execute `make` to build.

You can execute `make run` to build & upload & run executable on Raspberry Pi. By default it will
try to connect to pi@raspberrypi.local host. You can override it by setting `PI` variable:
`make run PI=alarm@alarmpi.local`

You should see colorful triangle spinning. By default vsync is enabled, but you can disable it by changing
`ENABLE_VSYNC` variable in source code.

Error handling is done using `assert`, feel free to change it when porting to your code.

[vc4]: https://dri.freedesktop.org/docs/drm/gpu/vc4.html
[libdrm]: https://cgit.freedesktop.org/mesa/drm/
[mesa]: https://www.mesa3d.org/
