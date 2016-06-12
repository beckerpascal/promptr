/* melder.c
 *
 * Copyright (C) 1992-2008 Paul Boersma
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
 * pb 2002/03/13 Mach
 * pb 2002/12/11 MelderInfo
 * pb 2003/12/29 Melder_warning: added XMapRaised because delete response is UNMAP
 * pb 2004/04/06 motif_information drains text window only, i.e. no longer updates all windows
                 (which used to cause up to seven seconds of delay in a 1-second sound window)
 * pb 2004/10/24 info buffer can grow
 * pb 2004/11/28 author notification in Melder_fatal
 * pb 2005/03/04 number and string comparisons, including regular expressions
 * pb 2005/06/16 removed enums from number and string comparisons (ints give no compiler warnings)
 * pb 2005/07/19 Melder_stringMatchesCriterion: regard NULL criterion as empty string
 * pb 2007/05/24 more wchar_t
 * pb 2007/05/26 Melder_stringMatchesCriterionW
 * pb 2007/06/19 removed some
 * pb 2007/08/12 wchar_t in helpProc
 * pb 2007/12/02 enums
 * pb 2007/12/13 Melder_writeToConsole
 * pb 2007/12/18 Gui
 * sdk 2008/01/22 GTK
 */

#include <math.h>
#include <time.h>
#include <ctype.h>
#include "melder.h"
#include "longchar.h"
#include "regularExp.h"
#ifdef _WIN32
	#include <windows.h>
#endif
#if defined (macintosh)
	#include "macport_on.h"
	//#include <Sound.h>
	#include "macport_off.h"
#endif
#ifndef CONSOLE_APPLICATION
	#include "Graphics.h"
	#include "machine.h"
	#ifdef macintosh
		#include "macport_on.h"
		//#include <Events.h>
		#include <Dialogs.h>
		#include <MacErrors.h>
		#include "macport_off.h"
	#endif
	#include "Gui.h"
#endif

#include "enums_getText.h"
#include "melder_enums.h"
#include "enums_getValue.h"
#include "melder_enums.h"

/********** Exported variables. **********/

int Melder_batch;   /* Don't we have a GUI?- Set once at application start-up. */
int Melder_backgrounding;   /* Are we running a script?- Set and unset dynamically. */
char Melder_buffer1 [30001], Melder_buffer2 [30001];
unsigned long Melder_systemVersion;

#ifndef CONSOLE_APPLICATION
	void *Melder_appContext;   /* XtAppContext* */
	void *Melder_topShell;   /* Widget */
#endif

static bool defaultPause (const wchar_t *message) {
	fprintf (stderr, "Pause: %s\nPress 'q' to stop, or any other key to continue.\n", Melder_peekWcsToUtf8 (message));
	int key = getc (stdin);
	return key != 'q' && key != 'Q';
}

static void defaultHelp (const wchar_t *query) {
	Melder_error3 (L"Do not know how to find help on \"", query, L"\".");
	Melder_flushError (NULL);
}

static void defaultSearch (void) {
	Melder_flushError ("Do not know how to search.");
}

static void defaultWarning (wchar_t *message) {
	Melder_writeToConsole (L"Warning: ", true);
	Melder_writeToConsole (message, true);
	Melder_writeToConsole (L"\n", true);
}

static void defaultFatal (wchar_t *message) {
	Melder_writeToConsole (L"Fatal error: ", true);
	Melder_writeToConsole (message, true);
	Melder_writeToConsole (L"\n", true);
}

static int defaultPublish (void *anything) {
	(void) anything;
	return 0;   /* Nothing published. */
}

static int defaultRecord (double duration) {
	(void) duration;
	return 0;   /* Nothing recorded. */
}

static int defaultRecordFromFile (MelderFile file) {
	(void) file;
	return 0;   /* Nothing recorded. */
}

static void defaultPlay (void) {}

static void defaultPlayReverse (void) {}

static int defaultPublishPlayed (void) {
	return 0;   /* Nothing published. */
}

/********** Current message methods: initialize to default (batch) behaviour. **********/

static struct {
	bool (*pause) (const wchar_t *message);
	void (*help) (const wchar_t *query);
	void (*search) (void);
	void (*warning) (wchar_t *message);
	void (*fatal) (wchar_t *message);
	int (*publish) (void *anything);
	int (*record) (double duration);
	int (*recordFromFile) (MelderFile fs);
	void (*play) (void);
	void (*playReverse) (void);
	int (*publishPlayed) (void);
}
	theMelder = {
		defaultPause, defaultHelp, defaultSearch,
		defaultWarning, defaultFatal,
		defaultPublish,
		defaultRecord, defaultRecordFromFile, defaultPlay, defaultPlayReverse, defaultPublishPlayed
	};

/********** CASUAL **********/

void Melder_casual (const char *format, ...) {
	va_list arg;
	va_start (arg, format);
	vsprintf (Melder_buffer1, format, arg);
	#if defined (_WIN32) && ! defined (CONSOLE_APPLICATION)
	if (! Melder_batch) {
		MessageBox (NULL, Melder_peekUtf8ToWcs (Melder_buffer1), L"Casual info", MB_OK);
	} else
	#endif
	fprintf (stderr, "%s\n", Melder_buffer1);
	va_end (arg);
}

/********** PROGRESS **********/

static int theProgressDepth = 0;
void Melder_progressOff (void) { theProgressDepth --; }
void Melder_progressOn (void) { theProgressDepth ++; }

#ifndef CONSOLE_APPLICATION
static int waitWhileProgress (double progress, const wchar_t *message, Widget dia, Widget scale, Widget label1, Widget label2, Widget cancelButton) {
	#if gtk
		// TODO: Something, what?
	#else
	#if defined (macintosh)
	{
		EventRecord event;
		(void) cancelButton;
		// BUG: key events are handled somewhat earlier nowadays, so the next trick does not really ignore key events (and menu commands).
		// Dangerous!
		while (GetNextEvent (keyDownMask, & event)) {
			if ((event.modifiers & cmdKey) && (event.message & charCodeMask) == '.') {
				FlushEvents (everyEvent, 0);
				XtUnmanageChild (dia);
				return 0;
			}
		}
		do { XtNextEvent ((XEvent *) & event); XtDispatchEvent ((XEvent *) & event); } while (event.what);
	}
	#elif defined (_WIN32)
	{
		XEvent event;
		while (PeekMessage (& event, 0, 0, 0, PM_REMOVE)) {
			if (event. message == WM_KEYDOWN) {
				/*
				 * Ignore all key-down messages, except Escape.
				 */
				if (LOWORD (event. wParam) == VK_ESCAPE) {
					XtUnmanageChild (dia);
					return 0;
				}
			} else if (event. message == WM_LBUTTONDOWN) {
				/*
				 * Ignore all mouse-down messages, except click in Interrupt button.
				 */
				Widget me = (Widget) GetWindowLong (event. hwnd, GWL_USERDATA);
				if (me == cancelButton) {
					XtUnmanageChild (dia);
					return 0;
				}
			} else if (event. message != WM_SYSKEYDOWN) {
				/*
				 * Process paint messages etc.
				 */
				DispatchMessage (& event);
			}
		}
	}
	#else
	{
		XEvent event;
		if (XCheckTypedWindowEvent (XtDisplay (cancelButton), XtWindow (cancelButton), ButtonPress, & event)) {
			XtUnmanageChild (dia);
			return 0;
		}
	}
	#endif
	if (progress >= 1.0) {
		GuiObject_hide (dia);
	} else {
		if (progress <= 0.0) progress = 0.0;
		GuiObject_show (dia);   // TODO: prevent raising to the front
		wchar_t *newline = wcschr (message, '\n');
		if (newline != NULL) {
			static MelderString buffer = { 0 };
			MelderString_copy (& buffer, message);
			buffer.string [newline - message] = '\0';
			GuiLabel_setString (label1, buffer.string);
			buffer.string [newline - message] = '\n';
			GuiLabel_setString (label2, buffer.string + (newline - message) + 1);
		} else {
			GuiLabel_setString (label1, message);
			GuiLabel_setString (label2, L"");
		}
		XmScaleSetValue (scale, floor (progress * 1000.0));
		XmUpdateDisplay (dia);
	}
	#endif
	return 1;
}
#endif

static int _Melder_progress (double progress, const wchar_t *message) {
	(void) progress;
	#ifndef CONSOLE_APPLICATION
	if (! Melder_batch && theProgressDepth >= 0 && Melder_debug != 14) {
		static clock_t lastTime;
		static Widget dia = NULL, scale = NULL, label1 = NULL, label2 = NULL, cancelButton = NULL;
		clock_t now = clock ();
		if (progress <= 0.0 || progress >= 1.0 ||
			now - lastTime > CLOCKS_PER_SEC / 4)   /* This time step must be much longer than the null-event waiting time. */
		{
			if (dia == NULL) {
				dia = GuiDialog_create (Melder_topShell, 200, 100, Gui_AUTOMATIC, Gui_AUTOMATIC,
					L"Work in progress", NULL, NULL, 0);

				#if gtk
					Widget form = GTK_DIALOG (dia) -> vbox;
					Widget buttons = GTK_DIALOG (dia) -> action_area;
				#elif motif
					Widget form = dia;    /* TODO: Kan dit ook met een define? */
					Widget buttons = dia;
				#endif

				label1 = GuiLabel_createShown (form, 3, 403, 0, Gui_AUTOMATIC, L"label1", 0);
				label2 = GuiLabel_createShown (form, 3, 403, 30, Gui_AUTOMATIC, L"label2", 0);
				#if gtk
					// TODO: Progressbar ofzo?
				#elif motif
					scale = XmCreateScale (dia, "scale", NULL, 0);
					XtVaSetValues (scale, XmNy, 70, XmNwidth, 400, XmNminimum, 0, XmNmaximum, 1000,
						XmNorientation, XmHORIZONTAL,
						#if ! defined (macintosh)
							XmNscaleHeight, 20,
						#endif
						NULL);
					GuiObject_show (scale);
					#if ! defined (macintosh)
						cancelButton = GuiButton_createShown (buttons, 0, 400, 170, Gui_AUTOMATIC,
							L"Interrupt", NULL, NULL, 0);
					#endif
				#endif
			}
			bool interruption = waitWhileProgress (progress, message, dia, scale, label1, label2, cancelButton);
			if (! interruption) Melder_error1 (L"Interrupted!");
			lastTime = now;
			return interruption;
		}
	}
	#endif
	return 1;   /* Proceed. */
}

static MelderString theProgressBuffer = { 0 };

int Melder_progress1 (double progress, const wchar_t *s1) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append1 (& theProgressBuffer, s1);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress2 (double progress, const wchar_t *s1, const wchar_t *s2) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append2 (& theProgressBuffer, s1, s2);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress3 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append3 (& theProgressBuffer, s1, s2, s3);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress4 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append4 (& theProgressBuffer, s1, s2, s3, s4);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress5 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append5 (& theProgressBuffer, s1, s2, s3, s4, s5);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress6 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append6 (& theProgressBuffer, s1, s2, s3, s4, s5, s6);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress7 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append7 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress8 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append8 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8);
	return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress9 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8, const wchar_t *s9) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append9 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8, s9);
	return _Melder_progress (progress, theProgressBuffer.string);
}

static void * _Melder_monitor (double progress, const wchar_t *message) {
	(void) progress;
	#ifndef CONSOLE_APPLICATION
	if (! Melder_batch && theProgressDepth >= 0) {
		static clock_t lastTime;
		static Widget dia = NULL, scale = NULL, label1 = NULL, label2 = NULL, cancelButton = NULL, drawingArea = NULL;
		clock_t now = clock ();
		static Any graphics = NULL;
		if (progress <= 0.0 || progress >= 1.0 ||
			now - lastTime > CLOCKS_PER_SEC / 4)   /* This time step must be much longer than the null-event waiting time. */
		{
			if (dia == NULL) {
				dia = GuiDialog_create (Melder_topShell, 200, 100, Gui_AUTOMATIC, Gui_AUTOMATIC,
					L"Work in progress", NULL, NULL, 0);

                #if gtk
                        Widget form = GTK_DIALOG (dia) -> vbox;
                        Widget buttons = GTK_DIALOG (dia) -> action_area;
                #elif motif
                        Widget form = dia;    /* TODO: Kan dit ook met een define? */
                        Widget buttons = dia;
                #endif


				label1 = GuiLabel_createShown (dia, 3, 403, 0, Gui_AUTOMATIC, L"label1", 0);
				label2 = GuiLabel_createShown (dia, 3, 403, 30, Gui_AUTOMATIC, L"label2", 0);

				#if gtk
					// TODO: Wat hier?
				#elif motif
					scale = XmCreateScale (dia, "scale", NULL, 0);
					XtVaSetValues (scale, XmNy, 70, XmNwidth, 400, XmNminimum, 0, XmNmaximum, 1000,
						XmNorientation, XmHORIZONTAL,
						#if ! defined (macintosh) && ! defined (_WIN32)
							XmNscaleHeight, 20,
						#endif
						NULL);
					GuiObject_show (scale);
					#if ! defined (macintosh)
						cancelButton = GuiButton_createShown (dia, 0, 400, 170, Gui_AUTOMATIC,
							L"Interrupt", NULL, NULL, 0);
					#endif
				#endif
				drawingArea = GuiDrawingArea_createShown (dia, 0, 400, 230, 430, NULL, NULL, NULL, NULL, NULL, 0);
				GuiObject_show (dia);
				graphics = Graphics_create_xmdrawingarea (drawingArea);
			}
			bool interruption = waitWhileProgress (progress, message, dia, scale, label1, label2, cancelButton);
			if (! interruption) Melder_error1 (L"Interrupted!");
			lastTime = now;
			if (progress == 0.0)
				return graphics;
			if (! interruption) return NULL;
		}
	}
	#endif
	return progress <= 0.0 ? NULL /* No Graphics. */ : & progress /* Any non-NULL pointer. */;
}

void * Melder_monitor1 (double progress, const wchar_t *s1) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append1 (& theProgressBuffer, s1);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor2 (double progress, const wchar_t *s1, const wchar_t *s2) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append2 (& theProgressBuffer, s1, s2);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor3 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append3 (& theProgressBuffer, s1, s2, s3);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor4 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append4 (& theProgressBuffer, s1, s2, s3, s4);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor5 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append5 (& theProgressBuffer, s1, s2, s3, s4, s5);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor6 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append6 (& theProgressBuffer, s1, s2, s3, s4, s5, s6);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor7 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append7 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor8 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append8 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8);
	return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor9 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8, const wchar_t *s9) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append9 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8, s9);
	return _Melder_monitor (progress, theProgressBuffer.string);
}

/********** PAUSE **********/

bool Melder_pause (const wchar_t *message) {
	return theMelder. pause (message);
}

/********** NUMBER AND STRING COMPARISONS **********/

int Melder_numberMatchesCriterion (double value, int which_kMelder_number, double criterion) {
	return
		(which_kMelder_number == kMelder_number_EQUAL_TO && value == criterion) ||
		(which_kMelder_number == kMelder_number_NOT_EQUAL_TO && value != criterion) ||
		(which_kMelder_number == kMelder_number_LESS_THAN && value < criterion) ||
		(which_kMelder_number == kMelder_number_LESS_THAN_OR_EQUAL_TO && value <= criterion) ||
		(which_kMelder_number == kMelder_number_GREATER_THAN && value > criterion) ||
		(which_kMelder_number == kMelder_number_GREATER_THAN_OR_EQUAL_TO && value >= criterion);
}

int Melder_stringMatchesCriterion (const wchar_t *value, int which_kMelder_string, const wchar_t *criterion) {
	if (value == NULL) {
		value = L"";   /* Regard null strings as empty strings, as is usual in Praat. */
	}
	if (criterion == NULL) {
		criterion = L"";   /* Regard null strings as empty strings, as is usual in Praat. */
	}
	if (which_kMelder_string <= kMelder_string_NOT_EQUAL_TO) {
		int matchPositiveCriterion = wcsequ (value, criterion);
		return (which_kMelder_string == kMelder_string_EQUAL_TO) == matchPositiveCriterion;
	}
	if (which_kMelder_string <= kMelder_string_DOES_NOT_CONTAIN) {
		int matchPositiveCriterion = wcsstr (value, criterion) != NULL;
		return (which_kMelder_string == kMelder_string_CONTAINS) == matchPositiveCriterion;
	}
	if (which_kMelder_string <= kMelder_string_DOES_NOT_START_WITH) {
		int matchPositiveCriterion = wcsnequ (value, criterion, wcslen (criterion));
		return (which_kMelder_string == kMelder_string_STARTS_WITH) == matchPositiveCriterion;
	}
	if (which_kMelder_string <= kMelder_string_DOES_NOT_END_WITH) {
		int criterionLength = wcslen (criterion), valueLength = wcslen (value);
		int matchPositiveCriterion = criterionLength <= valueLength && wcsequ (value + valueLength - criterionLength, criterion);
		return (which_kMelder_string == kMelder_string_ENDS_WITH) == matchPositiveCriterion;
	}
	if (which_kMelder_string == kMelder_string_MATCH_REGEXP) {
		char *place = NULL, *errorMessage;
		regexp *compiled_regexp = CompileRE (Melder_peekWcsToUtf8 (criterion), & errorMessage, 0);
		if (compiled_regexp == NULL) return FALSE;
		if (ExecRE (compiled_regexp, NULL, Melder_peekWcsToUtf8 (value), NULL, 0, '\0', '\0', NULL, NULL, NULL))
			place = compiled_regexp -> startp [0];
		free (compiled_regexp);
		return place != NULL;
	}
	return 0;   /* Should not occur. */
}

void Melder_help (const wchar_t *query) {
	theMelder. help (query);
}

void Melder_search (void) {
	theMelder. search ();
}

/********** WARNING **********/

static int theWarningDepth = 0;
void Melder_warningOff (void) { theWarningDepth --; }
void Melder_warningOn (void) { theWarningDepth ++; }

static MelderString theWarningBuffer = { 0 };

void Melder_warning1 (const wchar_t *s1) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append1 (& theWarningBuffer, s1);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning2 (const wchar_t *s1, const wchar_t *s2) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append2 (& theWarningBuffer, s1, s2);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning3 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append3 (& theWarningBuffer, s1, s2, s3);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning4 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append4 (& theWarningBuffer, s1, s2, s3, s4);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning5 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append5 (& theWarningBuffer, s1, s2, s3, s4, s5);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning6 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append6 (& theWarningBuffer, s1, s2, s3, s4, s5, s6);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning7 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append7 (& theWarningBuffer, s1, s2, s3, s4, s5, s6, s7);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning8 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append8 (& theWarningBuffer, s1, s2, s3, s4, s5, s6, s7, s8);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning9 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8, const wchar_t *s9) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append9 (& theWarningBuffer, s1, s2, s3, s4, s5, s6, s7, s8, s9);
	theMelder. warning (theWarningBuffer.string);
}

void Melder_beep (void) {
	#ifdef macintosh
		SysBeep (0);
	#else
		fprintf (stderr, "\a");
	#endif
}

/*********** FATAL **********/

int Melder_fatal (const char *format, ...) {
	const char *lead = strstr (format, "Praat cannot start up") ? "" :
		"Praat will crash. Notify the author (paul.boersma@uva.nl) with the following information:\n";
	va_list arg;
	va_start (arg, format);
	strcpy (Melder_buffer1, lead);
	vsprintf (Melder_buffer1 + strlen (lead), format, arg);
	theMelder. fatal (Melder_peekUtf8ToWcs (Melder_buffer1));
	va_end (arg);
	abort ();
	return 0;   /* Make some compilers happy, some unhappy. */
}

int _Melder_assert (const char *condition, const char *fileName, int lineNumber) {
	return Melder_fatal ("Assertion failed in file \"%s\" at line %d:\n   %s\n",
		fileName, lineNumber, condition);
}

#ifndef CONSOLE_APPLICATION
static bool pause_continued, pause_stopped;
static void gui_button_cb_continue (I, GuiButtonEvent event) { (void) event; (void) void_me; pause_continued = true; }
static void gui_button_cb_stop (I, GuiButtonEvent event) { (void) event; (void) void_me; pause_stopped = true; }

#if gtk
static void gtk_error (wchar_t *message) {
	Widget dialog = gtk_message_dialog_new (GTK_WINDOW(Melder_topShell),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 Melder_peekWcsToUtf8 (message));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void gtk_warning (wchar_t *message) {
	Widget dialog = gtk_message_dialog_new (GTK_WINDOW(Melder_topShell),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_OK,
					 Melder_peekWcsToUtf8 (message));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}
#endif

#if motif
static bool motif_pause (const wchar_t *message) {
	static Widget dia = NULL, continueButton = NULL, stopButton = NULL, rc, buttons, text;
	if (dia == NULL) {
		dia = GuiDialog_create (Melder_topShell, 200, 100, Gui_AUTOMATIC, Gui_AUTOMATIC, L"Pause", NULL, NULL, 0);
		rc = XmCreateRowColumn (dia, "rc", NULL, 0);
		text = GuiLabel_createShown (rc, 3, 403, Gui_AUTOMATIC, Gui_AUTOMATIC, L"text", 0);
		buttons = XmCreateRowColumn (rc, "rc", NULL, 0);
		XtVaSetValues (buttons, XmNorientation, XmHORIZONTAL, NULL);
		continueButton = GuiButton_createShown (buttons, 10, 310, Gui_AUTOMATIC, Gui_AUTOMATIC,
			L"Continue", gui_button_cb_continue, dia, 0);
		stopButton = GuiButton_createShown (buttons, 320, 380, Gui_AUTOMATIC, Gui_AUTOMATIC,
			L"Stop", gui_button_cb_stop, dia, 0);
		XtManageChild (buttons);
		XtManageChild (rc);
	}
	if (! message) message = L"";
	GuiLabel_setString (text, message);
	GuiDialog_show (dia);
	pause_continued = pause_stopped = FALSE;
	do {
		XEvent event;
		XtAppNextEvent (Melder_appContext, & event);
		XtDispatchEvent (& event);
	} while (! pause_continued && ! pause_stopped);
	GuiObject_hide (dia);
	return pause_continued;
}
#endif

#ifdef macintosh
static void motif_fatal (wchar_t *message) {
	DialogRef dialog;
	static UniChar messageU [2000+1];
	int messageLength = wcslen (message);
	for (int i = 0; i < messageLength; i ++) {
		messageU [i] = message [i];   // BUG: should convert to UTF16
	}
	CFStringRef messageCF = CFStringCreateWithCharacters (NULL, messageU, messageLength);
	CreateStandardAlert (kAlertStopAlert, messageCF, NULL, NULL, & dialog);
	CFRelease (messageCF);
	RunStandardAlert (dialog, NULL, NULL);
	SysError (11);
}
static void motif_error (wchar_t *messageW) {
	DialogRef dialog;
	static UniChar messageU [2000+1];
	int messageLength = wcslen (messageW);
	for (int i = 0; i < messageLength; i ++) {
		messageU [i] = messageW [i];   // BUG: should convert to UTF16
	}
	CFStringRef messageCF = CFStringCreateWithCharacters (NULL, messageU, messageLength);
	CreateStandardAlert (kAlertStopAlert, messageCF, NULL, NULL, & dialog);
	CFRelease (messageCF);
	RunStandardAlert (dialog, NULL, NULL);
	XmUpdateDisplay (0);
}
static void motif_warning (wchar_t *messageW) {
	DialogRef dialog;
	static UniChar messageU [2000+1];
	int messageLength = wcslen (messageW);
	for (int i = 0; i < messageLength; i ++) {
		messageU [i] = messageW [i];   // BUG: should convert to UTF16
	}
	CFStringRef messageCF = CFStringCreateWithCharacters (NULL, messageU, messageLength);
	CreateStandardAlert (kAlertNoteAlert, messageCF, NULL, NULL, & dialog);
	CFRelease (messageCF);
	RunStandardAlert (dialog, NULL, NULL);
	XmUpdateDisplay (0);
}
#elif defined (_WIN32)
static void motif_fatal (wchar_t *message) {
	MessageBox (NULL, message, L"Fatal error", MB_OK);
}
static void motif_error (wchar_t *message) {
	MessageBox (NULL, message, L"Message", MB_OK);
}
static void motif_warning (wchar_t *message) {
	MessageBox (NULL, message, L"Warning", MB_OK);
}
#elif motif
static Widget makeMessage (unsigned char dialogType, const char *resourceName, const char *title) {
	Arg arg [1];
	arg [0]. name = XmNautoUnmanage; arg [0]. value = True;
	Widget dialog = XmCreateMessageDialog (Melder_topShell, (char *) resourceName, arg, 1);
	XtVaSetValues (dialog,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XmNdialogType, dialogType,
		NULL);
	XtVaSetValues (XtParent (dialog), XmNtitle, title, XmNdeleteResponse, XmUNMAP, NULL);
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	return dialog;
}
static void motif_error (wchar_t *messageW) {
	static Widget dia = NULL;
	static char messageA [2000+1];
	int messageLength = wcslen (messageW);
	if (dia == NULL)
		dia = makeMessage (XmDIALOG_ERROR, "error", "Message");
	for (int i = 0; i <= messageLength; i ++) {
		messageA [i] = messageW [i];
	}
	XtVaSetValues (dia, motif_argXmString (XmNmessageString, messageA), NULL);
	XtManageChild (dia);
	XMapRaised (XtDisplay (XtParent (dia)), XtWindow (XtParent (dia)));   /* Because the delete response is UNMAP. */
}
static void motif_warning (wchar_t *message) {
	static Widget dia = NULL;
	if (dia == NULL)
		dia = makeMessage (XmDIALOG_WARNING, "warning", "Warning");
	XtVaSetValues (dia, motif_argXmString (XmNmessageString, Melder_peekWcsToUtf8 (message)), NULL);
	XtManageChild (dia);
	XMapRaised (XtDisplay (XtParent (dia)), XtWindow (XtParent (dia)));   /* Because the delete response is UNMAP. */
}
#endif
#endif

#ifndef CONSOLE_APPLICATION
void MelderGui_create (void *appContext, void *parent) {
	extern void gui_information (wchar_t *);
	Melder_appContext = appContext;
	#if gtk
		Melder_topShell = (Widget) GuiObject_parent (parent);
	#else
		Melder_topShell = (Widget) parent;
	#endif
	Melder_setInformationProc (gui_information);
	#if gtk
		Melder_setWarningProc (gtk_warning);
		Melder_setErrorProc (gtk_error);
	#elif motif
		Melder_setWarningProc (motif_warning);
		Melder_setErrorProc (motif_error);
		#if defined (macintosh) || defined (_WIN32)
			Melder_setFatalProc (motif_fatal);
		#endif
		Melder_setPauseProc (motif_pause);
	#endif
}
#endif

int Melder_publish (void *anything) {
	return theMelder. publish (anything);
}

int Melder_record (double duration) {
	return theMelder. record (duration);
}

int Melder_recordFromFile (MelderFile fs) {
	return theMelder. recordFromFile (fs);
}

void Melder_play (void) {
	theMelder. play ();
}

void Melder_playReverse (void) {
	theMelder. playReverse ();
}

int Melder_publishPlayed (void) {
	return theMelder. publishPlayed ();
}

/********** Procedures to override message methods (e.g., to enforce interactive behaviour). **********/

void Melder_setPauseProc (bool (*pause) (const wchar_t *))
	{ theMelder. pause = pause ? pause : defaultPause; }

void Melder_setHelpProc (void (*help) (const wchar_t *query))
	{ theMelder. help = help ? help : defaultHelp; }

void Melder_setSearchProc (void (*search) (void))
	{ theMelder. search = search ? search : defaultSearch; }

void Melder_setWarningProc (void (*warning) (wchar_t *))
	{ theMelder. warning = warning ? warning : defaultWarning; }

void Melder_setFatalProc (void (*fatal) (wchar_t *))
	{ theMelder. fatal = fatal ? fatal : defaultFatal; }

void Melder_setPublishProc (int (*publish) (void *))
	{ theMelder. publish = publish ? publish : defaultPublish; }

void Melder_setRecordProc (int (*record) (double))
	{ theMelder. record = record ? record : defaultRecord; }

void Melder_setRecordFromFileProc (int (*recordFromFile) (MelderFile))
	{ theMelder. recordFromFile = recordFromFile ? recordFromFile : defaultRecordFromFile; }

void Melder_setPlayProc (void (*play) (void))
	{ theMelder. play = play ? play : defaultPlay; }

void Melder_setPlayReverseProc (void (*playReverse) (void))
	{ theMelder. playReverse = playReverse ? playReverse : defaultPlayReverse; }

void Melder_setPublishPlayedProc (int (*publishPlayed) (void))
	{ theMelder. publishPlayed = publishPlayed ? publishPlayed : defaultPublishPlayed; }

/* End of file melder.c */
