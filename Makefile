# test prog
# build for hdr

GCCFLAGS = -g -Wall -I ./include
CLFLAGS = /EHsc /W3

all: HDRTest

HDRTest: HDR
	g++ $(GCCFLAGS) -o HDRReadTest HDR.a test/HDRReadTest.cpp

HDR:
	g++ $(GCCFLAGS) -c -o HDR.a src/HDR.cpp

clean:
	rm HDRReadTest HDR.a
