all: PraatSound.o

CFLAGS = -c -DMAKEPRAATLIB -I ../num -I ../sys -I ../fon -I ../stat

PraatSound.o: PraatSound.h PraatSound.cpp
	g++ $(CFLAGS) PraatSound.cpp


