/* praat_statistics.c
 *
 * Copyright (C) 1992-2007 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2002/03/07 GPL
 * pb 2002/03/09 OSX no mention of free memory
 * pb 2004/10/18 more big integers
 * pb 2005/03/02 pref string 260 bytes long
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2006/12/06 MelderString
 * pb 2006/12/26 theCurrentPraat
 * pb 2007/07/27 string deallocation size
 * pb 2007/08/12 wchar_t
 * pb 2007/12/09 preferences
 */

#include <time.h>
#include "praatP.h"

static struct {
	long batchSessions, interactiveSessions;
	double memory;
	wchar_t dateOfFirstSession [Preferences_STRING_BUFFER_SIZE];
} statistics;

void praat_statistics_prefs (void) {
	Preferences_addLong (L"PraatShell.batchSessions", & statistics.batchSessions, 0);
	Preferences_addLong (L"PraatShell.interactiveSessions", & statistics.interactiveSessions, 0);
	Preferences_addDouble (L"PraatShell.memory", & statistics.memory, 0.0);
	Preferences_addString (L"PraatShell.dateOfFirstSession", & statistics.dateOfFirstSession [0], L"");
}

void praat_statistics_prefsChanged (void) {
	if (! statistics.dateOfFirstSession [0]) {
		time_t today = time (NULL);
		wchar_t *newLine;
		wcscpy (statistics.dateOfFirstSession, Melder_peekUtf8ToWcs (ctime (& today)));
		newLine = wcschr (statistics.dateOfFirstSession, '\n');
		if (newLine) *newLine = '\0';
	}
	if (theCurrentPraat -> batch)
		statistics.batchSessions += 1;
	else
		statistics.interactiveSessions += 1;
}

void praat_statistics_exit (void) {
	statistics.memory += Melder_allocationSize ();
}

void praat_memoryInfo (void) {
	MelderInfo_open ();
	MelderInfo_writeLine2 (L"Currently in use:\n"
		L"   Strings: ", Melder_integer (MelderString_allocationCount () - MelderString_deallocationCount ()));
	MelderInfo_writeLine2 (L"   Arrays: ", Melder_integer (NUM_getTotalNumberOfArrays ()));
	MelderInfo_writeLine5 (L"   Things: ", Melder_integer (Thing_getTotalNumberOfThings ()),
		L" (objects in list: ", Melder_integer (theCurrentPraat -> n), L")");
	MelderInfo_writeLine2 (L"   Other: ",
		Melder_bigInteger (Melder_allocationCount () - Melder_deallocationCount ()
			- Thing_getTotalNumberOfThings () - NUM_getTotalNumberOfArrays ()
			- (MelderString_allocationCount () - MelderString_deallocationCount ())));
	MelderInfo_writeLine5 (
		L"\nMemory history of this session:\n"
		L"   Total created: ", Melder_bigInteger (Melder_allocationCount ()), L" (", Melder_bigInteger (Melder_allocationSize ()), L" bytes)");
	MelderInfo_writeLine2 (L"   Total deleted: ", Melder_bigInteger (Melder_deallocationCount ()));
	MelderInfo_writeLine5 (
		L"   Strings created: ", Melder_bigInteger (MelderString_allocationCount ()), L" (", Melder_bigInteger (MelderString_allocationSize ()), L" bytes)");
	MelderInfo_writeLine5 (
		L"   Strings deleted: ", Melder_bigInteger (MelderString_deallocationCount ()), L" (", Melder_bigInteger (MelderString_deallocationSize ()), L" bytes)");
	MelderInfo_writeLine3 (L"\nHistory of all sessions from ", statistics.dateOfFirstSession, L" until today:");
	MelderInfo_writeLine4 (L"   Interactive sessions: ", Melder_integer (statistics.interactiveSessions),
		L" batch sessions: ", Melder_integer (statistics.batchSessions));
	MelderInfo_writeLine3 (L"   Total memory use: ", Melder_bigInteger (statistics.memory + Melder_allocationSize ()), L" bytes");
	MelderInfo_writeLine2 (L"\nNumber of actions: ", Melder_integer (praat_getNumberOfActions ()));
	MelderInfo_close ();
}

/* End of file praat_statistics.c */
