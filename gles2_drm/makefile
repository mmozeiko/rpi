.PHONY=all run clean

PI=pi@raspberrypi.local

SRC=gles2_drm.c
EXE=gles2_drm.exe
DEPENDENCIES=`pkg-config --cflags --libs libdrm gbm egl glesv2`

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	ssh -t ${PI} ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile
	"${CC}" -Os -s -Wall -Wextra -lm -Wl,-rpath,'$$ORIGIN' ${DEPENDENCIES} -o $@ $<
