# Raspberry Pi

Various stuff with Raspberry Pi.


# Setting up toolchain

Demos in this repository requires clang toolchain that targets your Raspberry Pi
distribution. Following scripts will help you to compile clang on GNU/Linux desktop
for targeting Raspberry Pi with [Raspbian][raspbian], [Ubuntu][ubuntu],
[Arch Linux ARM][alarm] or [Alpine Linux][alpine] distributions:

* `setup.sh` - sets up generic environment variables for specific distribution and Pi
* `sysroot.py` - unpacks raspbian or alarm packages in sysroot
* `bootstrap.sh` - compiles clang toolchain

First you need to set up environment variables by sourcing `setup.sh` file:

    source setup.sh <distro> <pi_version>

Where `distro` is `raspbian` for Rasbian, `ubuntu` for Ubuntu, `alarm` for Arch Linux ARM
or `alpine` for Alpine Linux. `pi_version` is which Raspberry Pi you want to target -
`pi1`, `pi2`, `pi3`, `pi3-64`, `pi4` or `pi4-64`. pi3-64 is available only for Arch Linux ARM
and Alpine Linux.  pi4 is tested only on Arch Linux ARM and Raspbian. pi4-64 is tested only
on Arch Linux ARM.

This script will also set necessary environment variables for [pkg-config][pkgconfig] to
work - it will use only packages from sysroot folder, not from your host folder.

Remember to source it every time you want to use clang for compiling your code.

Next install basic packages (like C runtime) in sysroot with help of `sysroot.py` script.
For example, for Rasbian use this:

    ./sysroot.py --distro raspbian --sysroot "${SYSROOT}" libc6-dev libstdc++-6-dev

For Ubuntu:

    ./sysroot.py --distro ubuntu --sysroot "${SYSROOT}" libc6-dev libstdc++-6-dev

For Arch Linux ARM:

    ./sysroot.py --distro alarm --sysroot "${SYSROOT}" --target "${TARGET}" gcc glibc

For Alpine Linux:

    ./sysroot.py --distro alpine --sysroot ${SYSROOT} --target ${TARGET} g++ musl-dev

Make sure you have `dpkg-deb` executable available if you are targeting rasbian or ubuntu
(on Arch install it from AUR).

This script will create `sysroot` folder with development header files and libraries targeting
your choice of distribution and target architecutre. You'll need to repeat this process in case
you run `setup.sh` again with different distro/pi_version arguments. Each sysroot is setup
in separate folder. Package resolving is very simplified and may fail on complex dependencies,
also it **DOES NOT** verify integrity of downloaded packages, be careful!

Now execute `bootstrap.sh` script to compile clang:

    ./bootstrap.sh

This script requires **gcc** based buildsystem and **cmake** on your host machine. It will create
`toolchain` folder where it will install [clang compiler][clang] and [lld linker][lld]. Clang
will be built for ARM and AArch64 targets. To use it in your code use `${CC}` and `${CXX}`
variables for path to C and C++ compilers.


# Demos

Currently available demos:

* [gles2_bcm](gles2_bcm) - how to use GLES2 for Broadcom's propriatery OpenGL driver
* [gles2_drm](gles2_drm) - how to use GLES2 for open-source vc4 OpenGL driver with libdrm & mesa
* [gles2](gles2) - how to use GLES2 with any of two OpenGL drivers
* [gl2_drm](gl2_drm) - how to use GL2 for open-source vc4 OpenGL driver with libdrm & mesa


# Useful info about Raspberry Pi

To check which Pi you have, check contents of `/proc/device-tree/model` file. Alternatively check
`Revision` field in `/proc/cpuinfo` file and match to [this table][rpirev].

To get temperature of Pi, check `/sys/class/thermal/thermal_zone0/temp` (divide by 1000 to get °C).


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
[ubuntu]: https://wiki.ubuntu.com/ARM/RaspberryPi
[alarm]: https://archlinuxarm.org/
[alpine]: https://alpinelinux.org
[pkgconfig]: https://www.freedesktop.org/wiki/Software/pkg-config/
[rpirev]: https://elinux.org/RPi_HardwareHistory#Board_Revision_History
