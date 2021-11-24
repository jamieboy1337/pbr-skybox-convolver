# test prog
# build for hdr

# https://stackoverflow.com/questions/714100/os-detecting-makefile
ifeq ($(OS), Windows_NT)
	os := WIN
else
	os := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif


GCCFLAGS = -g -Wall -I ./include
CLFLAGS = /EHsc /W3



all: HDRTest

HDRTest: HDR
	g++ $(GCCFLAGS) -o HDRReadTest HDR.o test/HDRReadTest.cpp

HDR:
	g++ $(GCCFLAGS) -c -o HDR.o src/HDR.cpp

clean:
	rm HDRReadTest HDR.o
