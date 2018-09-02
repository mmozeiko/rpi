.PHONY=all run clean

PI=pi@raspberrypi.local

SRC=gles2_bcm.c
EXE=gles2_bcm.exe
DEPENDENCIES=`pkg-config --cflags --libs bcm_host brcmegl brcmglesv2`

all: ${EXE}

run: ${EXE}
	scp ${EXE} ${PI}:
	ssh -t ${PI} ./${EXE}

clean:
	rm -f ${EXE}

${EXE}: ${SRC} makefile
	"${CC}" -Os -s -Wall -Wextra -lm ${DEPENDENCIES} -o $@ $<
