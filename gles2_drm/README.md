# gles2_drm

Example how to do GLES2 rendering with open-source [vc4] OpenGL driver by using [libdrm][libdrm] and [mesa][mesa].

This is only way how to do hardware accelerated OpenGL rendering on 64-bit Arch Linux ARM.


# Preparations

We will build our own mesa library, to have the latest version and to not depend on X11 libraries. Raspbian mesa
package does not even have vc4 backend.

First install necessary development files for libdrm and mesa in sysroot - they require zlib and expat libraries.

For Raspbian do:

    ../sysroot.py --distro raspbian --sysroot "${SYSROOT}" zlib1g-dev libexpat1-dev

For Ubuntu do:

    ../sysroot.py --distro ubuntu --sysroot "${SYSROOT}" zlib1g-dev libexpat1-dev

For Arch Linux ARM:

    ../sysroot.py --distro alarm --sysroot "${SYSROOT}" --target "${TARGET}" zlib expat

For Alpine Linux:

    ../sysroot.py --distro alpine --sysroot "${SYSROOT}" --target "${TARGET}" zlib-dev expat-dev linux-headers

Remember to install `zlib1g` and `libexpat1` for Raspbian & Ubuntu or `zlib` and `expat` packages for
Arch Linux ARM or `zlib`, `expat` and `libstdc++` for Alpine Linux on your device.

Now execute `./build.sh` script. This will download and build libdrm and mesa libraries in `${SYSROOT}/usr/local`.
Extra build dependencies for host machine are: [meson][meson], [ninja][ninja] [python-mako][python-mako], and
[patchelf][patchelf].

When it finishes, upload them to your Raspberry Pi, in this example we will put them in home folder:

    scp "${SYSROOT}"/usr/local/lib/{libdrm.so.2,libgbm.so.1,libglapi.so.0,libEGL.so.1,libGLESv2.so.2,dri/vc4_dri.so,dri/v3d_dri.so} pi@raspberrypi.local:

The libraries need to be placed in same folder as main executable. Then there will be no need to set LD_LIBRARY_PATH.

Make sure you load open-source vc4 OpenGL driver on your Raspberry Pi. On Raspbian or Arch Linux ARM
check `/boot/config.txt` file, on Ubuntu check `/boot/firmware/config.txt`, it should have
`dtoverlay=vc4-fkms-v3d` line. This is not needed for 64-bit Arch Linux ARM, it loads vc4 driver automatically.

For Ubuntu you'll need skip u-boot bootloader because it does not load extra overlay for vc4. Edit
`/boot/firmware/config.txt` and change `kernel` line to `kernel=vmlinuz`. Then comment out
`device_tree_address` line - add `#` comment in front of it. Now copy overlay folder to boot partition:

    sudo cp -r /lib/firmware/`uname -r`/device-tree/overlays /boot/firmware/

More information [here][ubuntu_bootloader].


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
[ubuntu_bootloader]: https://wiki.ubuntu.com/ARM/RaspberryPi#Change_the_bootloader
[patchelf]: https://nixos.org/patchelf.html
[meson]: https://mesonbuild.com/
[ninja]: https://ninja-build.org/
[python-mako]: https://www.makotemplates.org/
