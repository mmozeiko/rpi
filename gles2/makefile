.PHONY=all run clean

PI=pi@raspberrypi.local

SRC=gles2.c
EXE=gles2.exe
ifeq (0,$(shell pkg-config --exists bcm_host; echo $$?))
  # no libraries, only compiler flags
  DEPENDENCIES=`pkg-config --cflags libdrm gbm bcm_host`
else
  # ubuntu does not have .pc files for Broadcom libraries
  DEPENDENCIES=`pkg-config --cflags libdrm gbm`
endif

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	ssh -t ${PI} ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile ../common/render.h
	"${CC}" -Os -s -Wall -Wextra -Wl,-rpath,'$$ORIGIN' -Ikhronos ${DEPENDENCIES} -lm -ldl -o $@ $<
