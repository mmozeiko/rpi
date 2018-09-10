.PHONY=all run clean

PI=pi@raspberrypi.local

SRC=gl2_drm.c
EXE=gl2_drm.exe
DEPENDENCIES=`pkg-config --cflags --libs libdrm gbm egl`

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	@# LIBGL_DRIVERS_PATH is needed only for ALPINE
	ssh -t ${PI} LIBGL_DRIVERS_PATH=. ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile
	"${CC}" -Os -s -Wall -Wextra -lm -Wl,-rpath,'$$ORIGIN' ${DEPENDENCIES} -o $@ $<
