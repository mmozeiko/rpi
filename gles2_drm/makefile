.PHONY=all run clean

PI=pi@raspberrypi.local

CC="${TOOLCHAIN}"/rpi-clang

SRC=gles2_drm.c
EXE=gles2_drm.exe
DEPENDENCIES=`pkg-config --cflags --libs libdrm gbm egl glesv2`

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	@# LIBGL_DRIVERS_PATH is needed only for ALPINE
	ssh -t ${PI} LIBGL_DRIVERS_PATH=. ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile ../common/render.h
	${CC} -Os -s -Wall -Wextra -lm -Wl,-rpath,'$$ORIGIN' ${DEPENDENCIES} -o $@ $<
