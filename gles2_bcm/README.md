# gles2_bcm

Example how to do GLES2 rendering with Broadcom proprietary OpenGL driver.

This driver is available in Raspbian or 32-bit based Arch Linux ARM (armv6 & armv7) distributions.

# Preparations

First install necessary development files in sysroot.

For Raspbian do:

    python ../sysroot.py --distro raspbian --sysroot "${SYSROOT}" libraspberrypi-dev

For Arch Linux ARM:

    python ../sysroot.py --distro alarm --target "${TARGET}" --sysroot "${SYSROOT}" raspberrypi-firmware

Make sure your Raspberry Pi has not loaded open-source vc4 OpenGL driver. Check `/boot/config.txt` file,
it must not have `dtoverlay=vc4-fkms-v3d` or `dtoverlay=vc4-kms-v3d` setting enabled.

# Building & running

Execute `make` to build.

You can execute `make run` to build & upload & run executable on Raspberry Pi. By default it will
try to connect to pi@raspberrypi.local host. You can override it by setting `PI` variable:
`make run PI=alarm@alarmpi.local`

You should see colorful triangle spinning. By default vsync is enabled, but you can disable it by changing
`ENABLE_VSYNC` variable in source code.

Error handling is done using `assert`, feel free to change it when porting to your code.
