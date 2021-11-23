# test prog
HDRTest:
	g++ -o HDRReadTest -I . HDRReadTest.cpp Convolver.cpp

clean:
	rm HDRReadTest
