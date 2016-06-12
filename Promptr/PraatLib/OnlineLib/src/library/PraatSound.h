#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>
#include <limits>

#ifdef MAKEPRAATLIB
#include <sys/time.h>
extern "C" {
	// Praat Stuff
	#include "melder.h"
	#include "Sound.h"
	#include "Pitch.h"
	#include "Sound_to_PointProcess.h"
	#include "Pitch_to_PointProcess.h"
	#include "Pitch_to_PitchTier.h"
	#include "Sound_to_Pitch.h"
	#include "VoiceAnalysis.h"
	#include "AmplitudeTier.h"
	#include "Harmonicity.h"
	#include "Sound_to_Harmonicity.h"
	#include "Sound_to_Intensity.h"
	#include "RealTier.h"
	#include "PointProcess.h"
	#include "Sound_to_Formant.h"
	#include "Formant.h"
	#include "Intensity.h"
	#include "Sampled.h"
	#include "Spectrum.h"
	#include "Ltas.h"
	#include "Sound_and_Spectrum.h"
}

#define THROW(errno, errdesc) { if(Melder_hasError()) { fprintf(stderr, "%s", Melder_getError()); Melder_clearError(); } fprintf(stderr, "%d %s, line %d in %s\n", errno, errdesc, __LINE__, __FILE__); throw(errno); }

#else

#define Sound void*
#define Sampled void*
#define Pitch void*
#define PointProcess void*
#define AmplitudeTier void*
#define Harmonicity void*
#define PitchTier void*
#define Formant void*
#define Intensity void*
#define Spectrum void*
#define Ltas void*

#endif


class PraatSound
{
public:

	enum WindowFunctions {wfRectangular, wfTriangular, wfParabolic, wfHanning, wfHamming, wfGaussian1, wfGaussian2, wfGaussian3, wfGaussian4, wfGaussian5, wfKaiser1, wfKaiser2};
	enum RawEncodings {reUnknown, reLinear8bitSigned, reLinear8bitUnsigned, reLinear16bitBE, reLinear16bitLE, reLinear24bitBE, reLinear24bitLE, reLinear32bitBE, reLinear32bitLE, reMuLaw, reALaw, reShorten, rePolyphone, reFloat32bitBE, reFloat32bitLE, reFlac, reMPEG};

	struct SegmentDef
	{
	
		SegmentDef(int id = -1);
		virtual ~SegmentDef();
	
		int id;
		double start;
		double end;
		WindowFunctions func;
		
		Sound snd;
		Pitch pitch; double pitch_fmin, pitch_fmax;
		PointProcess pp; Pitch pp_pitch;
		PitchTier pitcht; Pitch pitcht_pitch;
		AmplitudeTier peaks; PointProcess peaks_pp; double peaks_pmin, peaks_pmax, peaks_mpf;
		Harmonicity harm; bool harm_ac; double harm_step, harm_fmin, harm_silence_threshold, harm_periods_per_window;
		Intensity intens; bool intens_subtract_mean; double intens_step, intens_fmin;
		Formant form; int form_maxform; double form_step, form_fmax, form_window_length, form_preemphasis_frequency;
		Spectrum spec;
		Ltas ltas; Spectrum ltas_spec; double ltas_bandwidth;
	};

	// Creates a sound from a file *with* header that is supported by Praat
	// (WAV, AU, SPH,...)
	PraatSound(char* filename);
	PraatSound(wchar_t* filename);
	// Creates a sound from raw (i.e. without header) data.
	// The format/encoding must be specified.
	PraatSound(char* rawdata, int length, int sample_rate = 16000, int channels = 1, RawEncodings encoding = reLinear16bitBE);
	
	virtual ~PraatSound();
	
	// Reads segments from an 'RTTM' format file, overriding any existing parts.
	// If a window function should be applied to all segments, specify it here. Note that
	// using a window function makes computation slower and requires up to twice the memory.
	void SegmentsReadRTTM(char* filename, WindowFunctions func = PraatSound::wfRectangular);
	// Resets the list of segments, i.e. deletes all segments/
	void SegmentsClear(int capacity);
	// Adds a new segment using the given time range and window function.
	// Note: No window function (0) is faster and less memory-consuming.
	void SegmentsAdd(double start, double end, WindowFunctions func = PraatSound::wfRectangular);
	
	// Retrieves multiple features by name.
	// 'features' is an array of feature names.
	// 'segment' can be segment index, '-1' for full waveform, or '-2' (default) for all
	// segments (or same as '-1' if no segments are defined).
	// Return value is an array of 'double' values.
	// To benchmark, pre-initialize 'times' with an array of the same size as the
	// expected return array. It will contain one duration (in seconds) for each
	// feature.
	double* GetMultiple(char** features, int num_features, int segment = -2, double* times = NULL);
	
	
	double GetPitchMean(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchMedian(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchMin(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchMax(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchRange(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchStdDev(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchMAS(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool no_octave = true);
	double GetPitchVoiced(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	
	double GetPitchCandidatesMean(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchCandidatesMedian(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchCandidatesMin(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchCandidatesMax(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchCandidatesRange(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchCandidatesStdDev(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	
	double GetPitchStrengthMean(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchStrengthMedian(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchStrengthMin(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchStrengthMax(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchStrengthRange(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchStrengthStdDev(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	
	double GetPitchEnergyMean(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchEnergyMedian(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchEnergyMin(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchEnergyMax(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchEnergyRange(int segment = -1, double fmin = 75.0, double fmax = 600.0, bool quant = true);
	double GetPitchEnergyStdDev(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	
	double GetPitchTierNumSamples(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchTierMean(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	//double GetPitchTierMeanPoints(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	double GetPitchTierStdDev(int segment = -1, double fmin = 75.0, double fmax = 600.0);
	//double GetPitchTierStdDevPoints(int segment = -1, double fmin = 75.0, double fmax = 600.0);

	double GetPointProcessNumSamples(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetPointProcessNumPeriods(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetPointProcessPeriodMean(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetPointProcessPeriodStdDev(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	
	double GetJitterRAP(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetJitterPPQ5(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetJitterLocal(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetJitterLocalAbs(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);
	double GetJitterDDP(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3);

	double GetShimmerAPQ3(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3, double maximum_amplitude_factor = 1.6);
	double GetShimmerAPQ5(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3, double maximum_amplitude_factor = 1.6);
	double GetShimmerAPQ11(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3, double maximum_amplitude_factor = 1.6);
	double GetShimmerLocal(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3, double maximum_amplitude_factor = 1.6);
	double GetShimmerLocalDb(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3, double maximum_amplitude_factor = 1.6);
	double GetShimmerDDA(int segment = -1, double fmin = 75.0, double fmax = 600.0, double pmin = 0.0001, double pmax = 0.02, double maximum_period_factor = 1.3, double maximum_amplitude_factor = 1.6);

	double GetHarmonicityMean(int segment = -1, bool ac = true, double step = 0.01, double fmin = 75.0, double silence_threshold = 0.1, double periods_per_window = 4.5);
	double GetHarmonicityMedian(int segment = -1, bool ac = true, double step = 0.01, double fmin = 75.0, double silence_threshold = 0.1, double periods_per_window = 4.5);
	double GetHarmonicityMin(int segment = -1, bool ac = true, double step = 0.01, double fmin = 75.0, double silence_threshold = 0.1, double periods_per_window = 4.5, bool quant = true);
	double GetHarmonicityMax(int segment = -1, bool ac = true, double step = 0.01, double fmin = 75.0, double silence_threshold = 0.1, double periods_per_window = 4.5, bool quant = true);
	double GetHarmonicityRange(int segment = -1, bool ac = true, double step = 0.01, double fmin = 75.0, double silence_threshold = 0.1, double periods_per_window = 4.5, bool quant = true);
	double GetHarmonicityStdDev(int segment = -1, bool ac = true, double step = 0.01, double fmin = 75.0, double silence_threshold = 0.1, double periods_per_window = 4.5);

	double GetFormantMean(int segment = -1, int formant = 1, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0);
	double GetFormantMedian(int segment = -1, int formant = 1, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0);
	double GetFormantMin(int segment = -1, int formant = 1, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0, bool quant = true);
	double GetFormantMax(int segment = -1, int formant = 1, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0, bool quant = true);
	double GetFormantRange(int segment = -1, int formant = 1, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0, bool quant = true);
	double GetFormantStdDev(int segment = -1, int formant = 1, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0);

	double GetFormantsDispMean(int segment = -1, int formants = 5, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0);
	double GetFormantsDispMedian(int segment = -1, int formants = 5, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0);
	double GetFormantsDispMin(int segment = -1, int formants = 5, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0, bool quant = true);
	double GetFormantsDispMax(int segment = -1, int formants = 5, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0, bool quant = true);
	double GetFormantsDispRange(int segment = -1, int formants = 5, double step = 0.0, double fmax = 5500.0, double window_length = 0.025, double preemphasis_frequency = 50.0, bool quant = true);

	double GetEnergyMean(int segment = -1, double step = 0.0, double fmin = 100.0, bool subtract_mean = true);
	double GetEnergyMedian(int segment = -1, double step = 0.0, double fmin = 100.0, bool subtract_mean = true);
	double GetEnergyMin(int segment = -1, double step = 0.0, double fmin = 100.0, bool subtract_mean = true, bool quant = true);
	double GetEnergyMax(int segment = -1, double step = 0.0, double fmin = 100.0, bool subtract_mean = true, bool quant = true);
	double GetEnergyRange(int segment = -1, double step = 0.0, double fmin = 100.0, bool subtract_mean = true, bool quant = true);
	double GetEnergyStdDev(int segment = -1, double step = 0.0, double fmin = 100.0, bool subtract_mean = true);

	double GetLtasEnergyMean(int segment = -1, double fmin = 0.0, double fmax = 0.0, double bandwidth = 100.0);
	double GetLtasEnergyMin(int segment = -1, double fmin = 0.0, double fmax = 0.0, double bandwidth = 100.0);
	double GetLtasEnergyMax(int segment = -1, double fmin = 0.0, double fmax = 0.0, double bandwidth = 100.0);
	double GetLtasEnergyRange(int segment = -1, double fmin = 0.0, double fmax = 0.0, double bandwidth = 100.0);
	double GetLtasEnergyStdDev(int segment = -1, double fmin = 0.0, double fmax = 0.0, double bandwidth = 100.0);
	double GetLtasEnergySlope(int segment = -1, double flow_min = 0.0, double flow_max = 1000.0, double fhigh_min = 1000.0, double fhigh_max = 4000.0, double bandwidth = 100.0);
	double GetLtasEnergyLocalPeakHeight(int segment = -1, double environment_min = 1700.0, double environment_max = 4200.0, double peak_min = 2400.0, double peak_max = 3200.0, double bandwidth = 100.0);
	//double GetLtasHarmonicsRatio(int segment = -1, double bandwidth = 100.0, int hmax = 4);
	
	
	int GetNumSegments();
	double GetDuration();
	
	
	// -------------- Praat Parameters --------------
	
	bool p_Quant; // Use quantiles instead of absolute min/max (where possible)?
	double p_QuantMin, p_QuantMax;
	double p_Pitch_fmin, p_Pitch_fmax;
	double p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor;
	double p_AT_maximum_amplitude_factor;
	bool p_Harm_ac; double p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window;
	int p_Form_max; double p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency;
	bool p_Intens_subtract_mean; double p_Intens_step, p_Intens_fmin;
	int p_Ltas_hmax; double p_Ltas_bandwidth, p_Ltas_fmin, p_Ltas_fmax, p_Ltas_flow_min, p_Ltas_flow_max;
	double p_Ltas_fhigh_min, p_Ltas_fhigh_max, p_Ltas_environment_min, p_Ltas_environment_max, p_Ltas_peak_min, p_Ltas_peak_max;
	
private:

	SegmentDef whole;

	SegmentDef** m_segments;
	int m_num_segments;

	/*	
	Sound m_snd;
	Pitch m_pitch; Sound m_pitch_snd; double m_pitch_fmin, m_pitch_fmax;
	PointProcess m_pp;
	AmplitudeTier m_peaks;
	Harmonicity m_harm;
	Intensity m_intens;
	*/
	
	
	void InitInt();
	void ReadSoundInt(wchar_t* filename);
	void GetRange(int segment, double& min, double& max);
	SegmentDef* GetSegment(int segment, bool no_range = false);
	void GetSampledBounds(Sampled obj, double tmin, double tmax, int& imin, int& imax);
	double currenttime();
	
	Pitch UpdatePitch(int segment, double fmin, double fmax, bool no_range = false);
	PointProcess UpdatePointProcess(int segment, double fmin, double fmax, bool no_range = false);
	AmplitudeTier UpdateAmplitudeTier(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor);
	Harmonicity UpdateHarmonicity(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window, bool no_range = false);
	PitchTier UpdatePitchTier(int segment, double fmin, double fmax, bool no_range = false);
	Formant UpdateFormant(int segment, double step, double fmax, double window_length, double preemphasis_frequency, bool no_range = false);
	Intensity UpdateIntensity(int segment, double step, double fmin, bool subtract_mean, bool no_range = false);
	Spectrum UpdateSpectrum(int segment, bool no_range = false);
	Ltas UpdateLtas(int segment, double bandwidth, bool no_range = false);
	
	double* PitchCandidatesToArray(Pitch obj, double tmin, double tmax, int& length);
	double* PitchIntensityToArray(Pitch obj, double tmin, double tmax, int& length);
	double* PitchStrengthMeanToArray(Pitch obj, double tmin, double tmax, int& length);
	double GetPitchFrameFeature (int segment, double fmin, double fmax, int feat, int measure, bool alt);
	
	double ArrGetMean(double* arr, int length);
	double ArrGetQuantile(double* arr, int length, double pos);
	double ArrGetMin(double* arr, int length);
	double ArrGetMax(double* arr, int length);
	double ArrGetRange(double* arr, int length);
	double ArrGetStdDev(double* arr, int length);
	
};


