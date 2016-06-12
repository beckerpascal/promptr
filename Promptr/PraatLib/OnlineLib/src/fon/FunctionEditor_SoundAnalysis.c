/* FunctionEditor_SoundAnalysis.c
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
 * pb 2002/07/16 GPL
 * pb 2002/10/11 added a screen text for unavailable pitch
 * pb 2002/11/19 default log file names include "~" on Unix and Mach
 * pb 2002/11/19 added pulses and five separate analysis menus
 * pb 2003/02/19 clearer wording
 * pb 2003/02/24 spectral slices
 * pb 2003/03/03 Extract visible spectrogram: higher degree of oversampling than in editor
 * pb 2003/03/10 undid previous change because our PostScript code now does image interpolation
 * pb 2003/03/12 queriable
 * pb 2003/04/03 Get power at cursor cross
 * pb 2003/04/11 smaller log settings dialog
 * pb 2003/05/18 more shimmer measurements
 * pb 2003/05/20 longestAnalysis replaces the pitch/formant/intensity time steps,
 *               pitch.speckle, and formant.maximumDuration
 * pb 2003/05/21 pitch floor and ceiling replace the separate ranges for viewing and analysis
 * pb 2003/05/27 spectrogram maximum and autoscaling
 * pb 2003/07/20 moved voice report to VoiceAnalysis.c
 * pb 2003/08/23 reintroduced formant.numberOfTimeSteps
 * pb 2003/09/11 make sure that analyses objects can be extracted even before first drawing
 * pb 2003/09/16 advanced pitch settings: pitch.timeStep, pitch.viewFrom, pitch.viewTo, pitch.timeStepsPerView
 * pb 2003/10/01 time step settings: timeStepStrategy, fixedTimeStep, numberOfTimeStepsPerView
 * pb 2003/11/30 Sound_to_Spectrogram_windowShapeText
 * pb 2003/12/09 moved Spectrogram settings back in sync with preferences
 * pb 2004/04/16 introduced pulses.maximumPeriodFactor
 * pb 2004/05/12 extended voice report with mean F0 and harmonic-to-noise ratios
 * pb 2004/07/14 introduced pulses.maximumAmplitudeFactor
 * pb 2004/10/16 C++ compatible struct tags
 * pb 2004/10/24 intensity.averagingMethod
 * pb 2004/10/27 intensity.subtractMeanPressure
 * pb 2004/11/22 simplified Sound_to_Spectrum ()
 * pb 2004/11/28 improved screen text for unavailable pitch
 * pb 2004/11/28 warning in settings dialogs for non-standard time step strategies
 * pb 2005/03/02 all pref string buffers are 260 bytes long
 * pb 2005/03/07 'intensity' logging sensitive to averaging method
 * pb 2005/06/16 units
 * pb 2005/08/18 editor name in log files
 * pb 2006/02/27 more helpful text when analyses are not shown
 * pb 2006/09/12 better messages if analysis not available
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2006/12/10 MelderInfo
 * pb 2006/12/18 better info
 * pb 2007/01/27 changed Vector API
 * pb 2007/06/11 wchar_t
 * pb 2007/09/01 computeXXX procedures are global on behalf of the inheritors
 */

#include <time.h>
#include "FunctionEditor_SoundAnalysis.h"
#include "Sound_and_Spectrogram.h"
#include "Sound_and_Spectrum.h"
#include "Sound_to_Pitch.h"
#include "Sound_to_Intensity.h"
#include "Sound_to_Formant.h"
#include "Pitch_to_PointProcess.h"
#include "VoiceAnalysis.h"
#include "Preferences.h"
#include "EditorM.h"
#include "praat_script.h"

static const wchar_t * theMessage_Cannot_compute_spectrogram = L"The spectrogram is not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_pitch = L"The pitch contour is not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_formant = L"The formants are not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_intensity = L"The intensity curve is not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_pulses = L"The pulses are not defined at the edge of the sound.";

struct logInfo {
	int toInfoWindow, toLogFile;
	wchar_t fileName [Resources_STRING_BUFFER_SIZE], format [Resources_STRING_BUFFER_SIZE];
};

#if defined (macintosh)
	#define LOG_1_FILE_NAME L"~/Desktop/Pitch Log"
	#define LOG_2_FILE_NAME L"~/Desktop/Formant Log"
	#define LOG_3_FILE_NAME L"~/Desktop/Log script 3"
	#define LOG_4_FILE_NAME L"~/Desktop/Log script 4"
#elif defined (WIN32)
	#define LOG_1_FILE_NAME L"C:\\WINDOWS\\DESKTOP\\Pitch Log.txt"
	#define LOG_2_FILE_NAME L"C:\\WINDOWS\\DESKTOP\\Formant Log.txt"
	#define LOG_3_FILE_NAME L"C:\\WINDOWS\\DESKTOP\\Log script 3.praat"
	#define LOG_4_FILE_NAME L"C:\\WINDOWS\\DESKTOP\\Log script 4.praat"
#else
	#define LOG_1_FILE_NAME L"~/pitch_log"
	#define LOG_2_FILE_NAME L"~/formant_log"
	#define LOG_3_FILE_NAME L"~/log_script3"
	#define LOG_4_FILE_NAME L"~/log_script4"
#endif
#define LOG_1_FORMAT  L"Time 'time:6' seconds, pitch 'f0:2' Hertz"
#define LOG_2_FORMAT  L"'t1:4''tab$''t2:4''tab$''f1:0''tab$''f2:0''tab$''f3:0'"
static struct {
	double longestAnalysis;
	int timeStepStrategy; double fixedTimeStep; long numberOfTimeStepsPerView;
	struct FunctionEditor_spectrogram spectrogram;
	struct FunctionEditor_pitch pitch;
	struct FunctionEditor_intensity intensity;
	struct FunctionEditor_formant formant;
	struct FunctionEditor_pulses pulses;
	struct logInfo log [2];
	wchar_t logScript3 [Resources_STRING_BUFFER_SIZE], logScript4 [Resources_STRING_BUFFER_SIZE];
} preferences = {
	10.0,
	1 /* automatic time step */, 0.01 /* in case of a fixed step */, 100 /* in case of a fixed number per view */,
	{
		/* Spectrogram settings: */
		NULL /* data */, TRUE /* show */,
		0.0, 5000.0, /* Hertz */
		0.005 /* seconds */,
		70.0 /* dB */,
		/* Advanced spectrogram settings: */
		1000, 250,
		1 /* Fourier */,
		5 /* Gaussian */,
		TRUE /* autoscaling on */,
		100.0 /* dB/Hz */,
		6.0 /* dB/octave */,
		0.0 /* no dynamic compression */,
		/* Dynamic information: */
		0.0
	}, {
		NULL, TRUE,
		/* Pitch settings: */
		75.0, 500.0, Pitch_UNIT_HERTZ, FunctionEditor_pitch_DRAWING_METHOD_AUTOMATIC,
		/* Advanced pitch settings: */
		0.0 /* auto view from */, 0.0 /* auto view to */,
		1, FALSE,
		15, 0.03, 0.45,
		0.01, 0.35, 0.14,
		{ false }
	}, {
		NULL, FALSE,
		/* Intensity settings: */
		50.0, 100.0, Intensity_averaging_ENERGY, TRUE
	}, {
		NULL, TRUE,
		5500.0, 10,
		0.025,
		30.0, 1.0,
		1, 50.0,
	}, {
		/* Pulses settings: */
		NULL, TRUE,
		1.3, /* maximum period factor */
		1.6 /* maximum amplitude factor */
	}, {
		{ TRUE, TRUE, LOG_1_FILE_NAME, LOG_1_FORMAT },
		{ TRUE, TRUE, LOG_2_FILE_NAME, LOG_2_FORMAT }
	},
		LOG_3_FILE_NAME, LOG_4_FILE_NAME
};

void FunctionEditor_SoundAnalysis_prefs (void) {
	Resources_addDouble (L"FunctionEditor.longestAnalysis", & preferences.longestAnalysis);
	Resources_addInt (L"FunctionEditor.timeStepStrategy", & preferences.timeStepStrategy);
	Resources_addDouble (L"FunctionEditor.fixedTimeStep", & preferences.fixedTimeStep);
	Resources_addLong (L"FunctionEditor.numberOfTimeStepsPerView", & preferences.numberOfTimeStepsPerView);
	Resources_addInt (L"FunctionEditor.spectrogram.show", & preferences.spectrogram.show);
	Resources_addLong (L"FunctionEditor.spectrogram.timeSteps2", & preferences.spectrogram.timeSteps);
	Resources_addLong (L"FunctionEditor.spectrogram.frequencySteps2", & preferences.spectrogram.frequencySteps);
	Resources_addDouble (L"FunctionEditor.spectrogram.viewFrom2", & preferences.spectrogram.viewFrom);
	Resources_addDouble (L"FunctionEditor.spectrogram.viewTo2", & preferences.spectrogram.viewTo);
	Resources_addInt (L"FunctionEditor.spectrogram.method2", & preferences.spectrogram.method);
	Resources_addDouble (L"FunctionEditor.spectrogram.windowLength2", & preferences.spectrogram.windowLength);
	Resources_addInt (L"FunctionEditor.spectrogram.windowShape2", & preferences.spectrogram.windowShape);
	Resources_addDouble (L"FunctionEditor.spectrogram.maximum2", & preferences.spectrogram.maximum);
	Resources_addInt (L"FunctionEditor.spectrogram.autoscaling2", & preferences.spectrogram.autoscaling);
	Resources_addDouble (L"FunctionEditor.spectrogram.dynamicRange2", & preferences.spectrogram.dynamicRange);
	Resources_addDouble (L"FunctionEditor.spectrogram.preemphasis2", & preferences.spectrogram.preemphasis);
	Resources_addDouble (L"FunctionEditor.spectrogram.dynamicCompression2", & preferences.spectrogram.dynamicCompression);
	Resources_addInt (L"FunctionEditor.pitch.show", & preferences.pitch.show);
	Resources_addDouble (L"FunctionEditor.pitch.floor", & preferences.pitch.floor);
	Resources_addDouble (L"FunctionEditor.pitch.ceiling", & preferences.pitch.ceiling);
	Resources_addInt (L"FunctionEditor.pitch.unit", & preferences.pitch.unit);
	Resources_addInt (L"FunctionEditor.pitch.drawingMethod", & preferences.pitch.drawingMethod);
	Resources_addDouble (L"FunctionEditor.pitch.viewFrom", & preferences.pitch.viewFrom);
	Resources_addDouble (L"FunctionEditor.pitch.viewTo", & preferences.pitch.viewTo);
	Resources_addInt (L"FunctionEditor.pitch.method", & preferences.pitch.method);
	Resources_addInt (L"FunctionEditor.pitch.veryAccurate", & preferences.pitch.veryAccurate);
	Resources_addLong (L"FunctionEditor.pitch.maximumNumberOfCandidates", & preferences.pitch.maximumNumberOfCandidates);
	Resources_addDouble (L"FunctionEditor.pitch.silenceThreshold", & preferences.pitch.silenceThreshold);
	Resources_addDouble (L"FunctionEditor.pitch.voicingThreshold", & preferences.pitch.voicingThreshold);
	Resources_addDouble (L"FunctionEditor.pitch.octaveCost", & preferences.pitch.octaveCost);
	Resources_addDouble (L"FunctionEditor.pitch.octaveJumpCost", & preferences.pitch.octaveJumpCost);
	Resources_addDouble (L"FunctionEditor.pitch.voicedUnvoicedCost", & preferences.pitch.voicedUnvoicedCost);
	Resources_addInt (L"FunctionEditor.intensity.show", & preferences.intensity.show);
	Resources_addDouble (L"FunctionEditor.intensity.viewFrom", & preferences.intensity.viewFrom);
	Resources_addDouble (L"FunctionEditor.intensity.viewTo", & preferences.intensity.viewTo);
	Resources_addInt (L"FunctionEditor.intensity.averagingMethod", & preferences.intensity.averagingMethod);
	Resources_addInt (L"FunctionEditor.intensity.subtractMeanPressure", & preferences.intensity.subtractMeanPressure);
	Resources_addInt (L"FunctionEditor.formant.show", & preferences.formant.show);
	Resources_addDouble (L"FunctionEditor.formant.maximumFormant", & preferences.formant.maximumFormant);
	Resources_addLong (L"FunctionEditor.formant.numberOfPoles", & preferences.formant.numberOfPoles);
	Resources_addDouble (L"FunctionEditor.formant.windowLength", & preferences.formant.windowLength);
	Resources_addDouble (L"FunctionEditor.formant.dynamicRange", & preferences.formant.dynamicRange);
	Resources_addDouble (L"FunctionEditor.formant.dotSize", & preferences.formant.dotSize);
	Resources_addInt (L"FunctionEditor.formant.method", & preferences.formant.method);
	Resources_addDouble (L"FunctionEditor.formant.preemphasisFrom", & preferences.formant.preemphasisFrom);
	Resources_addInt (L"FunctionEditor.pulses.show", & preferences.pulses.show);
	Resources_addDouble (L"FunctionEditor.pulses.maximumPeriodFactor", & preferences.pulses.maximumPeriodFactor);
	Resources_addDouble (L"FunctionEditor.pulses.maximumAmplitudeFactor", & preferences.pulses.maximumAmplitudeFactor);
	Resources_addInt (L"FunctionEditor.log1.toInfoWindow", & preferences.log[0].toInfoWindow);
	Resources_addInt (L"FunctionEditor.log1.toLogFile", & preferences.log[0].toLogFile);
	Resources_addString (L"FunctionEditor.log1.fileName", & preferences.log[0].fileName [0]);
	Resources_addString (L"FunctionEditor.log1.format", & preferences.log[0].format [0]);
	Resources_addInt (L"FunctionEditor.log2.toInfoWindow", & preferences.log[1].toInfoWindow);
	Resources_addInt (L"FunctionEditor.log2.toLogFile", & preferences.log[1].toLogFile);
	Resources_addString (L"FunctionEditor.log2.fileName", & preferences.log[1].fileName [0]);
	Resources_addString (L"FunctionEditor.log2.format", & preferences.log[1].format [0]);
	Resources_addString (L"FunctionEditor.logScript3", & preferences.logScript3 [0]);
	Resources_addString (L"FunctionEditor.logScript4", & preferences.logScript4 [0]);
}

void FunctionEditor_SoundAnalysis_forget (I) {
	iam (FunctionEditor);
	forget (my spectrogram.data);
	forget (my pitch.data);
	forget (my intensity.data);
	forget (my formant.data);
	forget (my pulses.data);
}

static Sound extractSound (FunctionEditor me, double tmin, double tmax) {
	Sound sound = NULL;
	if (my longSound.data) {
		if (tmin < my longSound.data -> xmin) tmin = my longSound.data -> xmin;
		if (tmax > my longSound.data -> xmax) tmax = my longSound.data -> xmax;
		sound = LongSound_extractPart (my longSound.data, tmin, tmax, TRUE);
	} else if (my sound.data) {
		if (tmin < my sound.data -> xmin) tmin = my sound.data -> xmin;
		if (tmax > my sound.data -> xmax) tmax = my sound.data -> xmax;
		sound = Sound_extractPart (my sound.data, tmin, tmax, enumi (Sound_WINDOW, Rectangular), 1.0, TRUE);
	}
	return sound;
}

void FunctionEditor_SoundAnalysis_computeSpectrogram (I) {
	iam (FunctionEditor);
	Melder_progressOff ();
	if (my spectrogram.show && my endWindow - my startWindow <= my longestAnalysis &&
		(my spectrogram.data == NULL || my spectrogram.data -> xmin != my startWindow || my spectrogram.data -> xmax != my endWindow))
	{
		Sound sound = NULL;
		double margin = my spectrogram.windowShape == 5 ? my spectrogram.windowLength :  0.5 * my spectrogram.windowLength;
		forget (my spectrogram.data);
		sound = extractSound (me, my startWindow - margin, my endWindow + margin);
		if (sound != NULL) {
			my spectrogram.data = Sound_to_Spectrogram (sound, my spectrogram.windowLength,
				my spectrogram.viewTo, (my endWindow - my startWindow) / my spectrogram.timeSteps,
				my spectrogram.viewTo / my spectrogram.frequencySteps, my spectrogram.windowShape, 8.0, 8.0);
			if (my spectrogram.data != NULL) my spectrogram.data -> xmin = my startWindow, my spectrogram.data -> xmax = my endWindow;
			else Melder_clearError ();
			forget (sound);
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

static void computePitch_inside (FunctionEditor me) {
	Sound sound = NULL;
	double margin = my pitch.veryAccurate ? 3.0 / my pitch.floor : 1.5 / my pitch.floor;
	forget (my pitch.data);
	sound = extractSound (me, my startWindow - margin, my endWindow + margin);
	if (sound != NULL) {
		double pitchTimeStep =
			my timeStepStrategy == 2 ? my fixedTimeStep :
			my timeStepStrategy == 3 ? (my endWindow - my startWindow) / my numberOfTimeStepsPerView :
			0.0;   /* The default: determined by pitch floor. */
		my pitch.data = Sound_to_Pitch_any (sound, pitchTimeStep,
			my pitch.floor, my pitch.method == 1 ? 3.0 : 1.0, my pitch.maximumNumberOfCandidates,
			(my pitch.method - 1) * 2 + my pitch.veryAccurate,
			my pitch.silenceThreshold, my pitch.voicingThreshold,
			my pitch.octaveCost, my pitch.octaveJumpCost, my pitch.voicedUnvoicedCost, my pitch.ceiling);
		if (my pitch.data != NULL) my pitch.data -> xmin = my startWindow, my pitch.data -> xmax = my endWindow;
		else Melder_clearError ();
		forget (sound);
	} else Melder_clearError ();
}

void FunctionEditor_SoundAnalysis_computePitch (I) {
	iam (FunctionEditor);
	Melder_progressOff ();
	if (my pitch.show && my endWindow - my startWindow <= my longestAnalysis &&
		(my pitch.data == NULL || my pitch.data -> xmin != my startWindow || my pitch.data -> xmax != my endWindow))
	{
		computePitch_inside (me);
	}
	Melder_progressOn ();
}

void FunctionEditor_SoundAnalysis_computeIntensity (I) {
	iam (FunctionEditor);
	Melder_progressOff ();
	if (my intensity.show && my endWindow - my startWindow <= my longestAnalysis &&
		(my intensity.data == NULL || my intensity.data -> xmin != my startWindow || my intensity.data -> xmax != my endWindow))
	{
		Sound sound = NULL;
		double margin = 3.2 / my pitch.floor;
		forget (my intensity.data);
		sound = extractSound (me, my startWindow - margin, my endWindow + margin);
		if (sound != NULL) {
			my intensity.data = Sound_to_Intensity (sound, my pitch.floor,
				my endWindow - my startWindow > my longestAnalysis ? (my endWindow - my startWindow) / 100 : 0.0,
				my intensity.subtractMeanPressure);
			if (my intensity.data != NULL) my intensity.data -> xmin = my startWindow, my intensity.data -> xmax = my endWindow;
			else Melder_clearError ();
			forget (sound);
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

void FunctionEditor_SoundAnalysis_computeFormants (I) {
	iam (FunctionEditor);
	Melder_progressOff ();
	if (my formant.show && my endWindow - my startWindow <= my longestAnalysis &&
		(my formant.data == NULL || my formant.data -> xmin != my startWindow || my formant.data -> xmax != my endWindow))
	{
		Sound sound = NULL;
		double margin = my formant.windowLength;
		forget (my formant.data);
		if (my endWindow - my startWindow > my longestAnalysis)
			sound = extractSound (me,
				0.5 * (my startWindow + my endWindow - my longestAnalysis) - margin,
				0.5 * (my startWindow + my endWindow + my longestAnalysis) + margin);
		else
			sound = extractSound (me, my startWindow - margin, my endWindow + margin);
		if (sound != NULL) {
			double formantTimeStep =
				my timeStepStrategy == 2 ? my fixedTimeStep :
				my timeStepStrategy == 3 ? (my endWindow - my startWindow) / my numberOfTimeStepsPerView :
				0.0;   /* The default: determined by analysis window length. */
			my formant.data = Sound_to_Formant_any (sound, formantTimeStep,
				my formant.numberOfPoles, my formant.maximumFormant,
				my formant.windowLength, my formant.method, my formant.preemphasisFrom, 50.0);
			if (my formant.data != NULL) my formant.data -> xmin = my startWindow, my formant.data -> xmax = my endWindow;
			else Melder_clearError ();
			forget (sound);
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

void FunctionEditor_SoundAnalysis_computePulses (I) {
	iam (FunctionEditor);
	Melder_progressOff ();
	if (my pulses.show && my endWindow - my startWindow <= my longestAnalysis &&
		(my pulses.data == NULL || my pulses.data -> xmin != my startWindow || my pulses.data -> xmax != my endWindow))
	{
		forget (my pulses.data);   /* 20060912 */
		if (my pitch.data == NULL || my pitch.data -> xmin != my startWindow || my pitch.data -> xmax != my endWindow) {
			computePitch_inside (me);
		}
		if (my pitch.data != NULL) {
			Sound sound = NULL;
			/* forget (my pulses.data);   /* 20060912 */
			sound = extractSound (me, my startWindow, my endWindow);
			if (sound != NULL) {
				my pulses.data = Sound_Pitch_to_PointProcess_cc (sound, my pitch.data);
				if (my pulses.data == NULL) Melder_clearError ();
				forget (sound);
			} else Melder_clearError ();
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

#define FunctionEditor_PART_CURSOR  1
#define FunctionEditor_PART_SELECTION  2

static const wchar_t *FunctionEditor_partString (int part) {
	static const wchar_t *strings [] = { L"", L"CURSOR", L"SELECTION" };
	return strings [part];
}

static const wchar_t *FunctionEditor_partString_locative (int part) {
	static const wchar_t *strings [] = { L"", L"at CURSOR", L"in SELECTION" };
	return strings [part];
}

static int makeQueriable (FunctionEditor me, int allowCursor, double *tmin, double *tmax) {
	if (my endWindow - my startWindow > my longestAnalysis) {
		return Melder_error5 (L"Window too long to show analyses. Zoom in to at most ", Melder_half (my longestAnalysis), L" seconds "
			"or set the \"longest analysis\" to at least ", Melder_half (my endWindow - my startWindow), L" seconds.");
	}
	if (my startSelection == my endSelection) {
		if (allowCursor) {
			*tmin = *tmax = my startSelection;
			return FunctionEditor_PART_CURSOR;
		} else {
			return Melder_error ("Make a selection first.");
		}
	} else if (my startSelection < my startWindow || my endSelection > my endWindow) {
		return Melder_error ("Command ambiguous: a part of the selection is out of view. Either zoom or re-select.");
	}
	*tmin = my startSelection;
	*tmax = my endSelection;
	return FunctionEditor_PART_SELECTION;
}

/***** VIEW MENU *****/

static int menu_cb_showAnalyses (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Show analyses", 0)
		BOOLEAN ("Show spectrogram", 1)
		BOOLEAN ("Show pitch", 1)
		BOOLEAN ("Show intensity", 0)
		BOOLEAN ("Show formants", 1)
		BOOLEAN ("Show pulses", 0)
		POSITIVE ("Longest analysis (s)", "5.0")
	EDITOR_OK
		SET_INTEGER ("Show spectrogram", my spectrogram.show)
		SET_INTEGER ("Show pitch", my pitch.show)
		SET_INTEGER ("Show intensity", my intensity.show)
		SET_INTEGER ("Show formants", my formant.show)
		SET_INTEGER ("Show pulses", my pulses.show)
		SET_REAL ("Longest analysis", my longestAnalysis)
	EDITOR_DO
		XmToggleButtonGadgetSetState (my spectrogramToggle, preferences.spectrogram.show = my spectrogram.show = GET_INTEGER ("Show spectrogram"), False);
		XmToggleButtonGadgetSetState (my pitchToggle, preferences.pitch.show = my pitch.show = GET_INTEGER ("Show pitch"), False);
		XmToggleButtonGadgetSetState (my intensityToggle, preferences.intensity.show = my intensity.show = GET_INTEGER ("Show intensity"), False);
		XmToggleButtonGadgetSetState (my formantToggle, preferences.formant.show = my formant.show = GET_INTEGER ("Show formants"), False);
		XmToggleButtonGadgetSetState (my pulsesToggle, preferences.pulses.show = my pulses.show = GET_INTEGER ("Show pulses"), False);
		preferences.longestAnalysis = my longestAnalysis = GET_REAL ("Longest analysis");
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_timeStepSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Time step settings", "Time step settings...")
		OPTIONMENU ("Time step strategy", 1)
			OPTION ("automatic")
			OPTION ("fixed")
			OPTION ("view-dependent")
		LABEL ("", "")
		LABEL ("", "If the time step strategy is \"fixed\":")
		POSITIVE ("Fixed time step (s)", "0.01")
		LABEL ("", "")
		LABEL ("", "If the time step strategy is \"view-dependent\":")
		NATURAL ("Number of time steps per view", "100")
	EDITOR_OK
		SET_INTEGER ("Time step strategy", my timeStepStrategy)
		SET_REAL ("Fixed time step", my fixedTimeStep)
		SET_INTEGER ("Number of time steps per view", my numberOfTimeStepsPerView)
	EDITOR_DO
		preferences.timeStepStrategy = my timeStepStrategy = GET_INTEGER ("Time step strategy");
		preferences.fixedTimeStep = my fixedTimeStep = GET_REAL ("Fixed time step");
		preferences.numberOfTimeStepsPerView = my numberOfTimeStepsPerView = GET_INTEGER ("Number of time steps per view");
		forget (my pitch.data);
		forget (my formant.data);
		forget (my intensity.data);
		forget (my pulses.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

/***** SPECTROGRAM MENU *****/

static int menu_cb_showSpectrogram (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	preferences.spectrogram.show = my spectrogram.show = ! my spectrogram.show;
	FunctionEditor_redraw (me);
	return 1;
}

static int menu_cb_spectrogramSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Spectrogram settings", "Intro 3.2. Configuring the spectrogram")
		REAL ("left View range (Hz)", "0.0")
		POSITIVE ("right View range (Hz)", "5000.0")
		POSITIVE ("Window length (s)", "0.005")
		POSITIVE ("Dynamic range (dB)", "50.0")
		LABEL ("note1", "")
		LABEL ("note2", "")
	EDITOR_OK
		SET_REAL ("left View range", my spectrogram.viewFrom)
		SET_REAL ("right View range", my spectrogram.viewTo)
		SET_REAL ("Window length", my spectrogram.windowLength)
		SET_REAL ("Dynamic range", my spectrogram.dynamicRange)
		if (my spectrogram.timeSteps != 1000 || my spectrogram.frequencySteps != 250 || my spectrogram.method != 1 ||
			my spectrogram.windowShape != 5 || my spectrogram.maximum != 100.0 || my spectrogram.autoscaling != TRUE ||
			my spectrogram.preemphasis != 6.0 || my spectrogram.dynamicCompression != 0.0)
		{
			SET_STRING ("note1", "Warning: you have non-standard \"advanced settings\".")
		} else {
			SET_STRING ("note1", "(all of your \"advanced settings\" have their standard values)")
		}
		if (my timeStepStrategy != 1)
		{
			SET_STRING ("note2", "Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING ("note2", "(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.spectrogram.viewFrom = my spectrogram.viewFrom = GET_REAL ("left View range");
		preferences.spectrogram.viewTo = my spectrogram.viewTo = GET_REAL ("right View range");
		preferences.spectrogram.windowLength = my spectrogram.windowLength = GET_REAL ("Window length");
		preferences.spectrogram.dynamicRange = my spectrogram.dynamicRange = GET_REAL ("Dynamic range");
		forget (my spectrogram.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_advancedSpectrogramSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Advanced spectrogram settings", "Advanced spectrogram settings...")
		LABEL ("", "Time and frequency resolutions:")
		NATURAL ("Number of time steps", "1000")
		NATURAL ("Number of frequency steps", "250")
		LABEL ("", "Spectrogram analysis settings:")
		OPTIONMENU ("Method", 1)
			OPTION ("Fourier")
		OPTIONMENU ("Window shape", 6)
		{
			int i; for (i = 0; i < 6; i ++) {
				OPTIONW (Sound_to_Spectrogram_windowShapeText (i))
			}
		}
		LABEL ("", "Spectrogram view settings:")
		BOOLEAN ("Autoscaling", 1)
		REAL ("Maximum (dB/Hz)", "100.0")
		REAL ("Pre-emphasis (dB/oct)", "6.0")
		REAL ("Dynamic compression (0-1)", "0.0")
	EDITOR_OK
		SET_INTEGER ("Number of time steps", my spectrogram.timeSteps)
		SET_INTEGER ("Number of frequency steps", my spectrogram.frequencySteps)
		SET_INTEGER ("Method", my spectrogram.method)
		SET_INTEGER ("Window shape", my spectrogram.windowShape + 1)
		SET_REAL ("Maximum", my spectrogram.maximum)
		SET_INTEGER ("Autoscaling", my spectrogram.autoscaling)
		SET_REAL ("Pre-emphasis", my spectrogram.preemphasis)
		SET_REAL ("Dynamic compression", my spectrogram.dynamicCompression)
	EDITOR_DO
		preferences.spectrogram.timeSteps = my spectrogram.timeSteps = GET_INTEGER ("Number of time steps");
		preferences.spectrogram.frequencySteps = my spectrogram.frequencySteps = GET_INTEGER ("Number of frequency steps");
		preferences.spectrogram.method = my spectrogram.method = GET_INTEGER ("Method");
		preferences.spectrogram.windowShape = my spectrogram.windowShape = GET_INTEGER ("Window shape") - 1;
		preferences.spectrogram.maximum = my spectrogram.maximum = GET_REAL ("Maximum");
		preferences.spectrogram.autoscaling = my spectrogram.autoscaling = GET_INTEGER ("Autoscaling");
		preferences.spectrogram.preemphasis = my spectrogram.preemphasis = GET_REAL ("Pre-emphasis");
		preferences.spectrogram.dynamicCompression = my spectrogram.dynamicCompression = GET_REAL ("Dynamic compression");
		forget (my spectrogram.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_getFrequency (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	Melder_informationReal (my spectrogram.cursor, L"Hertz");
	return 1;
}

static int menu_cb_getSpectralPowerAtCursorCross (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my spectrogram.show)
		return Melder_error ("No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
	if (! my spectrogram.data) {
		FunctionEditor_SoundAnalysis_computeSpectrogram (me);
		if (! my spectrogram.data) return Melder_error1 (theMessage_Cannot_compute_spectrogram);
	}
	if (part != FunctionEditor_PART_CURSOR) return Melder_error ("Click inside the spectrogram first.");
	MelderInfo_open ();
	MelderInfo_write1 (Melder_double (Matrix_getValueAtXY (my spectrogram.data, tmin, my spectrogram.cursor)));
	MelderInfo_write5 (L" Pa2/Hz (at time = ", Melder_double (tmin), L" seconds and frequency = ",
		Melder_double (my spectrogram.cursor), L" Hz)");
	MelderInfo_close ();
	return 1;
}

static int menu_cb_extractVisibleSpectrogram (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	Spectrogram publish;
	if (! my spectrogram.show)
		return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
	if (! my spectrogram.data) {
		FunctionEditor_SoundAnalysis_computeSpectrogram (me);
		if (! my spectrogram.data) return Melder_error1 (theMessage_Cannot_compute_spectrogram);
	}
	publish = Data_copy (my spectrogram.data);
	if (publish == NULL) return 0;
	if (my publishCallback)
		my publishCallback (me, my publishClosure, publish);
	return 1;
}

static int menu_cb_viewSpectralSlice (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double start = my startSelection == my endSelection ?
		my spectrogram.windowShape == 5 ? my startSelection - my spectrogram.windowLength :
		my startSelection - my spectrogram.windowLength / 2 : my startSelection;
	double finish = my startSelection == my endSelection ?
		my spectrogram.windowShape == 5 ? my endSelection + my spectrogram.windowLength :
		my endSelection + my spectrogram.windowLength / 2 : my endSelection;
	Sound sound = extractSound (me, start, finish);
	Spectrum publish;
	if (sound == NULL) return 0;
	Sound_multiplyByWindow (sound,
		my spectrogram.windowShape == 0 ? enumi (Sound_WINDOW, Rectangular) :
		my spectrogram.windowShape == 1 ? enumi (Sound_WINDOW, Hamming) :
		my spectrogram.windowShape == 2 ? enumi (Sound_WINDOW, Triangular) :
		my spectrogram.windowShape == 3 ? enumi (Sound_WINDOW, Parabolic) :
		my spectrogram.windowShape == 4 ? enumi (Sound_WINDOW, Hanning) :
		my spectrogram.windowShape == 5 ? enumi (Sound_WINDOW, Gaussian2) : 0);
	publish = Sound_to_Spectrum (sound, TRUE);
	forget (sound);
	if (! publish) return 0;
	static MelderStringW sliceName = { 0 };
	MelderStringW_copyW (& sliceName, my data == NULL ? L"untitled" : ((Data) my data) -> nameW);
	MelderStringW_appendCharacter (& sliceName, '_');
	MelderStringW_append1 (& sliceName, Melder_fixed (0.5 * (my startSelection + my endSelection), 3));
	Thing_setNameW (publish, sliceName.string);
	if (my publishCallback)
		my publishCallback (me, my publishClosure, publish);
	return 1;
}

static int menu_cb_drawVisibleSpectrogram (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Draw visible spectrogram", 0)
		our form_pictureWindow (me, cmd);
		our form_pictureMargins (me, cmd);
		BOOLEAN ("Draw selection times", 1);
		BOOLEAN ("Draw selection hairs", 1);
		BOOLEAN ("Garnish", 1);
	EDITOR_OK
		our ok_pictureWindow (me, cmd);
		our ok_pictureMargins (me, cmd);
		SET_INTEGER ("Draw selection times", my picture.drawSelectionTimes);
		SET_INTEGER ("Draw selection hairs", my picture.drawSelectionHairs);
		SET_INTEGER ("Garnish", my picture.garnish);
	EDITOR_DO
		our do_pictureWindow (me, cmd);
		our do_pictureMargins (me, cmd);
		my picture.drawSelectionTimes = GET_INTEGER ("Draw selection times");
		my picture.drawSelectionHairs = GET_INTEGER ("Draw selection hairs");
		my picture.garnish = GET_INTEGER ("Garnish");
		FunctionEditor_setPicturePreferences (me);
		if (! my spectrogram.show)
			return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
		if (! my spectrogram.data) {
			FunctionEditor_SoundAnalysis_computeSpectrogram (me);
			if (! my spectrogram.data) return Melder_error1 (theMessage_Cannot_compute_spectrogram);
		}
		Editor_openPraatPicture (me);
		Spectrogram_paint (my spectrogram.data, my pictureGraphics, my startWindow, my endWindow, my spectrogram.viewFrom, my spectrogram.viewTo,
			my spectrogram.maximum, my spectrogram.autoscaling, my spectrogram.dynamicRange, my spectrogram.preemphasis,
			my spectrogram.dynamicCompression, my picture.garnish);
		FunctionEditor_garnish (me);
		Editor_closePraatPicture (me);
	EDITOR_END
}

/***** PITCH MENU *****/

static int menu_cb_showPitch (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	preferences.pitch.show = my pitch.show = ! my pitch.show;
	FunctionEditor_redraw (me);
	return 1;
}

static int menu_cb_pitchSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Pitch settings", "Intro 4.2. Configuring the pitch contour")
		POSITIVE ("left Pitch range (Hz)", "75.0")
		POSITIVE ("right Pitch range (Hz)", "500.0")
		OPTIONMENU ("Unit", 1)
			OPTIONS_ENUMW (ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, itext, Function_UNIT_TEXT_MENU), Pitch_UNIT_min, Pitch_UNIT_max)
		RADIO ("Optimize for", 1)
			RADIOBUTTON ("Intonation (AC method)")
			RADIOBUTTON ("Voice analysis (CC method)")
		OPTIONMENU ("Drawing method", FunctionEditor_pitch_DRAWING_METHOD_AUTOMATIC)
			OPTION ("curve")
			OPTION ("speckles")
			OPTION ("automatic")
		LABEL ("note1", "")
		LABEL ("note2", "")
	EDITOR_OK
		SET_REAL ("left Pitch range", my pitch.floor)
		SET_REAL ("right Pitch range", my pitch.ceiling)
		SET_INTEGER ("Unit", my pitch.unit + 1 - Pitch_UNIT_min)
		SET_INTEGER ("Drawing method", my pitch.drawingMethod)
		SET_INTEGER ("Optimize for", my pitch.method)
		if (my pitch.viewFrom != 0.0 || my pitch.viewTo != 0.0 ||
			my pitch.veryAccurate != FALSE || my pitch.maximumNumberOfCandidates != 15 ||
			my pitch.silenceThreshold != 0.03 || my pitch.voicingThreshold != 0.45 || my pitch.octaveCost != 0.01 ||
			my pitch.octaveJumpCost != 0.35 || my pitch.voicedUnvoicedCost != 0.14)
		{
			SET_STRING ("note1", "Warning: you have some non-standard \"advanced settings\".")
		} else {
			SET_STRING ("note1", "(all of your \"advanced settings\" have their standard values)")
		}
		if (my timeStepStrategy != 1)
		{
			SET_STRING ("note2", "Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING ("note2", "(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.pitch.floor = my pitch.floor = GET_REAL ("left Pitch range");
		preferences.pitch.ceiling = my pitch.ceiling = GET_REAL ("right Pitch range");
		preferences.pitch.unit = my pitch.unit = GET_INTEGER ("Unit") - 1 + Pitch_UNIT_min;
		preferences.pitch.drawingMethod = my pitch.drawingMethod = GET_INTEGER ("Drawing method");
		preferences.pitch.method = my pitch.method = GET_INTEGER ("Optimize for");
		forget (my pitch.data);
		forget (my intensity.data);
		forget (my pulses.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_advancedPitchSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Advanced pitch settings", "Advanced pitch settings...")
		LABEL ("", "Make view range different from analysis range:")
		REAL ("left View range (units)", "0.0 (= auto)")
		REAL ("right View range (units)", "0.0 (= auto)")
		LABEL ("", "Analysis settings:")
		BOOLEAN ("Very accurate", 0)
		NATURAL ("Max. number of candidates", "15")
		REAL ("Silence threshold", "0.03")
		REAL ("Voicing threshold", "0.45")
		REAL ("Octave cost", "0.01")
		REAL ("Octave-jump cost", "0.35")
		REAL ("Voiced / unvoiced cost", "0.14")
	EDITOR_OK
		SET_REAL ("left View range", my pitch.viewFrom)
		SET_REAL ("right View range", my pitch.viewTo)
		SET_INTEGER ("Very accurate", my pitch.veryAccurate)
		SET_INTEGER ("Max. number of candidates", my pitch.maximumNumberOfCandidates)
		SET_REAL ("Silence threshold", my pitch.silenceThreshold)
		SET_REAL ("Voicing threshold", my pitch.voicingThreshold)
		SET_REAL ("Octave cost", my pitch.octaveCost)
		SET_REAL ("Octave-jump cost", my pitch.octaveJumpCost)
		SET_REAL ("Voiced / unvoiced cost", my pitch.voicedUnvoicedCost)
	EDITOR_DO
		long maxnCandidates = GET_INTEGER ("Max. number of candidates");
		if (maxnCandidates < 2) return Melder_error ("Maximum number of candidates must be greater than 1.");
		preferences.pitch.viewFrom = my pitch.viewFrom = GET_REAL ("left View range");
		preferences.pitch.viewTo = my pitch.viewTo = GET_REAL ("right View range");
		preferences.pitch.veryAccurate = my pitch.veryAccurate = GET_INTEGER ("Very accurate");
		preferences.pitch.maximumNumberOfCandidates = my pitch.maximumNumberOfCandidates = GET_INTEGER ("Max. number of candidates");
		preferences.pitch.silenceThreshold = my pitch.silenceThreshold = GET_REAL ("Silence threshold");
		preferences.pitch.voicingThreshold = my pitch.voicingThreshold = GET_REAL ("Voicing threshold");
		preferences.pitch.octaveCost = my pitch.octaveCost = GET_REAL ("Octave cost");
		preferences.pitch.octaveJumpCost = my pitch.octaveJumpCost = GET_REAL ("Octave-jump cost");
		preferences.pitch.voicedUnvoicedCost = my pitch.voicedUnvoicedCost = GET_REAL ("Voiced / unvoiced cost");
		forget (my pitch.data);
		forget (my intensity.data);
		forget (my pulses.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_pitchListing (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my pitch.show)
		return Melder_error ("No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	MelderInfo_open ();
	MelderInfo_writeLine2 (L"Time_s   F0_", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit, Function_UNIT_TEXT_SHORT));
	if (part == FunctionEditor_PART_CURSOR) {
		double f0 = Pitch_getValueAtTime (my pitch.data, tmin, my pitch.unit, TRUE);
		f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		MelderInfo_writeLine3 (Melder_fixed (tmin, 6), L"   ", Melder_fixed (f0, 6));
	} else {
		long i, i1, i2;
		Sampled_getWindowSamples (my pitch.data, tmin, tmax, & i1, & i2);
		for (i = i1; i <= i2; i ++) {
			double t = Sampled_indexToX (my pitch.data, i);
			double f0 = Sampled_getValueAtSample (my pitch.data, i, Pitch_LEVEL_FREQUENCY, my pitch.unit);
			f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, my pitch.unit);
			MelderInfo_writeLine3 (Melder_fixed (t, 6), L"   ", Melder_fixed (f0, 6));
		}
	}
	MelderInfo_close ();
	return 1;
}

static int menu_cb_getPitch (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my pitch.show)
		return Melder_error ("No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		double f0 = Pitch_getValueAtTime (my pitch.data, tmin, my pitch.unit, TRUE);
		f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		Melder_information4 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit, 0),
			L" (interpolated pitch at CURSOR)");
	} else {
		double f0 = Pitch_getMean (my pitch.data, tmin, tmax, my pitch.unit);
		f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		Melder_information6 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit, 0),
			L" (mean pitch ", FunctionEditor_partString_locative (part), L")");
	}
	return 1;
}

static int menu_cb_getMinimumPitch (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax, f0;
	int part = makeQueriable (me, FALSE, & tmin, & tmax); iferror return 0;
	if (! my pitch.show)
		return Melder_error ("No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	f0 = Pitch_getMinimum (my pitch.data, tmin, tmax, my pitch.unit, TRUE);
	f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, my pitch.unit);
	Melder_information6 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit, 0),
		L" (minimum pitch ", FunctionEditor_partString_locative (part), L")");
	return 1;
}

static int menu_cb_getMaximumPitch (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax, f0;
	int part = makeQueriable (me, FALSE, & tmin, & tmax); iferror return 0;
	if (! my pitch.show)
		return Melder_error ("No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	f0 = Pitch_getMaximum (my pitch.data, tmin, tmax, my pitch.unit, TRUE);
	f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, my pitch.unit);
	Melder_information6 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit, 0),
		L" (maximum pitch ", FunctionEditor_partString_locative (part), L")");
	return 1;
}

static int menu_cb_extractVisiblePitchContour (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	if (! my pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	Pitch publish = Data_copy (my pitch.data);
	if (! publish) return 0;
	if (my publishCallback)
		my publishCallback (me, my publishClosure, publish);
	return 1;
}

static int menu_cb_drawVisiblePitchContour (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Draw visible pitch contour", 0)
		our form_pictureWindow (me, cmd);
		LABEL ("", "Pitch:")
		BOOLEAN ("Speckle", 0);
		our form_pictureMargins (me, cmd);
		BOOLEAN ("Draw selection times", 1);
		BOOLEAN ("Draw selection hairs", 1);
		BOOLEAN ("Garnish", 1);
	EDITOR_OK
		our ok_pictureWindow (me, cmd);
		SET_INTEGER ("Speckle", my pitch.picture.speckle);
		our ok_pictureMargins (me, cmd);
		SET_INTEGER ("Draw selection times", my picture.drawSelectionTimes);
		SET_INTEGER ("Draw selection hairs", my picture.drawSelectionHairs);
		SET_INTEGER ("Garnish", our preferences.picture.pitch.garnish);
	EDITOR_DO
		our do_pictureWindow (me, cmd);
		preferences.pitch.picture.speckle = my pitch.picture.speckle = GET_INTEGER ("Speckle");
		our do_pictureMargins (me, cmd);
		my picture.drawSelectionTimes = GET_INTEGER ("Draw selection times");
		my picture.drawSelectionHairs = GET_INTEGER ("Draw selection hairs");
		our preferences.picture.pitch.garnish = GET_INTEGER ("Garnish");
		FunctionEditor_setPicturePreferences (me);
		if (! my pitch.show)
			return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
		if (! my pitch.data) {
			FunctionEditor_SoundAnalysis_computePitch (me);
			if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
		}
		Editor_openPraatPicture (me);
		double pitchFloor_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, my pitch.floor, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		double pitchCeiling_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, my pitch.ceiling, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		double pitchFloor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchFloor_hidden, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		double pitchCeiling_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCeiling_hidden, Pitch_LEVEL_FREQUENCY, my pitch.unit);
		double pitchViewFrom_overt = my pitch.viewFrom < my pitch.viewTo ? my pitch.viewFrom : pitchFloor_overt;
		double pitchViewTo_overt = my pitch.viewFrom < my pitch.viewTo ? my pitch.viewTo : pitchCeiling_overt;
		Pitch_draw (my pitch.data, my pictureGraphics, my startWindow, my endWindow, pitchViewFrom_overt, pitchViewTo_overt,
			our preferences.picture.pitch.garnish, GET_INTEGER ("Speckle"), my pitch.unit);
		FunctionEditor_garnish (me);
		Editor_closePraatPicture (me);
	EDITOR_END
}

/***** INTENSITY MENU *****/

static int menu_cb_showIntensity (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	preferences.intensity.show = my intensity.show = ! my intensity.show;
	FunctionEditor_redraw (me);
	return 1;
}

static int menu_cb_intensitySettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Intensity settings", "Intro 6.2. Configuring the intensity contour")
		REAL ("left View range (dB)", "50.0")
		REAL ("right View range (dB)", "100.0")
		RADIO ("Averaging method", Intensity_averaging_ENERGY + 1)
			RADIOBUTTON ("median")
			RADIOBUTTON ("mean energy")
			RADIOBUTTON ("mean sones")
			RADIOBUTTON ("mean dB")
		BOOLEAN ("Subtract mean pressure", 1)
		LABEL ("", "Note: the pitch floor is taken from the pitch settings.")
		LABEL ("note2", "")
	EDITOR_OK
		SET_REAL ("left View range", my intensity.viewFrom)
		SET_REAL ("right View range", my intensity.viewTo)
		SET_INTEGER ("Averaging method", my intensity.averagingMethod + 1)
		SET_INTEGER ("Subtract mean pressure", my intensity.subtractMeanPressure)
		if (my timeStepStrategy != 1)
		{
			SET_STRING ("note2", "Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING ("note2", "(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.intensity.viewFrom = my intensity.viewFrom = GET_REAL ("left View range");
		preferences.intensity.viewTo = my intensity.viewTo = GET_REAL ("right View range");
		preferences.intensity.averagingMethod = my intensity.averagingMethod = GET_INTEGER ("Averaging method") - 1;
		preferences.intensity.subtractMeanPressure = my intensity.subtractMeanPressure = GET_INTEGER ("Subtract mean pressure");
		forget (my intensity.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_extractVisibleIntensityContour (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	Intensity publish;
	if (! my intensity.show)
		return Melder_error ("No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! my intensity.data) {
		FunctionEditor_SoundAnalysis_computeIntensity (me);
		if (! my intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	publish = Data_copy (my intensity.data);
	if (! publish) return 0;
	if (my publishCallback)
		my publishCallback (me, my publishClosure, publish);
	return 1;
}

static int menu_cb_intensityListing (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my intensity.show)
		return Melder_error ("No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! my intensity.data) {
		FunctionEditor_SoundAnalysis_computeIntensity (me);
		if (! my intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Time_s   Intensity_dB");
	if (part == FunctionEditor_PART_CURSOR) {
		double intensity = Vector_getValueAtX (my intensity.data, tmin, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR);
		MelderInfo_writeLine3 (Melder_fixed (tmin, 6), L"   ", Melder_fixed (intensity, 6));
	} else {
		long i, i1, i2;
		Sampled_getWindowSamples (my intensity.data, tmin, tmax, & i1, & i2);
		for (i = i1; i <= i2; i ++) {
			double t = Sampled_indexToX (my intensity.data, i);
			double intensity = Vector_getValueAtX (my intensity.data, t, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_NEAREST);
			MelderInfo_writeLine3 (Melder_fixed (t, 6), L"   ", Melder_fixed (intensity, 6));
		}
	}
	MelderInfo_close ();
	return 1;
}

static int menu_cb_getIntensity (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my intensity.show)
		return Melder_error ("No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! my intensity.data) {
		FunctionEditor_SoundAnalysis_computeIntensity (me);
		if (! my intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		Melder_information2 (Melder_double (Vector_getValueAtX (my intensity.data, tmin, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR)), L" dB (intensity at CURSOR)");
	} else {
		static const wchar_t *methodString [] = { L"median", L"mean-energy", L"mean-sones", L"mean-dB" };
		Melder_information6 (Melder_double (Intensity_getAverage (my intensity.data, tmin, tmax, my intensity.averagingMethod)),
			L" dB (", methodString [my intensity.averagingMethod], L" intensity ", FunctionEditor_partString_locative (part), L")");
	}
	return 1;
}

/***** FORMANT MENU *****/

static int menu_cb_showFormants (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	preferences.formant.show = my formant.show = ! my formant.show;
	FunctionEditor_redraw (me);
	return 1;
}

static int menu_cb_formantSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Formant settings", "Intro 5.2. Configuring the formant contours")
		POSITIVE ("Maximum formant (Hz)", "5500.0")
		POSITIVE ("Number of formants", "5.0")
		POSITIVE ("Window length (s)", "0.025")
		REAL ("Dynamic range (dB)", "30.0")
		POSITIVE ("Dot size (mm)", "1.0")
		LABEL ("note1", "")
		LABEL ("note2", "")
	EDITOR_OK
		SET_REAL ("Maximum formant", my formant.maximumFormant)
		SET_REAL ("Number of formants", 0.5 * my formant.numberOfPoles)
		SET_REAL ("Window length", my formant.windowLength)
		SET_REAL ("Dynamic range", my formant.dynamicRange)
		SET_REAL ("Dot size", my formant.dotSize)
		if (my formant.method != 1 || my formant.preemphasisFrom != 50.0) {
			SET_STRING ("note1", "Warning: you have non-standard \"advanced settings\".")
		} else {
			SET_STRING ("note1", "(all of your \"advanced settings\" have their standard values)")
		}
		if (my timeStepStrategy != 1)
		{
			SET_STRING ("note2", "Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING ("note2", "(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.formant.maximumFormant = my formant.maximumFormant = GET_REAL ("Maximum formant");
		preferences.formant.numberOfPoles = my formant.numberOfPoles = 2.0 * GET_REAL ("Number of formants");
		preferences.formant.windowLength = my formant.windowLength = GET_REAL ("Window length");
		preferences.formant.dynamicRange = my formant.dynamicRange = GET_REAL ("Dynamic range");
		preferences.formant.dotSize = my formant.dotSize = GET_REAL ("Dot size");
		forget (my formant.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_advancedFormantSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Advanced formant settings", "Advanced formant settings...")
		RADIO ("Method", 1)
			RADIOBUTTON ("Burg")
		POSITIVE ("Pre-emphasis from (Hz)", "50.0")
	EDITOR_OK
		SET_INTEGER ("Method", my formant.method)
		SET_REAL ("Pre-emphasis from", my formant.preemphasisFrom)
	EDITOR_DO
		preferences.formant.method = my formant.method = GET_INTEGER ("Method");
		preferences.formant.preemphasisFrom = my formant.preemphasisFrom = GET_REAL ("Pre-emphasis from");
		forget (my formant.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_extractVisibleFormantContour (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	Formant publish;
	if (! my formant.show)
		return Melder_error ("No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! my formant.data) {
		FunctionEditor_SoundAnalysis_computeFormants (me);
		if (! my formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	publish = Data_copy (my formant.data);
	if (! publish) return 0;
	if (my publishCallback)
		my publishCallback (me, my publishClosure, publish);
	return 1;
}

static int menu_cb_formantListing (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my formant.show)
		return Melder_error ("No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! my formant.data) {
		FunctionEditor_SoundAnalysis_computeFormants (me);
		if (! my formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Time_s   F1_Hz   F2_Hz   F3_Hz   F4_Hz");
	if (part == FunctionEditor_PART_CURSOR) {
		double f1 = Formant_getValueAtTime (my formant.data, 1, tmin, 0);
		double f2 = Formant_getValueAtTime (my formant.data, 2, tmin, 0);
		double f3 = Formant_getValueAtTime (my formant.data, 3, tmin, 0);
		double f4 = Formant_getValueAtTime (my formant.data, 4, tmin, 0);
		MelderInfo_write5 (Melder_fixed (tmin, 6), L"   ", Melder_fixed (f1, 6), L"   ", Melder_fixed (f2, 6));
		MelderInfo_writeLine4 (L"   ", Melder_fixed (f3, 6), L"   ", Melder_fixed (f4, 6));
	} else {
		long i, i1, i2;
		Sampled_getWindowSamples (my formant.data, tmin, tmax, & i1, & i2);
		for (i = i1; i <= i2; i ++) {
			double t = Sampled_indexToX (my formant.data, i);
			double f1 = Formant_getValueAtTime (my formant.data, 1, t, 0);
			double f2 = Formant_getValueAtTime (my formant.data, 2, t, 0);
			double f3 = Formant_getValueAtTime (my formant.data, 3, t, 0);
			double f4 = Formant_getValueAtTime (my formant.data, 4, t, 0);
			MelderInfo_write5 (Melder_fixed (t, 6), L"   ", Melder_fixed (f1, 6), L"   ", Melder_fixed (f2, 6));
			MelderInfo_writeLine4 (L"   ", Melder_fixed (f3, 6), L"   ", Melder_fixed (f4, 6));
		}
	}
	MelderInfo_close ();
	return 1;
}

static int do_getFormant (FunctionEditor me, int iformant) {
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my formant.show)
		return Melder_error ("No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! my formant.data) {
		FunctionEditor_SoundAnalysis_computeFormants (me);
		if (! my formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		Melder_information4 (Melder_double (Formant_getValueAtTime (my formant.data, iformant, tmin, 0)),
			L" Hertz (nearest F", Melder_integer (iformant), L" to CURSOR)");
	} else {
		Melder_information6 (Melder_double (Formant_getMean (my formant.data, iformant, tmin, tmax, 0)),
			L" Hertz (mean F", Melder_integer (iformant), L" ", FunctionEditor_partString_locative (part), L")");
	}
	return 1;
}
static int do_getBandwidth (FunctionEditor me, int iformant) {
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	if (! my formant.show)
		return Melder_error ("No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! my formant.data) {
		FunctionEditor_SoundAnalysis_computeFormants (me);
		if (! my formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		Melder_information4 (Melder_double (Formant_getBandwidthAtTime (my formant.data, iformant, tmin, 0)),
			L" Hertz (nearest B", Melder_integer (iformant), L" to CURSOR)");
	} else {
		Melder_information6 (Melder_double (Formant_getBandwidthAtTime (my formant.data, iformant, 0.5 * (tmin + tmax), 0)),
			L" Hertz (B", Melder_integer (iformant), L" in centre of ", FunctionEditor_partString (part), L")");
	}
	return 1;
}
static int menu_cb_getFirstFormant (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getFormant (me, 1); }
static int menu_cb_getFirstBandwidth (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getBandwidth (me, 1); }
static int menu_cb_getSecondFormant (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getFormant (me, 2); }
static int menu_cb_getSecondBandwidth (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getBandwidth (me, 2); }
static int menu_cb_getThirdFormant (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getFormant (me, 3); }
static int menu_cb_getThirdBandwidth (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getBandwidth (me, 3); }
static int menu_cb_getFourthFormant (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getFormant (me, 4); }
static int menu_cb_getFourthBandwidth (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor); return do_getBandwidth (me, 4); }

static int menu_cb_getFormant (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Get formant", 0)
		NATURAL ("Formant number", "5")
	EDITOR_OK
	EDITOR_DO
		if (! do_getFormant (me, GET_INTEGER ("Formant number"))) return 0;
	EDITOR_END
}

static int menu_cb_getBandwidth (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Get bandwidth", 0)
		NATURAL ("Formant number", "5")
	EDITOR_OK
	EDITOR_DO
		if (! do_getBandwidth (me, GET_INTEGER ("Formant number"))) return 0;
	EDITOR_END
}

/***** PULSE MENU *****/

static int menu_cb_showPulses (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	preferences.pulses.show = my pulses.show = ! my pulses.show;
	FunctionEditor_redraw (me);
	return 1;
}

static int menu_cb_advancedPulsesSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Advanced pulses settings", "Advanced pulses settings...")
		POSITIVE ("Maximum period factor", "1.3")
		POSITIVE ("Maximum amplitude factor", "1.6")
	EDITOR_OK
		SET_REAL ("Maximum period factor", my pulses.maximumPeriodFactor)
		SET_REAL ("Maximum amplitude factor", my pulses.maximumAmplitudeFactor)
	EDITOR_DO
		preferences.pulses.maximumPeriodFactor = my pulses.maximumPeriodFactor = GET_REAL ("Maximum period factor");
		preferences.pulses.maximumAmplitudeFactor = my pulses.maximumAmplitudeFactor = GET_REAL ("Maximum amplitude factor");
		forget (my pulses.data);
		FunctionEditor_redraw (me);
	EDITOR_END
}

static int menu_cb_extractVisiblePulses (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	Pitch publish;
	if (! my pulses.show)
		return Melder_error ("No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! my pulses.data) {
		FunctionEditor_SoundAnalysis_computePulses (me);
		if (! my pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	publish = Data_copy (my pulses.data);
	if (! publish) return 0;
	if (my publishCallback)
		my publishCallback (me, my publishClosure, publish);
	return 1;
}

static int menu_cb_voiceReport (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	time_t today = time (NULL);
	Sound sound = NULL;
	double tmin, tmax;
	int part = makeQueriable (me, FALSE, & tmin, & tmax); iferror return 0;
	if (! my pulses.show)
		return Melder_error ("No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! my pulses.data) {
		FunctionEditor_SoundAnalysis_computePulses (me);
		if (! my pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	sound = extractSound (me, tmin, tmax);
	if (! sound) return Melder_error ("Selection too small (or out of memory).");
	MelderInfo_open ();
	MelderInfo_writeLine4 (L"-- Voice report for ", my nameW, L" --\nDate: ", Melder_peekAsciiToWcs (ctime (& today)));
	if ((my pitch.method & 2) == 0)
		MelderInfo_writeLine1 (L"WARNING: some of the following measurements may be imprecise.\n"
			"For more precision, go to \"Pitch settings\" and choose \"Optimize for voice analysis\".\n");
	MelderInfo_writeLine2 (L"Time range of ", FunctionEditor_partString (part));
	Sound_Pitch_PointProcess_voiceReport (sound, my pitch.data, my pulses.data, tmin, tmax,
		my pitch.floor, my pitch.ceiling, my pulses.maximumPeriodFactor, my pulses.maximumAmplitudeFactor, my pitch.silenceThreshold, my pitch.voicingThreshold);
	MelderInfo_close ();
	forget (sound);
	return 1;
}

static int menu_cb_pulseListing (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	long i, i1, i2;
	double tmin, tmax;
	makeQueriable (me, FALSE, & tmin, & tmax); iferror return 0;
	if (! my pulses.show)
		return Melder_error ("No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! my pulses.data) {
		FunctionEditor_SoundAnalysis_computePulses (me);
		if (! my pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Time_s");
	i1 = PointProcess_getHighIndex (my pulses.data, tmin);
	i2 = PointProcess_getLowIndex (my pulses.data, tmax);
	for (i = i1; i <= i2; i ++) {
		double t = my pulses.data -> t [i];
		MelderInfo_writeLine1 (Melder_fixed (t, 12));
	}
	MelderInfo_close ();
	return 1;
}

/*
static int cb_getJitter_xx (FunctionEditor me, double (*PointProcess_getJitter_xx) (PointProcess, double, double, double, double, double)) {
	double minimumPeriod = 0.8 / my pitch.ceiling, maximumPeriod = 1.25 / my pitch.floor;
	if (! my pulses.show)
		return Melder_error ("No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! my pulses.data) {
		computePulses (me);
		if (! my pulses.data) return Melder_error (theMessage_Cannot_compute_pulses);
	}
	if (my startSelection == my endSelection)
		return Melder_error ("Make a selection first.");
	if (! queriable (me)) return 0;
	Melder_informationReal (PointProcess_getJitter_xx (my pulses.data, my startSelection, my endSelection,
		minimumPeriod, maximumPeriod, my pulses.maximumPeriodFactor), NULL);
	return 1;
}
DIRECT (FunctionEditor, cb_getJitter_local) if (! cb_getJitter_xx (me, PointProcess_getJitter_local)) return 0; END
DIRECT (FunctionEditor, cb_getJitter_local_absolute) if (! cb_getJitter_xx (me, PointProcess_getJitter_local_absolute)) return 0; END
DIRECT (FunctionEditor, cb_getJitter_rap) if (! cb_getJitter_xx (me, PointProcess_getJitter_rap)) return 0; END
DIRECT (FunctionEditor, cb_getJitter_ppq5) if (! cb_getJitter_xx (me, PointProcess_getJitter_ppq5)) return 0; END
DIRECT (FunctionEditor, cb_getJitter_ddp) if (! cb_getJitter_xx (me, PointProcess_getJitter_ddp)) return 0; END

static int cb_getShimmer_xx (FunctionEditor me, double (*PointProcess_Sound_getShimmer_xx) (PointProcess, Sound, double, double, double, double, double)) {
	Sound sound = NULL;
	double minimumPeriod = 0.8 / my pitch.ceiling, maximumPeriod = 1.25 / my pitch.floor;
	if (! my pulses.show)
		return Melder_error ("No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! my pulses.data) {
		computePulses (me);
		if (! my pulses.data) return Melder_error (theMessage_Cannot_compute_pulses);
	}
	if (my startSelection == my endSelection)
		return Melder_error ("Make a selection first.");
	if (! queriable (me)) return 0;
	sound = extractSound (me, my startSelection, my endSelection);
	if (! sound) return Melder_error ("Selection too small (or out of memory).");
	Melder_informationReal (PointProcess_Sound_getShimmer_xx (my pulses.data, sound, my startSelection, my endSelection,
		minimumPeriod, maximumPeriod, my pulses.maximumAmplitudeFactor), NULL);
	forget (sound);
	return 1;
}
DIRECT (FunctionEditor, cb_getShimmer_local) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_local)) return 0; END
DIRECT (FunctionEditor, cb_getShimmer_local_dB) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_local_dB)) return 0; END
DIRECT (FunctionEditor, cb_getShimmer_apq3) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_apq3)) return 0; END
DIRECT (FunctionEditor, cb_getShimmer_apq5) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_apq5)) return 0; END
DIRECT (FunctionEditor, cb_getShimmer_apq11) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_apq11)) return 0; END
DIRECT (FunctionEditor, cb_getShimmer_dda) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_dda)) return 0; END
*/

/***** SELECT MENU *****/

static int menu_cb_moveCursorToMinimumPitch (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	if (! my pitch.show)
		return Melder_error ("No pitch contour is visible.\nFirst choose \"Show pitch\" from the View menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	if (my startSelection == my endSelection) {
		return Melder_error ("Empty selection.");
	} else {
		double time;
		Pitch_getMinimumAndTime (my pitch.data, my startSelection, my endSelection,
			my pitch.unit, 1, NULL, & time);
		if (! NUMdefined (time))
			return Melder_error ("Selection is voiceless.");
		my startSelection = my endSelection = time;
		FunctionEditor_marksChanged (me);
	}
	return 1;
}

static int menu_cb_moveCursorToMaximumPitch (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	if (! my pitch.show)
		return Melder_error ("No pitch contour is visible.\nFirst choose \"Show pitch\" from the View menu.");
	if (! my pitch.data) {
		FunctionEditor_SoundAnalysis_computePitch (me);
		if (! my pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	if (my startSelection == my endSelection) {
		return Melder_error1 (L"Empty selection.");
	} else {
		double time;
		Pitch_getMaximumAndTime (my pitch.data, my startSelection, my endSelection,
			my pitch.unit, 1, NULL, & time);
		if (! NUMdefined (time))
			return Melder_error1 (L"Selection is voiceless.");
		my startSelection = my endSelection = time;
		FunctionEditor_marksChanged (me);
	}
	return 1;
}

void FunctionEditor_SoundAnalysis_draw (I) {
	iam (FunctionEditor);
	double pitchFloor_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, my pitch.floor, Pitch_LEVEL_FREQUENCY, my pitch.unit);
	double pitchCeiling_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, my pitch.ceiling, Pitch_LEVEL_FREQUENCY, my pitch.unit);
	double pitchFloor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchFloor_hidden, Pitch_LEVEL_FREQUENCY, my pitch.unit);
	double pitchCeiling_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCeiling_hidden, Pitch_LEVEL_FREQUENCY, my pitch.unit);
	double pitchViewFrom_overt = my pitch.viewFrom < my pitch.viewTo ? my pitch.viewFrom : pitchFloor_overt;
	double pitchViewTo_overt = my pitch.viewFrom < my pitch.viewTo ? my pitch.viewTo : pitchCeiling_overt;
	double pitchViewFrom_hidden = ClassFunction_isUnitLogarithmic (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit) ? log10 (pitchViewFrom_overt) : pitchViewFrom_overt;
	double pitchViewTo_hidden = ClassFunction_isUnitLogarithmic (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit) ? log10 (pitchViewTo_overt) : pitchViewTo_overt;

	Graphics_setWindow (my graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (my graphics, Graphics_WHITE);
	Graphics_fillRectangle (my graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (my graphics, Graphics_BLACK);
	Graphics_rectangle (my graphics, 0.0, 1.0, 0.0, 1.0);

	if (my endWindow - my startWindow > my longestAnalysis) {
		Graphics_setFont (my graphics, Graphics_FONT_HELVETICA);
		Graphics_setFontSize (my graphics, 9);
		Graphics_setTextAlignment (my graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text3 (my graphics, 0.5, 0.67, L"To see the analyses, zoom in to at most ", Melder_half (my longestAnalysis), L" seconds,");
		Graphics_text (my graphics, 0.5, 0.33, L"or raise the \"longest analysis\" setting with \"Show analyses\" in the View menu.");
		Graphics_setFontSize (my graphics, 10);
		return;
	}
	FunctionEditor_SoundAnalysis_computeSpectrogram (me);
	if (my spectrogram.show && my spectrogram.data != NULL) {
		Spectrogram_paintInside (my spectrogram.data, my graphics, my startWindow, my endWindow, 
			my spectrogram.viewFrom, my spectrogram.viewTo, my spectrogram.maximum, my spectrogram.autoscaling,
			my spectrogram.dynamicRange, my spectrogram.preemphasis, my spectrogram.dynamicCompression);
	}
	FunctionEditor_SoundAnalysis_computePitch (me);
	if (my pitch.show && my pitch.data != NULL) {
		double periodsPerAnalysisWindow = my pitch.method == 1 ? 3.0 : 1.0;
		double greatestNonUndersamplingTimeStep = 0.5 * periodsPerAnalysisWindow / my pitch.floor;
		double defaultTimeStep = 0.5 * greatestNonUndersamplingTimeStep;
		double timeStep =
			my timeStepStrategy == 2 ? my fixedTimeStep :
			my timeStepStrategy == 3 ? (my endWindow - my startWindow) / my numberOfTimeStepsPerView :
			defaultTimeStep;
		int undersampled = timeStep > greatestNonUndersamplingTimeStep;
		long numberOfVisiblePitchPoints = (long) ((my endWindow - my startWindow) / timeStep);
		Graphics_setColour (my graphics, Graphics_CYAN);
		Graphics_setLineWidth (my graphics, 3.0);
		if ((my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_AUTOMATIC && (undersampled || numberOfVisiblePitchPoints < 101)) ||
		    my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_SPECKLE)
		{
			Pitch_drawInside (my pitch.data, my graphics, my startWindow, my endWindow, pitchViewFrom_overt, pitchViewTo_overt, 2, my pitch.unit);
		}
		if ((my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_AUTOMATIC && ! undersampled) ||
		    my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_CURVE)
		{
			Pitch_drawInside (my pitch.data, my graphics, my startWindow, my endWindow, pitchViewFrom_overt, pitchViewTo_overt, FALSE, my pitch.unit);
		}
		Graphics_setColour (my graphics, Graphics_BLUE);
		Graphics_setLineWidth (my graphics, 1.0);
		if ((my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_AUTOMATIC && (undersampled || numberOfVisiblePitchPoints < 101)) ||
		    my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_SPECKLE)
		{
			Pitch_drawInside (my pitch.data, my graphics, my startWindow, my endWindow, pitchViewFrom_overt, pitchViewTo_overt, 1, my pitch.unit);
		}
		if ((my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_AUTOMATIC && ! undersampled) ||
		    my pitch.drawingMethod == FunctionEditor_pitch_DRAWING_METHOD_CURVE)
		{
			Pitch_drawInside (my pitch.data, my graphics, my startWindow, my endWindow, pitchViewFrom_overt, pitchViewTo_overt, FALSE, my pitch.unit);
		}
		Graphics_setColour (my graphics, Graphics_BLACK);
	}
	FunctionEditor_SoundAnalysis_computeIntensity (me);
	if (my intensity.show && my intensity.data != NULL) {
		Graphics_setColour (my graphics, my spectrogram.show ? Graphics_YELLOW : Graphics_LIME);
		Graphics_setLineWidth (my graphics, my spectrogram.show ? 1.0 : 3.0);
		Intensity_drawInside (my intensity.data, my graphics, my startWindow, my endWindow,
			my intensity.viewFrom, my intensity.viewTo);
		Graphics_setLineWidth (my graphics, 1.0);
		Graphics_setColour (my graphics, Graphics_BLACK);
	}
	FunctionEditor_SoundAnalysis_computeFormants (me);
	if (my formant.show && my formant.data != NULL) {
		Graphics_setColour (my graphics, Graphics_RED);
		Formant_drawSpeckles_inside (my formant.data, my graphics, my startWindow, my endWindow, 
			my spectrogram.viewFrom, my spectrogram.viewTo, my formant.dynamicRange, my formant.dotSize);
		Graphics_setColour (my graphics, Graphics_BLACK);
	}
	/*
	 * Draw vertical scales.
	 */
	if (my pitch.show) {
		double pitchCursor_overt = NUMundefined, pitchCursor_hidden = NUMundefined;
		Graphics_setWindow (my graphics, my startWindow, my endWindow, pitchViewFrom_hidden, pitchViewTo_hidden);
		Graphics_setColour (my graphics, Graphics_NAVY);
		if (my pitch.data) {
			if (my startSelection == my endSelection)
				pitchCursor_hidden = Pitch_getValueAtTime (my pitch.data, my startSelection, my pitch.unit, 1);
			else
				pitchCursor_hidden = Pitch_getMean (my pitch.data, my startSelection, my endSelection, my pitch.unit);
			pitchCursor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCursor_hidden, Pitch_LEVEL_FREQUENCY, my pitch.unit);
			if (NUMdefined (pitchCursor_hidden)) {
				Graphics_setTextAlignment (my graphics, Graphics_LEFT, Graphics_HALF);
				Graphics_printf (my graphics, my endWindow, pitchCursor_hidden, L"%.5g %ls", pitchCursor_overt,
					ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit,
						Function_UNIT_TEXT_SHORT | Function_UNIT_TEXT_GRAPHICAL));
			}
			if (! NUMdefined (pitchCursor_hidden) || Graphics_dyWCtoMM (my graphics, pitchCursor_hidden - pitchViewFrom_hidden) > 5.0) {
				Graphics_setTextAlignment (my graphics, Graphics_LEFT, Graphics_BOTTOM);
				Graphics_printf (my graphics, my endWindow, pitchViewFrom_hidden - Graphics_dyMMtoWC (my graphics, 0.5), L"%.4g %ls",
					pitchViewFrom_overt,
					ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit,
						Function_UNIT_TEXT_SHORT | Function_UNIT_TEXT_GRAPHICAL));
			}
			if (! NUMdefined (pitchCursor_hidden) || Graphics_dyWCtoMM (my graphics, pitchViewTo_hidden - pitchCursor_hidden) > 5.0) {
				Graphics_setTextAlignment (my graphics, Graphics_LEFT, Graphics_TOP);
				Graphics_printf (my graphics, my endWindow, pitchViewTo_hidden, L"%.4g %ls",
					pitchViewTo_overt,
					ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, my pitch.unit,
						Function_UNIT_TEXT_SHORT | Function_UNIT_TEXT_GRAPHICAL));
			}
		} else {
			Graphics_setTextAlignment (my graphics, Graphics_CENTRE, Graphics_HALF);
			Graphics_text (my graphics, 0.5 * (my startWindow + my endWindow), 0.5 * (pitchViewFrom_hidden + pitchViewTo_hidden),
				L"(Cannot show pitch contour. Zoom out or change bottom of pitch range in pitch settings.)");
		}
		Graphics_setColour (my graphics, Graphics_BLACK);
	}
	if (my intensity.show) {
		double intensityCursor = NUMundefined;
		int intensityCursorVisible;
		int textColour, alignment;
		double y;
		if (! my pitch.show) textColour = Graphics_GREEN, alignment = Graphics_LEFT, y = my endWindow;
		else if (! my spectrogram.show && ! my formant.show) textColour = Graphics_GREEN, alignment = Graphics_RIGHT, y = my startWindow;
		else textColour = my spectrogram.show ? Graphics_LIME : Graphics_GREEN, alignment = Graphics_RIGHT, y = my endWindow;
		Graphics_setWindow (my graphics, my startWindow, my endWindow, my intensity.viewFrom, my intensity.viewTo);
		if (my intensity.data) {
			if (my startSelection == my endSelection) {
				intensityCursor = Vector_getValueAtX (my intensity.data, my startSelection, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR);
			} else {
				intensityCursor = Intensity_getAverage (my intensity.data, my startSelection, my endSelection, my intensity.averagingMethod);
			}
		}
		Graphics_setColour (my graphics, textColour);
		intensityCursorVisible = NUMdefined (intensityCursor) && intensityCursor > my intensity.viewFrom && intensityCursor < my intensity.viewTo;
		if (intensityCursorVisible) {
			static const wchar_t *methodString [] = { L" (.5)", L" (\\muE)", L" (\\muS)", L" (\\mu)" };
			Graphics_setTextAlignment (my graphics, alignment, Graphics_HALF);
			Graphics_printf (my graphics, y, intensityCursor, L"%.4g dB%ls", intensityCursor,
				my startSelection == my endSelection ? L"" : methodString [my intensity.averagingMethod]);
		}
		if (! intensityCursorVisible || Graphics_dyWCtoMM (my graphics, intensityCursor - my intensity.viewFrom) > 5.0) {
			Graphics_setTextAlignment (my graphics, alignment, Graphics_BOTTOM);
			Graphics_printf (my graphics, y, my intensity.viewFrom - Graphics_dyMMtoWC (my graphics, 0.5),
				L"%.4g dB", my intensity.viewFrom);
		}
		if (! intensityCursorVisible || Graphics_dyWCtoMM (my graphics, my intensity.viewTo - intensityCursor) > 5.0) {
			Graphics_setTextAlignment (my graphics, alignment, Graphics_TOP);
			Graphics_printf (my graphics, y, my intensity.viewTo, L"%.4g dB", my intensity.viewTo);
		}
		Graphics_setColour (my graphics, Graphics_BLACK);
	}
	if (my spectrogram.show || my formant.show) {
		wchar_t text [100];
		double textWidth;
		int frequencyCursorVisible = my spectrogram.cursor > my spectrogram.viewFrom && my spectrogram.cursor < my spectrogram.viewTo;
		Graphics_setWindow (my graphics, my startWindow, my endWindow, my spectrogram.viewFrom, my spectrogram.viewTo);
		/*
		 * Range marks.
		 */
		Graphics_setLineType (my graphics, Graphics_DRAWN);
		Graphics_setColour (my graphics, Graphics_BLACK);
		if (! frequencyCursorVisible || Graphics_dyWCtoMM (my graphics, my spectrogram.cursor - my spectrogram.viewFrom) > 5.0) {
			swprintf (text, 100, L"%.4g Hz", my spectrogram.viewFrom);
			textWidth = Graphics_textWidth (my graphics, text) + Graphics_dxMMtoWC (my graphics, 0.5);
			/*Graphics_line (my graphics, my startWindow - textWidth, my spectrogram.viewFrom, my startWindow, my spectrogram.viewFrom);*/
			Graphics_setTextAlignment (my graphics, Graphics_RIGHT, Graphics_BOTTOM);
			Graphics_text (my graphics, my startWindow, my spectrogram.viewFrom - Graphics_dyMMtoWC (my graphics, 0.5), text);
		}
		if (! frequencyCursorVisible || Graphics_dyWCtoMM (my graphics, my spectrogram.viewTo - my spectrogram.cursor) > 5.0) {
			swprintf (text, 100, L"%.4g Hz", my spectrogram.viewTo);
			textWidth = Graphics_textWidth (my graphics, text) + Graphics_dxMMtoWC (my graphics, 0.5);
			/*Graphics_line (my graphics, my startWindow - textWidth, my spectrogram.viewTo, my startWindow, my spectrogram.viewTo);*/
			Graphics_setTextAlignment (my graphics, Graphics_RIGHT, Graphics_TOP);
			Graphics_text (my graphics, my startWindow, my spectrogram.viewTo, text);
		}
		/*
		 * Cursor lines.
		 */
		Graphics_setLineType (my graphics, Graphics_DOTTED);
		Graphics_setColour (my graphics, Graphics_RED);
		if (frequencyCursorVisible) {
			double x = my startWindow, y = my spectrogram.cursor;
			Graphics_setTextAlignment (my graphics, Graphics_RIGHT, Graphics_HALF);
			Graphics_printf (my graphics, x, y, L"%.5g Hz", y);
			Graphics_line (my graphics, x, y, my endWindow, y);
		}
		/*
		if (my startSelection >= my startWindow && my startSelection <= my endWindow)
			Graphics_line (my graphics, my startSelection, my spectrogram.viewFrom, my startSelection, my spectrogram.viewTo);
		if (my endSelection > my startWindow && my endSelection < my endWindow && my endSelection != my startSelection)
			Graphics_line (my graphics, my endSelection, my spectrogram.viewFrom, my endSelection, my spectrogram.viewTo);*/
		/*
		 * Cadre.
		 */
		Graphics_setLineType (my graphics, Graphics_DRAWN);
		Graphics_setColour (my graphics, Graphics_BLACK);
		Graphics_rectangle (my graphics, my startWindow, my endWindow, my spectrogram.viewFrom, my spectrogram.viewTo);
	}
}

void FunctionEditor_SoundAnalysis_drawPulses (I) {
	iam (FunctionEditor);
	FunctionEditor_SoundAnalysis_computePulses (me);
	if (my pulses.show && my endWindow - my startWindow <= my longestAnalysis && my pulses.data != NULL) {
		long i;
		PointProcess point = my pulses.data;
		Graphics_setWindow (my graphics, my startWindow, my endWindow, -1.0, 1.0);
		Graphics_setColour (my graphics, Graphics_BLUE);
		if (point -> nt < 2000) for (i = 1; i <= point -> nt; i ++) {
			double t = point -> t [i];
			if (t >= my startWindow && t <= my endWindow)
				Graphics_line (my graphics, t, -0.9, t, 0.9);
		}
		Graphics_setColour (my graphics, Graphics_BLACK);
	}
}

static int menu_cb_logSettings (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	EDITOR_FORM ("Log settings", "Log files")
		OPTIONMENU ("Write log 1 to", 3)
			OPTION ("Log file only")
			OPTION ("Info window only")
			OPTION ("Log file and Info window")
		LABEL ("", "Log file 1:")
		TEXTFIELDW (L"Log file 1", LOG_1_FILE_NAME)
		LABEL ("", "Log 1 format:")
		TEXTFIELDW (L"Log 1 format", LOG_1_FORMAT)
		OPTIONMENU ("Write log 2 to", 3)
			OPTION ("Log file only")
			OPTION ("Info window only")
			OPTION ("Log file and Info window")
		LABEL ("", "Log file 2:")
		TEXTFIELDW (L"Log file 2", LOG_2_FILE_NAME)
		LABEL ("", "Log 2 format:")
		TEXTFIELDW (L"Log 2 format", LOG_2_FORMAT)
		LABEL ("", "Log script 3:")
		TEXTFIELDW (L"Log script 3", LOG_3_FILE_NAME)
		LABEL ("", "Log script 4:")
		TEXTFIELDW (L"Log script 4", LOG_4_FILE_NAME)
	EDITOR_OK
		SET_INTEGER ("Write log 1 to", preferences.log[0].toLogFile + 2 * preferences.log[0].toInfoWindow)
		SET_STRINGW (L"Log file 1", preferences.log[0].fileName)
		SET_STRINGW (L"Log 1 format", preferences.log[0].format)
		SET_INTEGER ("Write log 2 to", preferences.log[1].toLogFile + 2 * preferences.log[1].toInfoWindow)
		SET_STRINGW (L"Log file 2", preferences.log[1].fileName)
		SET_STRINGW (L"Log 2 format", preferences.log[1].format)
		SET_STRINGW (L"Log script 3", preferences.logScript3)
		SET_STRINGW (L"Log script 4", preferences.logScript4)
	EDITOR_DO
		preferences.log[0].toLogFile = (GET_INTEGER ("Write log 1 to") & 1) != 0;
		preferences.log[0].toInfoWindow = (GET_INTEGER ("Write log 1 to") & 2) != 0;
		wcscpy (preferences.log[0].fileName, GET_STRINGW (L"Log file 1"));
		wcscpy (preferences.log[0].format, GET_STRINGW (L"Log 1 format"));
		preferences.log[1].toLogFile = (GET_INTEGER ("Write log 2 to") & 1) != 0;
		preferences.log[1].toInfoWindow = (GET_INTEGER ("Write log 2 to") & 2) != 0;
		wcscpy (preferences.log[1].fileName, GET_STRINGW (L"Log file 2"));
		wcscpy (preferences.log[1].format, GET_STRINGW (L"Log 2 format"));
		wcscpy (preferences.logScript3, GET_STRINGW (L"Log script 3"));
		wcscpy (preferences.logScript4, GET_STRINGW (L"Log script 4"));
	EDITOR_END
}

static int do_deleteLogFile (FunctionEditor me, int which) {
	structMelderFile file = { 0 };
	(void) me;
	if (! Melder_pathToFileW (preferences.log[which].fileName, & file)) return 0;
	MelderFile_delete (& file);
	return 1;
}
static int menu_cb_deleteLogFile1 (EDITOR_ARGS) { EDITOR_IAM (FunctionEditor); return do_deleteLogFile (me, 0); }
static int menu_cb_deleteLogFile2 (EDITOR_ARGS) { EDITOR_IAM (FunctionEditor); return do_deleteLogFile (me, 1); }

static int do_log (FunctionEditor me, int which) {
	wchar_t format [1000], *p;
	double tmin, tmax;
	int part = makeQueriable (me, TRUE, & tmin, & tmax); iferror return 0;
	wcscpy (format, preferences.log[which].format);
	for (p = format; *p !='\0'; p ++) if (*p == '\'') {
		/*
		 * Found a left quote. Search for a matching right quote.
		 */
		wchar_t *q = p + 1, varName [300], *r, *s, *colon;
		int precision = -1;
		double value = NUMundefined;
		wchar_t *stringValue = NULL;
		while (*q != '\0' && *q != '\'') q ++;
		if (*q == '\0') break;   /* No matching right quote: done with this line. */
		if (q - p == 1) continue;   /* Ignore empty variable names. */
		/*
		 * Found a right quote. Get potential variable name.
		 */
		for (r = p + 1, s = varName; q - r > 0; r ++, s ++) *s = *r;
		*s = '\0';   /* Trailing null byte. */
		colon = wcschr (varName, ':');
		if (colon) {
			precision = wcstol (colon + 1, NULL, 10);
			*colon = '\0';
		}
		if (wcsequ (varName, L"time")) {
			value = 0.5 * (tmin + tmax);
		} else if (wcsequ (varName, L"t1")) {
			value = tmin;
		} else if (wcsequ (varName, L"t2")) {
			value = tmax;
		} else if (wcsequ (varName, L"dur")) {
			value = tmax - tmin;
		} else if (wcsequ (varName, L"freq")) {
			value = my spectrogram.cursor;
		} else if (wcsequ (varName, L"tab$")) {
			stringValue = L"\t";
		} else if (wcsequ (varName, L"editor$")) {
			stringValue = my nameW;
		} else if (wcsequ (varName, L"f0")) {
			if (! my pitch.show)
				return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
			if (! my pitch.data) {
				return Melder_error1 (theMessage_Cannot_compute_pitch);
			}
			if (part == FunctionEditor_PART_CURSOR) {
				value = Pitch_getValueAtTime (my pitch.data, tmin, my pitch.unit, 1);
			} else {
				value = Pitch_getMean (my pitch.data, tmin, tmax, my pitch.unit);
			}
		} else if (varName [0] == 'f' && varName [1] >= '1' && varName [1] <= '5' && varName [2] == '\0') {
			if (! my formant.show)
				return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
			if (! my formant.data) {
				return Melder_error1 (theMessage_Cannot_compute_formant);
			}
			if (part == FunctionEditor_PART_CURSOR) {
				value = Formant_getValueAtTime (my formant.data, varName [1] - '0', tmin, 0);
			} else {
				value = Formant_getMean (my formant.data, varName [1] - '0', tmin, tmax, 0);
			}
		} else if (varName [0] == 'b' && varName [1] >= '1' && varName [1] <= '5' && varName [2] == '\0') {
			if (! my formant.show)
				return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
			if (! my formant.data) {
				return Melder_error1 (theMessage_Cannot_compute_formant);
			}
			value = Formant_getBandwidthAtTime (my formant.data, varName [1] - '0', 0.5 * (tmin + tmax), 0);
		} else if (wcsequ (varName, L"intensity")) {
			if (! my intensity.show)
				return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
			if (! my intensity.data) {
				return Melder_error1 (theMessage_Cannot_compute_intensity);
			}
			if (part == FunctionEditor_PART_CURSOR) {
				value = Vector_getValueAtX (my intensity.data, tmin, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR);
			} else {
				value = Intensity_getAverage (my intensity.data, tmin, tmax, my intensity.averagingMethod);
			}
		} else if (wcsequ (varName, L"power")) {
			if (! my spectrogram.show)
				return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
			if (! my spectrogram.data) {
				return Melder_error1 (theMessage_Cannot_compute_spectrogram);
			}
			if (part != FunctionEditor_PART_CURSOR) return Melder_error1 (L"Click inside the spectrogram first.");
			value = Matrix_getValueAtXY (my spectrogram.data, tmin, my spectrogram.cursor);
		}
		if (NUMdefined (value)) {
			int varlen = (q - p) - 1, headlen = p - format;
			wchar_t formattedNumber [400];
			if (precision >= 0) {
				swprintf (formattedNumber, 400, L"%.*f", precision, value);
			} else {
				swprintf (formattedNumber, 400, L"%.17g", value);
			}
			int arglen = wcslen (formattedNumber);
			static MelderStringW buffer = { 0 };
			MelderStringW_ncopyW (& buffer, format, headlen);
			MelderStringW_append2 (& buffer, formattedNumber, p + varlen + 2);
			wcscpy (format, buffer.string);
			p += arglen - 1;
		} else if (stringValue != NULL) {
			int varlen = (q - p) - 1, headlen = p - format, arglen = wcslen (stringValue);
			static MelderStringW buffer = { 0 };
			MelderStringW_ncopyW (& buffer, format, headlen);
			MelderStringW_append2 (& buffer, stringValue, p + varlen + 2);
			wcscpy (format, buffer.string);
			p += arglen - 1;
		} else {
			p = q - 1;   /* Go to before next quote. */
		}
	}
	if (preferences.log[which].toInfoWindow) {
		MelderInfo_write1 (format);
		MelderInfo_close ();
	}
	if (preferences.log[which].toLogFile) {
		structMelderFile file = { 0 };
		wcscat (format, L"\n");
		if (! Melder_relativePathToFile (preferences.log[which].fileName, & file)) return 0;
		if (! MelderFile_appendText (& file, format)) return 0;
	}
	return 1;
}

static int menu_cb_log1 (EDITOR_ARGS) { EDITOR_IAM (FunctionEditor); return do_log (me, 0); }
static int menu_cb_log2 (EDITOR_ARGS) { EDITOR_IAM (FunctionEditor); return do_log (me, 1); }

static int menu_cb_logScript3 (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	return DO_RunTheScriptFromAnyAddedEditorCommand (me, preferences.logScript3);
}
static int menu_cb_logScript4 (EDITOR_ARGS) {
	EDITOR_IAM (FunctionEditor);
	return DO_RunTheScriptFromAnyAddedEditorCommand (me, preferences.logScript4);
}

void FunctionEditor_SoundAnalysis_viewMenus (I) {
	iam (FunctionEditor);
	Editor_addCommand (me, L"View", L"Analysis window:", motif_INSENSITIVE, menu_cb_showAnalyses);
	Editor_addCommand (me, L"View", L"Show analyses...", 0, menu_cb_showAnalyses);
	Editor_addCommand (me, L"View", L"Time step settings...", 0, menu_cb_timeStepSettings);
	Editor_addCommand (me, L"View", L"-- sound analysis --", 0, 0);
}

void FunctionEditor_SoundAnalysis_selectionQueries (I) {
	iam (FunctionEditor);
	Editor_addCommand (me, L"Query", L"-- query log --", 0, NULL);
	Editor_addCommand (me, L"Query", L"Log settings...", 0, menu_cb_logSettings);
	Editor_addCommand (me, L"Query", L"Delete log file 1", 0, menu_cb_deleteLogFile1);
	Editor_addCommand (me, L"Query", L"Delete log file 2", 0, menu_cb_deleteLogFile2);
	Editor_addCommand (me, L"Query", L"Log 1", motif_F12, menu_cb_log1);
	Editor_addCommand (me, L"Query", L"Log 2", motif_F12 + motif_SHIFT, menu_cb_log2);
	Editor_addCommand (me, L"Query", L"Log script 3 (...)", motif_F12 + motif_OPTION, menu_cb_logScript3);
	Editor_addCommand (me, L"Query", L"Log script 4 (...)", motif_F12 + motif_COMMAND, menu_cb_logScript4);
}

void FunctionEditor_SoundAnalysis_addMenus (I) {
	iam (FunctionEditor);
	EditorMenu menu;

	menu = Editor_addMenu (me, L"Spectrum", 0);
	my spectrogramToggle = EditorMenu_addCommand (menu, L"Show spectrogram",
		motif_CHECKABLE | (preferences.spectrogram.show ? motif_CHECKED : 0), menu_cb_showSpectrogram);
	EditorMenu_addCommand (menu, L"Spectrogram settings...", 0, menu_cb_spectrogramSettings);
	EditorMenu_addCommand (menu, L"Advanced spectrogram settings...", 0, menu_cb_advancedSpectrogramSettings);
	EditorMenu_addCommand (menu, L"-- query spectrogram --", 0, NULL);
	EditorMenu_addCommand (menu, L"Query:", motif_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	EditorMenu_addCommand (menu, L"Get frequency at frequency cursor", 0, menu_cb_getFrequency);
	EditorMenu_addCommand (menu, L"Get spectral power at cursor cross", motif_F7, menu_cb_getSpectralPowerAtCursorCross);
	EditorMenu_addCommand (menu, L"-- copy spectrum to objects window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Copy to objects window:", motif_INSENSITIVE, menu_cb_extractVisibleSpectrogram /* dummy */);
	EditorMenu_addCommand (menu, L"Extract visible spectrogram", 0, menu_cb_extractVisibleSpectrogram);
	EditorMenu_addCommand (menu, L"View spectral slice", 'L', menu_cb_viewSpectralSlice);
	EditorMenu_addCommand (menu, L"-- draw spectrum to picture window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Draw to picture window:", motif_INSENSITIVE, menu_cb_drawVisibleSpectrogram /* dummy */);
	EditorMenu_addCommand (menu, L"Draw visible spectrogram...", 0, menu_cb_drawVisibleSpectrogram);

	menu = Editor_addMenu (me, L"Pitch", 0);
	my pitchToggle = EditorMenu_addCommand (menu, L"Show pitch",
		motif_CHECKABLE | (preferences.pitch.show ? motif_CHECKED : 0), menu_cb_showPitch);
	EditorMenu_addCommand (menu, L"Pitch settings...", 0, menu_cb_pitchSettings);
	EditorMenu_addCommand (menu, L"Advanced pitch settings...", 0, menu_cb_advancedPitchSettings);
	EditorMenu_addCommand (menu, L"-- query pitch --", 0, NULL);
	EditorMenu_addCommand (menu, L"Query:", motif_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	EditorMenu_addCommand (menu, L"Pitch listing", 0, menu_cb_pitchListing);
	EditorMenu_addCommand (menu, L"Get pitch", motif_F5, menu_cb_getPitch);
	EditorMenu_addCommand (menu, L"Get minimum pitch", motif_F5 + motif_COMMAND, menu_cb_getMinimumPitch);
	EditorMenu_addCommand (menu, L"Get maximum pitch", motif_F5 + motif_SHIFT, menu_cb_getMaximumPitch);
	EditorMenu_addCommand (menu, L"-- select pitch --", 0, NULL);
	EditorMenu_addCommand (menu, L"Select:", motif_INSENSITIVE, menu_cb_moveCursorToMinimumPitch /* dummy */);
	EditorMenu_addCommand (menu, L"Move cursor to minimum pitch", motif_COMMAND + motif_SHIFT + 'L', menu_cb_moveCursorToMinimumPitch);
	EditorMenu_addCommand (menu, L"Move cursor to maximum pitch", motif_COMMAND + motif_SHIFT + 'H', menu_cb_moveCursorToMaximumPitch);
	EditorMenu_addCommand (menu, L"-- copy pitch to objects window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Copy to objects window:", motif_INSENSITIVE, menu_cb_extractVisiblePitchContour /* dummy */);
	EditorMenu_addCommand (menu, L"Extract visible pitch contour", 0, menu_cb_extractVisiblePitchContour);
	EditorMenu_addCommand (menu, L"-- draw pitch to picture window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Draw to picture window:", motif_INSENSITIVE, menu_cb_drawVisiblePitchContour /* dummy */);
	EditorMenu_addCommand (menu, L"Draw visible pitch contour...", 0, menu_cb_drawVisiblePitchContour);

	menu = Editor_addMenu (me, L"Intensity", 0);
	my intensityToggle = EditorMenu_addCommand (menu, L"Show intensity",
		motif_CHECKABLE | (preferences.intensity.show ? motif_CHECKED : 0), menu_cb_showIntensity);
	EditorMenu_addCommand (menu, L"Intensity settings...", 0, menu_cb_intensitySettings);
	EditorMenu_addCommand (menu, L"-- query intensity --", 0, NULL);
	EditorMenu_addCommand (menu, L"Query:", motif_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	EditorMenu_addCommand (menu, L"Intensity listing", 0, menu_cb_intensityListing);
	EditorMenu_addCommand (menu, L"Get intensity", motif_F8, menu_cb_getIntensity);
	EditorMenu_addCommand (menu, L"-- copy intensity to objects window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Copy to objects window:", motif_INSENSITIVE, menu_cb_extractVisibleIntensityContour /* dummy */);
	EditorMenu_addCommand (menu, L"Extract visible intensity contour", 0, menu_cb_extractVisibleIntensityContour);

	menu = Editor_addMenu (me, L"Formant", 0);
	my formantToggle = EditorMenu_addCommand (menu, L"Show formants",
		motif_CHECKABLE | (preferences.formant.show ? motif_CHECKED : 0), menu_cb_showFormants);
	EditorMenu_addCommand (menu, L"Formant settings...", 0, menu_cb_formantSettings);
	EditorMenu_addCommand (menu, L"Advanced formant settings...", 0, menu_cb_advancedFormantSettings);
	EditorMenu_addCommand (menu, L"-- query formants --", 0, NULL);
	EditorMenu_addCommand (menu, L"Query:", motif_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	EditorMenu_addCommand (menu, L"Formant listing", 0, menu_cb_formantListing);
	EditorMenu_addCommand (menu, L"Get first formant", motif_F1, menu_cb_getFirstFormant);
	EditorMenu_addCommand (menu, L"Get first bandwidth", 0, menu_cb_getFirstBandwidth);
	EditorMenu_addCommand (menu, L"Get second formant", motif_F2, menu_cb_getSecondFormant);
	EditorMenu_addCommand (menu, L"Get second bandwidth", 0, menu_cb_getSecondBandwidth);
	EditorMenu_addCommand (menu, L"Get third formant", motif_F3, menu_cb_getThirdFormant);
	EditorMenu_addCommand (menu, L"Get third bandwidth", 0, menu_cb_getThirdBandwidth);
	EditorMenu_addCommand (menu, L"Get fourth formant", motif_F4, menu_cb_getFourthFormant);
	EditorMenu_addCommand (menu, L"Get fourth bandwidth", 0, menu_cb_getFourthBandwidth);
	EditorMenu_addCommand (menu, L"Get formant...", 0, menu_cb_getFormant);
	EditorMenu_addCommand (menu, L"Get bandwidth...", 0, menu_cb_getBandwidth);
	EditorMenu_addCommand (menu, L"-- copy formant to objects window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Copy to objects window:", motif_INSENSITIVE, menu_cb_extractVisibleFormantContour /* dummy */);
	EditorMenu_addCommand (menu, L"Extract visible formant contour", 0, menu_cb_extractVisibleFormantContour);

	menu = Editor_addMenu (me, L"Pulses", 0);
	my pulsesToggle = EditorMenu_addCommand (menu, L"Show pulses",
		motif_CHECKABLE | (preferences.pulses.show ? motif_CHECKED : 0), menu_cb_showPulses);
	EditorMenu_addCommand (menu, L"Advanced pulses settings...", 0, menu_cb_advancedPulsesSettings);
	EditorMenu_addCommand (menu, L"-- query pulses --", 0, NULL);
	EditorMenu_addCommand (menu, L"Query:", motif_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	EditorMenu_addCommand (menu, L"Voice report", 0, menu_cb_voiceReport);
	EditorMenu_addCommand (menu, L"Pulse listing", 0, menu_cb_pulseListing);
	/*
	EditorMenu_addCommand (menu, L"Get jitter (local)", 0, cb_getJitter_local);
	EditorMenu_addCommand (menu, L"Get jitter (local, absolute)", 0, cb_getJitter_local_absolute);
	EditorMenu_addCommand (menu, L"Get jitter (rap)", 0, cb_getJitter_rap);
	EditorMenu_addCommand (menu, L"Get jitter (ppq5)", 0, cb_getJitter_ppq5);
	EditorMenu_addCommand (menu, L"Get jitter (ddp)", 0, cb_getJitter_ddp);
	EditorMenu_addCommand (menu, L"Get shimmer (local)", 0, cb_getShimmer_local);
	EditorMenu_addCommand (menu, L"Get shimmer (local_dB)", 0, cb_getShimmer_local_dB);
	EditorMenu_addCommand (menu, L"Get shimmer (apq3)", 0, cb_getShimmer_apq3);
	EditorMenu_addCommand (menu, L"Get shimmer (apq5)", 0, cb_getShimmer_apq5);
	EditorMenu_addCommand (menu, L"Get shimmer (apq11)", 0, cb_getShimmer_apq11);
	EditorMenu_addCommand (menu, L"Get shimmer (dda)", 0, cb_getShimmer_dda);
	*/
	EditorMenu_addCommand (menu, L"-- copy pulses to objects window --", 0, NULL);
	EditorMenu_addCommand (menu, L"Copy to objects window:", motif_INSENSITIVE, menu_cb_extractVisiblePulses /* dummy */);
	EditorMenu_addCommand (menu, L"Extract visible pulses", 0, menu_cb_extractVisiblePulses);
}

void FunctionEditor_SoundAnalysis_init (I) {
	iam (FunctionEditor);
	my longestAnalysis = preferences.longestAnalysis;
	if (preferences.timeStepStrategy < 1 || preferences.timeStepStrategy > 3)
		preferences.timeStepStrategy = 1;
	if (preferences.log[0].toLogFile == FALSE && preferences.log[0].toInfoWindow == FALSE)
		preferences.log[0].toLogFile = TRUE, preferences.log[0].toInfoWindow = TRUE;
	if (preferences.log[1].toLogFile == FALSE && preferences.log[1].toInfoWindow == FALSE)
		preferences.log[1].toLogFile = TRUE, preferences.log[1].toInfoWindow = TRUE;
	my timeStepStrategy = preferences.timeStepStrategy;
	my fixedTimeStep = preferences.fixedTimeStep;
	my numberOfTimeStepsPerView = preferences.numberOfTimeStepsPerView;
	my spectrogram = preferences.spectrogram;
	my pitch = preferences.pitch;
	my intensity = preferences.intensity;
	my formant = preferences.formant;
	my pulses = preferences.pulses;
}

/* End of file FunctionEditor_SoundAnalysis.c */
