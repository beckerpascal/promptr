# File: makefile

include makefile.defs

all: praatfiles

praatfiles: libpraat.a
	cp --reply=yes libpraat.a ../
	cp --reply=yes library/PraatSound.h ../

libpraat.a:
	cd GSL; make
	cd num; make
	cd num/glpk; make
	cd kar; make
	cd audio; make
	cd mp3; make
	cd FLAC; make
	cd sys; make
	cd stat; make
	cd fon; make
	cd dwsys; make
	cd dwtools; make
	cd LPC; make
	cd FFNet; make
	cd artsynth; make
	cd library; make
	rm -f libpraat.a
	ar r libpraat.a `find num glpk audio stat LPC FFNet dwtools artsynth fon stat dwsys sys GSL kar FLAC mp3 library -name "*.o"`

clean:
	rm -f libpraat.a
	find . -type f -name "*.a" -exec rm -f {} \;
	find . -type f -name "*.o" -exec rm -f {} \;

distclean:
	find . -type f -name "*.o" -exec rm -f {} \;
