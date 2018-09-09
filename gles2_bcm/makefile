.PHONY=all run clean

PI=pi@raspberrypi.local

SRC=gles2_bcm.c
EXE=gles2_bcm.exe
ifeq (0,$(shell pkg-config --exists bcm_host; echo $$?))
  DEPENDENCIES=`pkg-config --cflags --libs bcm_host brcmegl brcmglesv2`
else
  # ubuntu does not have .pc files for Broadcom libraries
  DEPENDENCIES=-lbcm_host -lbrcmEGL -lbrcmGLESv2
endif

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	ssh -t ${PI} ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile
	"${CC}" -Os -s -Wall -Wextra -lm ${DEPENDENCIES} -o $@ $<
