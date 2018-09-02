#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

if [ "$#" != 2 ]; then
    cat <<EOF
Usage: setup.sh <distro> <target>

<distro>
    raspbian - uses Rasbian sysroot
    alarm - uses Arch Linux ARM sysroot

<target>
    pi1 - armv6 code for Pi 0 and Pi 1
    pi2 - armv7 code for Pi 2 and Pi 3
    pi3 - armv7 code for Pi 3
    pi3-64 - armv8 code for Pi 3
EOF
    exit 1
fi

if [ "$1" == "raspbian" ]; then 
    export TARGET=arm-linux-gnueabihf
elif [ "$1" == "alarm" ]; then
    if [ "$2" == "pi1" ]; then
        export TARGET=armv6l-unknown-linux-gnueabihf
    elif [ "$2" == "pi2" ] || [ "$2" == "pi3" ]; then
        export TARGET=armv7l-unknown-linux-gnueabihf
    elif [ "$2" == "pi3-64" ]; then
        export TARGET=aarch64-unknown-linux-gnu
    else
        echo "ERROR: Unknown $2 target!"
        exit 1
    fi
else
    echo "ERROR: Unknown $1 distribution!"
    exit 1
fi

if [ "$2" == "pi1" ]; then
    RPI_CFLAGS="-mcpu=arm1176jzf-s -mfpu=vfp"
elif [ "$2" == "pi2" ]; then
    RPI_CFLAGS="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mthumb"
elif [ "$2" == "pi3" ]; then
    RPI_CFLAGS="-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mthumb"
elif [ "$2" == "pi3-64" ]; then
    RPI_CFLAGS="-march=armv8-a -mcpu=cortex-a53"
else
    echo "ERROR: Unknown $2 distribution!"
    exit 1
fi

# this is folder where clang & lld will live, created by ./bootstrap.sh
export TOOLCHAIN="${ROOT}"/toolchain

# this is sysroot which you need to download with ./sysroot.py script
export SYSROOT="${ROOT}"/sysroot/$1-$2

# use these env variables in your scripts / buildsystem for compiler/linker
export CC="${TOOLCHAIN}/rpi-clang"
export CXX="${TOOLCHAIN}/rpi-clang++"

mkdir -p "${SYSROOT}" "${TOOLCHAIN}"

cat > "${CC}" << EOF
#!/bin/sh
"${TOOLCHAIN}"/bin/clang -fuse-ld=lld -Wno-unused-command-line-argument ${RPI_CFLAGS} --target=${TARGET} --sysroot=${SYSROOT} "\$@"
EOF

cat > "${CXX}" << EOF
#!/bin/sh
"${TOOLCHAIN}"/bin/clang++ -fuse-ld=lld -Wno-unused-command-line-argument ${RPI_CFLAGS} --target=${TARGET} --sysroot=${SYSROOT} "\$@"
EOF

chmod +x "${CC}" "${CXX}"

export PKG_CONFIG_SYSROOT_DIR="${SYSROOT}"
export PKG_CONFIG_LIBDIR="${SYSROOT}/usr/local/lib/pkgconfig:${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/lib/${TARGET}/pkgconfig:${SYSROOT}/opt/vc/lib/pkgconfig"
export PKG_CONFIG_DIR=""

echo "Environment set up for $1 targeting $2"
