.PHONY=all run clean

PI=pi@raspberrypi.local

SRC=gles2.c
EXE=gles2.exe
DEPENDENCIES=`pkg-config --cflags libdrm gbm bcm_host` # no libraries, only compiler flags

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	ssh -t ${PI} ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile
	"${CC}" -Os -s -Wall -Wextra -Wl,-rpath,'$$ORIGIN' -Ikhronos ${DEPENDENCIES} -lm -ldl -o $@ $<
