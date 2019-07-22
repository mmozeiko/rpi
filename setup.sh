#!/bin/bash

if [[ "${BASH_SOURCE}" == "${0}" ]]; then
  echo "You need to source, not run this script!"
  exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE}" )" >/dev/null && pwd )"

if [ "$#" != 2 ]; then
    cat <<EOF
Usage: setup.sh <distro> <target>

<distro>
    raspbian - uses Rasbian sysroot
    ubuntu - uses Ubuntu sysroot
    alarm - uses Arch Linux ARM sysroot
    alpine - uses Alpine Linux sysroot

<target>
    pi1 - armv6 code for Pi 0 and Pi 1
    pi2 - armv7 code for Pi 2 and Pi 3
    pi3 - armv7 code for Pi 3
    pi3-64 - armv8 code for Pi 3
    pi4 - armv7 code for Pi 4
EOF
    return 1
fi

if [ "$1" == "raspbian" ] || [ "$1" == "ubuntu" ]; then
    export TARGET=arm-linux-gnueabihf
elif [ "$1" == "alarm" ]; then
    if [ "$2" == "pi1" ]; then
        export TARGET=armv6l-unknown-linux-gnueabihf
    elif [ "$2" == "pi2" ] || [ "$2" == "pi3" ] || [ "$2" == "pi4" ]; then
        export TARGET=armv7l-unknown-linux-gnueabihf
    elif [ "$2" == "pi3-64" ]; then
        export TARGET=aarch64-unknown-linux-gnu
    else
        echo "ERROR: Unknown $2 target!"
        return 1
    fi
elif [ "$1" == "alpine" ]; then
    if [ "$2" == "pi1" ] || [ "$2" == "pi2" ] || [ "$2" == "pi3" ]; then
        export TARGET=armv6-alpine-linux-musleabihf
    elif [ "$2" == "pi3-64" ]; then
        export TARGET=aarch64-alpine-linux-musl
    else
        echo "ERROR: Unknown $2 target!"
        return 1
    fi
else
    echo "ERROR: Unknown $1 distribution!"
    return 1
fi

if [ "$2" == "pi1" ]; then
    RPI_CFLAGS="-mcpu=arm1176jzf-s -mfpu=vfp"
    MESON_CPU="armv6hl"
elif [ "$2" == "pi2" ]; then
    RPI_CFLAGS="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mthumb"
    MESON_CPU="armv7hl"
elif [ "$2" == "pi3" ]; then
    RPI_CFLAGS="-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mthumb"
    MESON_CPU="armv7hl"
elif [ "$2" == "pi3-64" ]; then
    RPI_CFLAGS="-mcpu=cortex-a53"
    MESON_CPU="aarch64"
elif [ "$2" == "pi4" ]; then
    RPI_CFLAGS="-mcpu=cortex-a72 -mfpu=neon-fp-armv8 -mthumb"
    MESON_CPU="armv7hl"
else
    echo "ERROR: Unknown $2 target!"
    return 1
fi

# this is folder where clang & lld will live, created by ./bootstrap.sh
export TOOLCHAIN="${ROOT}"/toolchain

# this is sysroot which you need to download with ./sysroot.py script
export SYSROOT="${ROOT}"/sysroot/$1-$2

mkdir -p "${SYSROOT}" "${TOOLCHAIN}"

CC="${TOOLCHAIN}/rpi-clang"
CXX="${TOOLCHAIN}/rpi-clang++"

cat > "${CC}" << EOF
#!/bin/sh
"${TOOLCHAIN}"/bin/clang -fuse-ld=lld -Wno-unused-command-line-argument ${RPI_CFLAGS} --target=${TARGET} --sysroot=${SYSROOT} "\$@"
EOF

cat > "${CXX}" << EOF
#!/bin/sh
"${TOOLCHAIN}"/bin/clang++ -fuse-ld=lld -Wno-unused-command-line-argument ${RPI_CFLAGS} --target=${TARGET} --sysroot=${SYSROOT} "\$@"
EOF

cat > "${TOOLCHAIN}/meson-cross.txt" << EOF
[binaries]
c = '${CC}'
cpp = '${CXX}'
ar = '${TOOLCHAIN}/bin/llvm-ar'
strip = '${TOOLCHAIN}/bin/llvm-strip'
pkgconfig = '`which pkg-config`'

[properties]
c_link_args = ['-Wl,-rpath=\$ORIGIN']
sys_root = '${SYSROOT}'

[host_machine]
system = 'linux'
cpu_family = 'arm'
cpu = '${MESON_CPU}'
endian = 'little'
EOF

chmod +x "${CC}" "${CXX}"

export PKG_CONFIG_SYSROOT_DIR="${SYSROOT}"
export PKG_CONFIG_LIBDIR="${SYSROOT}/usr/local/lib/pkgconfig:${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/lib/${TARGET}/pkgconfig:${SYSROOT}/opt/vc/lib/pkgconfig"
export PKG_CONFIG_DIR=""

echo "Environment set up for $1 targeting $2"
