# File: makefile

all: static

dynamic: scriptmain.cpp scriptmain.h
	g++ -o praatsample scriptmain.cpp -L .. -L /usr/local/lib -L /usr/X11R6/lib -lXm -lXmu -lXft -lXrender -ljpeg -lpng12 -lfontconfig -lfreetype -lz -lexpat -lXp -lXt -lSM -lICE -lXext -lX11 -lm -lpthread -lasound -ldl -lc -lpraat

static: scriptmain.cpp scriptmain.h
	g++ -o praatsample scriptmain.cpp ../libpraat.a -L /usr/local/lib -L /usr/X11R6/lib -static -lXm -lXmu -lXft -lXrender -ljpeg -lpng12 -lfontconfig -lfreetype -lz -lexpat -lXp -lXt -lSM -lICE -lXext -lX11 -lm -lpthread -lasound -ldl -lc

clean:
	rm -f praatsample
