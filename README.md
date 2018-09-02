# Raspberry Pi

Various stuff with Raspberry Pi.

# Setting up toolchain

Demos in this repository requires clang toolchain that can target your distribution.
Following scripts will help you to compile clang on your GNU/Linux desktop for
targeting Raspberry Pi with [Raspbian][raspbian] or [Arch Linux ARM][alarm] distributions:

* `setup.sh` - sets up generic environment variables for specific distribution and Pi
* `sysroot.py` - unpacks raspbian or alarm packages in sysroot
* `bootstrap.sh` - compiles clang toolchain

First you need to set up environment variables by sourcing `setup.sh` file:

    source setup.sh <distro> <pi_version>

Where `distro` is `raspbian` for Rasbian, or `alarm` for Arch Linux ARM.
`pi_version` is which Raspberry Pi you want to target - `pi1`, `pi2`, `pi3` or `pi3-64`.
pi3-64 is available only for Arch Linux ARM.

This script will also set necessary environment variables for [pkg-config][pkgconfig] to work.

Remember to source it every time you want to use clang for compiling your code.

Next install basic packages (like C runtime) in sysroot with help of `sysroot.py` script.
For example, for Rasbian use this:

    python ./sysroot.py --distro raspbian --sysroot "${SYSROOT}" libc6-dev libstdc++-6-dev

Example for Arch Linux ARM:

    python ./sysroot.py --distro alarm --sysroot "${SYSROOT}" --target "${TARGET}" glibc gcc

This will create `sysroot` folder with development header files and libraries targeting your
choice of distribution and target architecutre. You'll need to repeat this process in case
you run `setup.sh` again with different distro/pi_version arguments. Each sysroot is setup
in separate folder.

Now execute `bootstrap.sh` script to compile clang:

    ./bootstrap.sh

This will create `toolchain` folder where it will install [clang compiler][clang] and [lld linker][lld] .
Clang will be built for target ARM and AArch64 targets. To use it in your code use `${CC}`
and `${CXX}` variables for path to C and C++ compilers.

# Demos

Currently available demos:

* [gles2_bcm](gles2_bcm) - example how to use GLES2 for Broadcom's propriatery OpenGL driver
* [gles2_drm](gles2_drm) - example how to use GLES2 for open-source vc4 OpenGL driver with libdrm & mesa

# /boot/config.txt stuff

`gpu_mem=128` - how much memoy to allocate for GPU in MiB

`dtoverlay=vc4-fkms-v3d` - enables open-source OpenGL driver

`hdmi_force_hotplug=1` - HDMI always enabled

`hdmi_group=2` - sets HDMI mode

`hdmi_mode=82` - forces 1920x1080@60 HDMI mode

`disable_overscan=0` - disables HDMI overscan

[clang]: https://clang.llvm.org/
[lld]: https://lld.llvm.org/
[raspbian]: https://www.raspberrypi.org/downloads/raspbian/
[alarm]: https://archlinuxarm.org/
[pkgconfig]: https://www.freedesktop.org/wiki/Software/pkg-config/
