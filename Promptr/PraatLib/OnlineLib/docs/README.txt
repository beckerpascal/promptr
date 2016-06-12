PraatLib v0.3
-------------

PraatLib is a C++ library wrapper to Praat, a prosodic feature calculation toolkit. PraatLib allows to calculate a number of prosodic features and takes care of interfeature depencies. This means, if a feature depends on another, it tries to make sure that each features is only calculates once to save CPU time. So the main strength of the library lies in calculating a set of features at once. Praatlib currently supports over 70 different features, which are thoroughly documented in the Praat help. A technical documentation is provided in the file praatlib-0.3.pdf in this directory. 

Version 0.3 of the library is now based on Praat 5.0.30 (July 22nd, 2008).
For more information on Praat see: http://www.fon.hum.uva.nl/praat/

LICENSE
-------

PraatLib has been developed at International Computer Science Institute (ICSI) in Berkeley, USA and Deutsches Forschungszentrum fuer Kuenstliche Intelligenz (DFKI) in Saarbruecken, Germany. 

Praat has been developed by Paul Boersma and David Weenik at the University of Amsterdam, The Netherlands. Praat is released under GNU GPL v2. As a result, PraatLib is released under the same license. 

Main contributors:
------------------
Michael Feld, DFKI
Gerald Friedland, ICSI
Christian Mueller, DFKI 
Benoit Favre, ICSI
JoanIsaac Biel, ICSI

Contact for questions, feedback, etc:
Michael.Feld@dfki.de
fractor@icsi.berkeley.edu
