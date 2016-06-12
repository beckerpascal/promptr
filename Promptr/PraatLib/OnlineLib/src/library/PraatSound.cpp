#include "PraatSound.h"

using namespace std;


PraatSound::SegmentDef::SegmentDef(int id)
	: id(id), start(0), end(0)
{
	func = wfRectangular;
	snd = NULL;
	pitch = NULL; pp = NULL; pitcht = NULL; intens = NULL; harm = NULL; peaks = NULL; form = NULL; spec = NULL; ltas = NULL;
	pp_pitch = NULL;
	peaks_pp = NULL;
	ltas_spec = NULL;
}

PraatSound::SegmentDef::~SegmentDef()
{
#ifdef DEBUG
	printf("   Deleting segment %i range %f .. %f...\n", id, start, end);
#endif
	if ((int)intens == -1) intens = NULL; if (intens) forget(intens);
	if ((int)harm == -1) harm = NULL; if (harm) forget(harm);
	if ((int)peaks == -1) peaks = NULL; if (peaks) forget(peaks);
	if ((int)pp == -1) pp = NULL; if (pp) forget(pp);
	if ((int)pitcht == -1) pitcht = NULL; if (pitcht) forget(pitcht);
	if ((int)pitch == -1) pitch = NULL; if (pitch) forget(pitch);
	if ((int)form == -1) form = NULL; if (form) forget(form);
	if ((int)ltas == -1) ltas = NULL; if (ltas) forget(ltas);
	if ((int)spec == -1) spec = NULL; if (spec) forget(spec);
	if (snd) forget(snd);
#ifdef DEBUG
	printf("   ...done.\n", id, start, end);
#endif
}


PraatSound::PraatSound(wchar_t* filename)
{
	ReadSoundInt(filename);
}

PraatSound::PraatSound(char* filename)
{
	// Convert filename to WCHAR
#ifdef DEBUG
	printf("File: %s \n", filename);
#endif
	//if (setlocale(LC_CTYPE, "en_US") == NULL) THROW(1, "Error - SetLocale failed");
	int len = strlen(filename);
	int reqlen = mbstowcs(NULL, filename, len);
	if (len != reqlen) THROW(1, "Error - unexpected error converting filename to Unicode");
	wchar_t* filename2 = new wchar_t[len + 1]; // +1 because length does not include '0'
	memset(filename2, 0, (len+1)*sizeof(wchar_t));
	reqlen = mbstowcs(filename2, filename, len);
	ReadSoundInt(filename2);
	delete[] filename2;
}

PraatSound::PraatSound(char* rawdata, int length, int samprate, int channels, RawEncodings encoding)
{
	// Parse the audio data into a Praat sound object
	whole.snd = NULL;
	int bitrate;
	switch (encoding) {
		case reLinear8bitSigned:
		case reLinear8bitUnsigned:
			bitrate = 8;
			break;
		case reLinear16bitBE:
		case reLinear16bitLE:
		case reMuLaw:
		case reALaw:
			bitrate = 16;
			break;
		case reLinear24bitBE:
		case reLinear24bitLE:
			bitrate = 24;
			break;
		case reLinear32bitBE:
		case reLinear32bitLE:
		case reFloat32bitBE:
		case reFloat32bitLE:
			bitrate = 32;
			break;
		default:
			THROW(1, "Error - The specified encoding is not supported by this library.");
	}
#ifdef DEBUG
	printf("Reading %ib raw audio data (%i Hz, %i bits/sec, %i channels)...\n", length, samprate, bitrate, channels);
#endif
	long numberOfChannels = channels;
	long numberOfSamples = length / (channels * bitrate / 8);
	double sampleRate = double(samprate);;
	whole.snd = Sound_createSimple(numberOfChannels, numberOfSamples / sampleRate, sampleRate);
	MemFileBuf mfb = mfopen(rawdata, length, 0);
	if (channels == 1) {
		Melder_readAudioToFloatB(mfb, numberOfChannels, int(encoding), whole.snd->z[1], NULL, numberOfSamples);
	} else if (channels = 2) {
		Melder_readAudioToFloatB(mfb, numberOfChannels, int(encoding), whole.snd->z[1], whole.snd->z[2], numberOfSamples);
	} else {
		THROW(1, "Error - Invalid value for 'channels' parameter!");
	}
	mfclose(mfb);
	//iferror { Melder_error ("(Sound_readFromRawAlawFile:) File %s not read.", MelderFile_messageName (file)); forget (me); }
	InitInt();
}


void PraatSound::ReadSoundInt(wchar_t* filename)
{
	structMelderFile file;
	wcscpy (file.path, filename);
  
	// Read the audio file
#ifdef DEBUG
	printf("Reading file...\n");
#endif
	whole.snd = Sound_readFromSoundFile(&file); // in "Sound_files.c"
	if (whole.snd == 0) {
		MelderFile_close(&file);
		THROW(5, "Error - Praat was unable to read the file!");
	}
	// delete file; //?
	
	InitInt();
}

void PraatSound::InitInt()
{
	whole.start = whole.snd->xmin;
	whole.end = whole.snd->xmax;
	whole.func = wfRectangular;
	
	m_segments = NULL;
	p_Quant = true;
	p_QuantMin = 0.05; p_QuantMax = 0.95;
	p_Pitch_fmin = 75.0; p_Pitch_fmax = 600.0;
	p_PP_pmin = 0.0001; p_PP_pmax = 0.02; p_PP_maximum_period_factor = 1.3;
	p_AT_maximum_amplitude_factor = 1.6;
	p_Harm_ac = true; p_Harm_step = 0.01; p_Harm_fmin = 75.0; p_Harm_silence_threshold = 0.1; p_Harm_periods_per_window = 4.5;
	p_Form_max = 5; p_Form_step = 0.0; p_Form_fmax = 5500.0; p_Form_window_length = 0.025; p_Form_preemphasis_frequency = 50.0;
	p_Intens_subtract_mean = true; p_Intens_step = 0.0; p_Intens_fmin = 100.0;
	p_Ltas_hmax = 4; p_Ltas_bandwidth = 100.0; p_Ltas_fmin = 0.0; p_Ltas_fmax = 0.0;
	p_Ltas_flow_min = 0.0; p_Ltas_flow_max = 1000.0; p_Ltas_fhigh_min = 1000.0; p_Ltas_fhigh_max = 4000.0;
	p_Ltas_environment_min = 1700.0; p_Ltas_environment_max = 4200.0; p_Ltas_peak_min = 2400.0; p_Ltas_peak_max = 3200.0;
}


PraatSound::~PraatSound()
{
#ifdef DEBUG
	printf("Cleaning up PraatSound object...\n");
#endif
	SegmentsClear(0); // deletes 'm_segments'
#ifdef DEBUG
	printf("Main destructor finished.\n");
#endif
}



double* PraatSound::GetMultiple(char** features, int num_features, int segment, double* times)
{
	double* ret;
	if (segment == -2) {
		if (m_segments == NULL || m_num_segments == 0) {
			return GetMultiple(features, num_features, -1, times);
		} else {
			ret = new double[num_features * m_num_segments];
			int vecsize = num_features * (int)sizeof(double);
#ifdef DEBUG
			printf("Getting %i features from %i segments, vecsize is %i \n", num_features, m_num_segments, vecsize);
#endif
			for (int i=0; i<m_num_segments; i++) {
				double* times2 = NULL;
				if (times != NULL) times2 = times + num_features * i;
				double* tmpret = GetMultiple(features, num_features, i, times2);
				memcpy(ret + num_features * i, tmpret, vecsize);
				delete[] tmpret;
			}
			return ret;
		}
	}
	ret = new double[num_features];
	double tstart;
	for (int i=0; i<num_features; i++) {
		tstart = currenttime();
#ifdef DEBUG
			printf("Getting feature %i/%i '%s' \n", i+1, num_features, features[i]);
#endif
		if (strcmp(features[i], "f0_mean") == 0) ret[i] = GetPitchMean(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_med") == 0) ret[i] = GetPitchMedian(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_min") == 0) ret[i] = GetPitchMin(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_max") == 0) ret[i] = GetPitchMax(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_range") == 0) ret[i] = GetPitchRange(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_stddev") == 0) ret[i] = GetPitchStdDev(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_mas") == 0) ret[i] = GetPitchMAS(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_voiced") == 0) ret[i] = GetPitchVoiced(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_cand_mean") == 0) ret[i] = GetPitchCandidatesMean(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_cand_med") == 0) ret[i] = GetPitchCandidatesMedian(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_cand_min") == 0) ret[i] = GetPitchCandidatesMin(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_cand_max") == 0) ret[i] = GetPitchCandidatesMax(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_cand_range") == 0) ret[i] = GetPitchCandidatesRange(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_cand_stddev") == 0) ret[i] = GetPitchCandidatesStdDev(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_str_mean") == 0) ret[i] = GetPitchStrengthMean(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_str_med") == 0) ret[i] = GetPitchStrengthMedian(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_str_min") == 0) ret[i] = GetPitchStrengthMin(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_str_max") == 0) ret[i] = GetPitchStrengthMax(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_str_range") == 0) ret[i] = GetPitchStrengthRange(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_str_stddev") == 0) ret[i] = GetPitchStrengthStdDev(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_en_mean") == 0) ret[i] = GetPitchEnergyMean(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_en_med") == 0) ret[i] = GetPitchEnergyMedian(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_en_min") == 0) ret[i] = GetPitchEnergyMin(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_en_max") == 0) ret[i] = GetPitchEnergyMax(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_en_range") == 0) ret[i] = GetPitchEnergyRange(segment, p_Pitch_fmin, p_Pitch_fmax, p_Quant);
		else if (strcmp(features[i], "f0_en_stddev") == 0) ret[i] = GetPitchEnergyStdDev(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_samples") == 0) ret[i] = GetPitchTierNumSamples(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_mean_curve") == 0) ret[i] = GetPitchTierMean(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "f0_stddev_curve") == 0) ret[i] = GetPitchTierStdDev(segment, p_Pitch_fmin, p_Pitch_fmax);
		else if (strcmp(features[i], "pp_samples") == 0) ret[i] = GetPointProcessNumSamples(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "pp_periods") == 0) ret[i] = GetPointProcessNumPeriods(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "pp_period_mean") == 0) ret[i] = GetPointProcessPeriodMean(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "pp_period_stddev") == 0) ret[i] = GetPointProcessPeriodStdDev(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "jitt_rap") == 0) ret[i] = GetJitterRAP(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "jitt_ppq5") == 0) ret[i] = GetJitterPPQ5(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "jitt_l") == 0) ret[i] = GetJitterLocal(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "jitt_la") == 0) ret[i] = GetJitterLocalAbs(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "jitt_ddp") == 0) ret[i] = GetJitterDDP(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor);
		else if (strcmp(features[i], "shim_apq3") == 0) ret[i] = GetShimmerAPQ3(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor, p_AT_maximum_amplitude_factor);
		else if (strcmp(features[i], "shim_apq5") == 0) ret[i] = GetShimmerAPQ5(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor, p_AT_maximum_amplitude_factor);
		else if (strcmp(features[i], "shim_apq11") == 0) ret[i] = GetShimmerAPQ11(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor, p_AT_maximum_amplitude_factor);
		else if (strcmp(features[i], "shim_l") == 0) ret[i] = GetShimmerLocal(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor, p_AT_maximum_amplitude_factor);
		else if (strcmp(features[i], "shim_ldb") == 0) ret[i] = GetShimmerLocalDb(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor, p_AT_maximum_amplitude_factor);
		else if (strcmp(features[i], "shim_dda") == 0) ret[i] = GetShimmerDDA(segment, p_Pitch_fmin, p_Pitch_fmax, p_PP_pmin, p_PP_pmax, p_PP_maximum_period_factor, p_AT_maximum_amplitude_factor);
		else if (strcmp(features[i], "harm_mean") == 0) ret[i] = GetHarmonicityMean(segment, p_Harm_ac, p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window);
		else if (strcmp(features[i], "harm_med") == 0) ret[i] = GetHarmonicityMedian(segment, p_Harm_ac, p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window);
		else if (strcmp(features[i], "harm_min") == 0) ret[i] = GetHarmonicityMin(segment, p_Harm_ac, p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window, p_Quant);
		else if (strcmp(features[i], "harm_max") == 0) ret[i] = GetHarmonicityMax(segment, p_Harm_ac, p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window, p_Quant);
		else if (strcmp(features[i], "harm_range") == 0) ret[i] = GetHarmonicityRange(segment, p_Harm_ac, p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window, p_Quant);
		else if (strcmp(features[i], "harm_stddev") == 0) ret[i] = GetHarmonicityStdDev(segment, p_Harm_ac, p_Harm_step, p_Harm_fmin, p_Harm_silence_threshold, p_Harm_periods_per_window);
		else if (strcmp(features[i], "form_disp_mean") == 0) ret[i] = GetFormantsDispMean(segment, p_Form_max, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency);
		else if (strcmp(features[i], "form_disp_med") == 0) ret[i] = GetFormantsDispMedian(segment, p_Form_max, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency);
		else if (strcmp(features[i], "form_disp_min") == 0) ret[i] = GetFormantsDispMin(segment, p_Form_max, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency, p_Quant);
		else if (strcmp(features[i], "form_disp_max") == 0) ret[i] = GetFormantsDispMax(segment, p_Form_max, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency, p_Quant);
		else if (strcmp(features[i], "form_disp_range") == 0) ret[i] = GetFormantsDispRange(segment, p_Form_max, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency, p_Quant);
		else if (strcmp(features[i], "en_mean") == 0) ret[i] = GetEnergyMean(segment, p_Intens_step, p_Intens_fmin, p_Intens_subtract_mean);
		else if (strcmp(features[i], "en_med") == 0) ret[i] = GetEnergyMedian(segment, p_Intens_step, p_Intens_fmin, p_Intens_subtract_mean);
		else if (strcmp(features[i], "en_min") == 0) ret[i] = GetEnergyMin(segment, p_Intens_step, p_Intens_fmin, p_Intens_subtract_mean, p_Quant);
		else if (strcmp(features[i], "en_max") == 0) ret[i] = GetEnergyMax(segment, p_Intens_step, p_Intens_fmin, p_Intens_subtract_mean, p_Quant);
		else if (strcmp(features[i], "en_range") == 0) ret[i] = GetEnergyRange(segment, p_Intens_step, p_Intens_fmin, p_Intens_subtract_mean, p_Quant);
		else if (strcmp(features[i], "en_stddev") == 0) ret[i] = GetEnergyStdDev(segment, p_Intens_step, p_Intens_fmin, p_Intens_subtract_mean);
		else if (strcmp(features[i], "ltas_mean") == 0) ret[i] = GetLtasEnergyMean(segment, p_Ltas_fmin, p_Ltas_fmax, p_Ltas_bandwidth);
		else if (strcmp(features[i], "ltas_min") == 0) ret[i] = GetLtasEnergyMin(segment, p_Ltas_fmin, p_Ltas_fmax, p_Ltas_bandwidth);
		else if (strcmp(features[i], "ltas_max") == 0) ret[i] = GetLtasEnergyMax(segment, p_Ltas_fmin, p_Ltas_fmax, p_Ltas_bandwidth);
		else if (strcmp(features[i], "ltas_range") == 0) ret[i] = GetLtasEnergyRange(segment, p_Ltas_fmin, p_Ltas_fmax, p_Ltas_bandwidth);
		else if (strcmp(features[i], "ltas_stddev") == 0) ret[i] = GetLtasEnergyStdDev(segment, p_Ltas_fmin, p_Ltas_fmax, p_Ltas_bandwidth);
		else if (strcmp(features[i], "ltas_slope") == 0) ret[i] = GetLtasEnergySlope(segment, p_Ltas_flow_min, p_Ltas_flow_max, p_Ltas_fhigh_min, p_Ltas_fhigh_max, p_Ltas_bandwidth);
		else if (strcmp(features[i], "ltas_lph") == 0) ret[i] = GetLtasEnergyLocalPeakHeight(segment, p_Ltas_environment_min, p_Ltas_environment_max, p_Ltas_peak_min, p_Ltas_peak_max, p_Ltas_bandwidth);
		else if (strlen(features[i]) > 3 && features[i][0] == 'f' && features[i][2] == '_') { // f<n>_...
			char* formant_str = new char[2]; formant_str[0] = features[i][1]; formant_str[1] = '\0';
			int formant = atoi(formant_str); delete[] formant_str;
			if (strcmp(features[i] + 3, "mean") == 0) ret[i] = GetFormantMean(segment, formant, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency);
			else if (strcmp(features[i] + 3, "med") == 0) ret[i] = GetFormantMedian(segment, formant, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency);
			else if (strcmp(features[i] + 3, "min") == 0) ret[i] = GetFormantMin(segment, formant, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency, p_Quant);
			else if (strcmp(features[i] + 3, "max") == 0) ret[i] = GetFormantMax(segment, formant, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency, p_Quant);
			else if (strcmp(features[i] + 3, "range") == 0) ret[i] = GetFormantRange(segment, formant, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency, p_Quant);
			else if (strcmp(features[i] + 3, "stddev") == 0) ret[i] = GetFormantStdDev(segment, formant, p_Form_step, p_Form_fmax, p_Form_window_length, p_Form_preemphasis_frequency);
			else {
				printf("Error - Unknown Feature: %s \n", features[i]);
				throw(1001);
			}
		}
		else {
			printf("Error - Unknown Feature: %s \n", features[i]);
			throw(1001);
		}
		if (times != NULL) times[i] = currenttime() - tstart; // benchmark
	}
	return ret;
}


void PraatSound::GetRange(int segment, double& min, double& max)
{
	if (segment == -1) {
		min = whole.start;
		max = whole.end; // everything
	} else if (segment >= 0) {
		if (m_segments[segment]->func == PraatSound::wfRectangular) {
			// No window function: use subrange of full sound
			min = m_segments[segment]->start;
			max = m_segments[segment]->end;
		} else {
			// Window function: use part, but whole segment
			min = m_segments[segment]->snd->xmin;
			max = m_segments[segment]->snd->xmax;
		}
	} else {
		printf("Error - Invalid segment number: %i \n", segment);
		throw(1003);
	}
//	printf("     range: %f .. %f \n", min, max);
}

PraatSound::SegmentDef* PraatSound::GetSegment(int segment, bool no_range)
{
	// This returns the segment where the *objects* for this segment
	// are stored. This is either the specified segment, or the whole
	// waveform (if segment=-1 or if no window function is defined
	// and we can use a subrange of the whole wave)
	if (segment == -1) {
//		printf("     sound: full waveform\n");
		return &whole;
	} else {
		SegmentDef* seg = m_segments[segment];
		if (seg->func == PraatSound::wfRectangular && !no_range) {
			// use range parameters instead of sound part
			// (This is only possible for features that support ranges. Use 'no_range=true' for the rest.)
//			printf("     sound: full waveform (but using subrange)\n");
			return &whole;
		} else if (seg->snd == NULL) {
			// Create part and apply window function
//			printf("Extracting part %i (%f .. %f) ...\n", segment, seg->start, seg->end);
			seg->snd = Sound_extractPart(whole.snd, seg->start, seg->end, (kSound_windowShape)seg->func, 1.0, 0);
			/*if(Melder_hasError()) {
				fprintf(stderr, "Melder_error: %s\n", Melder_getError());
				Melder_clearError();
			}*/
			if (seg->snd == NULL) {
				//printf("Error - Praat failed to extract segment %i !\n", segment);
				//fprintf(stderr, "seg->snd = %p, whole.snd = %p, seg->start = %f, seg->end = %f, seg->func = %d\n", seg->snd, whole.snd, seg->start, seg->end, seg->func);
				THROW(1002, "Error - Praat failed to extract segment!");
			}
		}
//		printf("     sound: segment %i \n", segment);
		return seg;
	}
}

void PraatSound::GetSampledBounds(Sampled obj, double tmin, double tmax, int& imin, int& imax)
{
	imin = Sampled_xToNearestIndex(obj, tmin);
	imax = Sampled_xToNearestIndex(obj, tmax);
	if (imin < 0) imin = 0;
	if (imax < 0) imax = 0;
	if (imin > obj->nx) imin = obj->nx;
	if (imax > obj->nx) imax = obj->nx;
	//printf("Min = %f, Max = %f, IMin = %i, IMax = %i \n", tmin, tmax, imin, imax);
}


Pitch PraatSound::UpdatePitch(int segment, double fmin, double fmax, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
//double dur = seg->snd->xmax - seg->snd->xmin;
//printf("Sound on which pitch will be computed has duration %f ...\n", dur);
	if (seg->pitch == NULL || seg->pitch_fmin != fmin || seg->pitch_fmax != fmax) {
#ifdef DEBUG
		printf("Computing PITCH for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		seg->pitch = Sound_to_Pitch(seg->snd, 0.0, fmin, fmax); // In "Sound_to_Pitch.c"
		if (seg->pitch == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute PITCH!\n");
#endif
			seg->pitch = (Pitch)-1;
		}
		seg->pitch_fmin = fmin;
		seg->pitch_fmax = fmax;
	}
	return seg->pitch;
}

PointProcess PraatSound::UpdatePointProcess(int segment, double fmin, double fmax, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	Pitch pitch = UpdatePitch(segment, fmin, fmax, no_range);
	if ((int)pitch == -1) return (PointProcess)-1;
	if (seg->pp == NULL || seg->pp_pitch != pitch || seg->pitch_fmin != fmin || seg->pitch_fmax != fmax) {
#ifdef DEBUG
		printf("Computing POINTPROCESS for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		seg->pp = Sound_Pitch_to_PointProcess_cc(seg->snd, pitch); // In "Pitch_to_PointProcess.c"
		if (seg->pp == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute POINTPROCESS!\n");
#endif
			seg->pp = (PointProcess)-1;
		}
		seg->pp_pitch = pitch;
	}
	return seg->pp;
}

PitchTier PraatSound::UpdatePitchTier(int segment, double fmin, double fmax, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	Pitch pitch = UpdatePitch(segment, fmin, fmax, no_range);
	if ((int)pitch == -1) return (PitchTier)-1;
	if (seg->pitcht == NULL || seg->pitcht_pitch != pitch || seg->pitch_fmin != fmin || seg->pitch_fmax != fmax) {
#ifdef DEBUG
		printf("Computing PITCH-TIER for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		seg->pitcht = Pitch_to_PitchTier(pitch); // In "Pitch_to_PitchTier.c"
		if (seg->pitcht == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute PITCH-TIER!\n");
#endif
			seg->pitcht = (PitchTier)-1;
		}
		seg->pitcht_pitch = pitch;
	}
	return seg->pitcht;
}

AmplitudeTier PraatSound::UpdateAmplitudeTier(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	// AmplitudeTier is a special case in such way that it can be computed on a range of the underlying object;
	// however, basically all functions working on AmplitudeTier have no range.
	// It would be possible to optimize its computation by considering different computation paths
	// (over segments or full waveforms), but due to complexity, it is not currently done.
	
	SegmentDef* pp_seg = GetSegment(segment, false);
	SegmentDef* peaks_seg = GetSegment(segment, true);
	PointProcess pp = UpdatePointProcess(pp_seg->id, fmin, fmax, false);
	if ((int)pp == -1) return (AmplitudeTier)-1;
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (peaks_seg->peaks == NULL || peaks_seg->peaks_pp != pp || pp_seg->pitch_fmin != fmin || pp_seg->pitch_fmax != fmax || peaks_seg->peaks_pmin != pmin || peaks_seg->peaks_pmax != pmax || peaks_seg->peaks_mpf != maximum_period_factor) {
#ifdef DEBUG
		printf("Computing AMPLITUDE for segment %i on PP-segment %i (req by %i) ...\n", peaks_seg->id, pp_seg->id, segment);
#endif
		peaks_seg->peaks = PointProcess_Sound_to_AmplitudeTier_period(pp, pp_seg->snd, tmin, tmax, pmin, pmax, maximum_period_factor); // in "AmplitudeTier.c"
		if (peaks_seg->peaks == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute AMPLITUDE!\n");
#endif
			peaks_seg->peaks = (AmplitudeTier)-1;
		}
		peaks_seg->peaks_pp = pp;
		peaks_seg->peaks_pmin = pmin;
		peaks_seg->peaks_pmax = pmax;
		peaks_seg->peaks_mpf = maximum_period_factor;
	}
	return peaks_seg->peaks;
}

Harmonicity PraatSound::UpdateHarmonicity(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	if (seg->harm == NULL) {
#ifdef DEBUG
		printf("Computing HARMONICITY for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		if (ac) {
			seg->harm = Sound_to_Harmonicity_ac(seg->snd, step, fmin, silence_threshold, periods_per_window); // In "Sound_to_Harmonicity.c"
		} else {
			seg->harm = Sound_to_Harmonicity_cc(seg->snd, step, fmin, silence_threshold, periods_per_window); // In "Sound_to_Harmonicity.c"
		}
		if (seg->harm == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute HARMONICITY!\n");
#endif
			seg->harm = (Harmonicity)-1;
		}
		seg->harm_ac = ac;
		seg->harm_step = step;
		seg->harm_fmin = fmin;
		seg->harm_silence_threshold = silence_threshold;
		seg->harm_periods_per_window = periods_per_window;
	}
	return seg->harm;
}

Formant PraatSound::UpdateFormant(int segment, double step, double fmax, double window_length, double preemphasis_frequency, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	if (seg->form == NULL || seg->form_maxform != p_Form_max || seg->form_fmax != fmax || seg->form_window_length != window_length || seg->form_preemphasis_frequency != preemphasis_frequency) {
#ifdef DEBUG
		printf("Computing FORMANTS for segment %i up to %i (req by %i) ...\n", seg->id, p_Form_max, segment);
#endif
		seg->form = Sound_to_Formant_burg(seg->snd, step, p_Form_max, fmax, window_length, preemphasis_frequency);
		if (seg->form == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute FORMANTS!\n");
#endif
			seg->form = (Formant)-1;
		}
		seg->form_maxform = p_Form_max;
		seg->form_step = step;
		seg->form_fmax = fmax;
		seg->form_window_length = window_length;
		seg->form_preemphasis_frequency = preemphasis_frequency;
	}
	return seg->form;
}

Intensity PraatSound::UpdateIntensity(int segment, double step, double fmin, bool subtract_mean, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	if (seg->intens == NULL || seg->intens_step != step || seg->intens_fmin != fmin || seg->intens_subtract_mean != subtract_mean) {
#ifdef DEBUG
		printf("Computing INTENSITY for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		seg->intens = Sound_to_Intensity(seg->snd, fmin, step, subtract_mean); // In "Sound_to_Intensity.c"
		if (seg->intens == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute INTENSITY!\n");
#endif
			seg->intens = (Intensity)-1;
		}
		seg->intens_step = step;
		seg->intens_fmin = fmin;
		seg->intens_subtract_mean = subtract_mean;
	}
	return seg->intens;
}

Spectrum PraatSound::UpdateSpectrum(int segment, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	if (seg->spec == NULL) {
#ifdef DEBUG
		printf("Computing SPECTRUM for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		seg->spec = Sound_to_Spectrum(seg->snd, 1); // In "Sound_and_Spectrum.c"
		if (seg->spec == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute SPECTRUM!\n");
#endif
			seg->spec = (Spectrum)-1;
		}
	}
	return seg->spec;
}

Ltas PraatSound::UpdateLtas(int segment, double bandwidth, bool no_range)
{
	SegmentDef* seg = GetSegment(segment, no_range);
	Spectrum spec = UpdateSpectrum(segment, no_range);
	if ((int)spec == -1) return (Ltas)-1;
	if (seg->ltas == NULL || seg->ltas_spec != spec || seg->ltas_bandwidth != bandwidth) {
#ifdef DEBUG
		printf("Computing LTAS for segment %i (req by %i) ...\n", seg->id, segment);
#endif
		seg->ltas = Spectrum_to_Ltas(spec, bandwidth); // In "Ltas.c"
		if (seg->ltas == NULL) {
#ifdef DEBUG
			printf("Warning - Praat failed to compute LTAS!\n");
#endif
			seg->ltas = (Ltas)-1;
		}
		seg->ltas_spec = spec;
		seg->ltas_bandwidth = bandwidth;
		// This code appears in Praat's "Sound_to_Ltas" function
		double correction = -10 * log10 (spec->dx * seg->snd->nx * seg->snd->dx);
		for (int iband = 1; iband <= seg->ltas->nx; iband ++) {
			seg->ltas->z[1][iband] += correction;
		}
	}
	return seg->ltas;
}





double PraatSound::GetPitchMean(int segment, double fmin, double fmax)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Pitch_getMean(pitch, tmin, tmax, kPitch_unit_HERTZ); // In "Pitch.c"
}
double PraatSound::GetPitchMedian(int segment, double fmin, double fmax)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Pitch_getQuantile(pitch, tmin, tmax, 0.5, kPitch_unit_HERTZ); // In "Pitch.c"
}
double PraatSound::GetPitchMin(int segment, double fmin, double fmax, bool quant)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (quant) {
		return Pitch_getQuantile(pitch, tmin, tmax, p_QuantMin, kPitch_unit_HERTZ); // In "Pitch.c"
	} else {
		return Pitch_getMinimum(pitch, tmin, tmax, kPitch_unit_HERTZ, Pitch_LINEAR); // In "Pitch.c"
	}
}
double PraatSound::GetPitchMax(int segment, double fmin, double fmax, bool quant)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (quant) {
		return Pitch_getQuantile(pitch, tmin, tmax, p_QuantMax, kPitch_unit_HERTZ); // In "Pitch.c"
	} else {
		return Pitch_getMaximum(pitch, tmin, tmax, kPitch_unit_HERTZ, Pitch_LINEAR); // In "Pitch.c"
	}
}
double PraatSound::GetPitchRange(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchMax(segment, fmin, fmax, quant) - GetPitchMin(segment, fmin, fmax, quant);
}
double PraatSound::GetPitchStdDev(int segment, double fmin, double fmax)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Pitch_getStandardDeviation(pitch, tmin, tmax, kPitch_unit_HERTZ); // In "Pitch.c"
}
double PraatSound::GetPitchMAS(int segment, double fmin, double fmax, bool no_octave)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax, true);
	if ((int)pitch == -1) return numeric_limits<double>::infinity();
	long nVoiced;
	double ret;
	if (no_octave) {
		nVoiced = Pitch_getMeanAbsSlope_noOctave(pitch, &ret); // In "Pitch.c"
	} else {
		nVoiced = Pitch_getMeanAbsSlope_semitones(pitch, &ret); // In "Pitch.c"
	}
	return ret;
}

double* PraatSound::PitchCandidatesToArray(Pitch pitch, double tmin, double tmax, int& length)
{
	int imin, imax;
	GetSampledBounds((Sampled)pitch, tmin, tmax, imin, imax);
	length = imax - imin + 1; if (length <= 0) return NULL;
	double* arr = new double[length]; int j = 0;
	for (int i=imin; i<=imax; i++) {
		arr[j] = double(pitch->frame[i].nCandidates); j++;
	}
	return arr;
}
double* PraatSound::PitchIntensityToArray(Pitch pitch, double tmin, double tmax, int& length)
{
	int imin, imax;
	GetSampledBounds((Sampled)pitch, tmin, tmax, imin, imax);
	length = imax - imin + 1; if (length <= 0) return NULL;
	double* arr = new double[length]; int j = 0;
	for (int i=imin; i<=imax; i++) {
		arr[j] = double(pitch->frame[i].intensity); j++;
	}
	return arr;
}
double* PraatSound::PitchStrengthMeanToArray(Pitch pitch, double tmin, double tmax, int& length)
{
	int imin, imax;
	GetSampledBounds((Sampled)pitch, tmin, tmax, imin, imax);
	length = imax - imin + 1; if (length <= 0) return NULL;
	double* arr = new double[length]; int j = 0;
	for (int i=imin; i<=imax; i++) {
		double sum = 0;
		for (int n=2; n<=pitch->frame[i].nCandidates; n++) {
			// 1 seems to be always 0
			//printf(" p-strength for frame %i candidate %i = %e \n", i, n, pitch->frame[i].candidate[n].strength);
			sum += pitch->frame[i].candidate[n].strength;
		}
		if (pitch->frame[i].nCandidates > 0) {
			arr[j] = sum / double(pitch->frame[i].nCandidates);
			j++;
		} else {
			length--;
		}
	}
	return arr;
}
double PraatSound::GetPitchFrameFeature(int segment, double fmin, double fmax, int feat, int measure, bool alt)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return 0;
	double tmin, tmax; GetRange(segment, tmin, tmax);
	int length; double* arr; double ret;
	switch (feat) {
		case 1:
			arr = PitchCandidatesToArray(pitch, tmin, tmax, length); break;
		case 2:
			arr = PitchIntensityToArray(pitch, tmin, tmax, length); break;
		case 3:
			arr = PitchStrengthMeanToArray(pitch, tmin, tmax, length); break;
	}
	if (length <= 0) return 0;
	switch (measure) {
		case 1: // mean
			ret = ArrGetMean(arr, length); break;
		case 2: // median
			ret = ArrGetQuantile(arr, length, 0.5); break;
		case 3: // min
			if (!alt) ret = ArrGetMin(arr, length);
			else ret = ArrGetQuantile(arr, length, p_QuantMin);
			break;
		case 4: // max
			if (!alt) ret = ArrGetMax(arr, length);
			else ret = ArrGetQuantile(arr, length, p_QuantMax);
			break;
		case 5: // range
			if (!alt) ret = ArrGetRange(arr, length);
			else ret = ArrGetQuantile(arr, length, p_QuantMax) - ArrGetQuantile(arr, length, p_QuantMin);
			break;
		case 6: // stddev
			ret = ArrGetStdDev(arr, length); break;
	}
	delete[] arr;
	return ret;
}

double PraatSound::GetPitchCandidatesMean(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 1, 1, false);
}
double PraatSound::GetPitchCandidatesMedian(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 1, 2, false);
}
double PraatSound::GetPitchCandidatesMin(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 1, 3, quant);
}
double PraatSound::GetPitchCandidatesMax(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 1, 4, quant);
}
double PraatSound::GetPitchCandidatesRange(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 1, 5, quant);
}
double PraatSound::GetPitchCandidatesStdDev(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 1, 6, false);
}

double PraatSound::GetPitchStrengthMean(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 3, 1, false);
}
double PraatSound::GetPitchStrengthMedian(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 3, 2, false);
}
double PraatSound::GetPitchStrengthMin(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 3, 3, quant);
}
double PraatSound::GetPitchStrengthMax(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 3, 4, quant);
}
double PraatSound::GetPitchStrengthRange(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 3, 5, quant);
}
double PraatSound::GetPitchStrengthStdDev(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 3, 6, false);
}

double PraatSound::GetPitchEnergyMean(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 2, 1, false);
}
double PraatSound::GetPitchEnergyMedian(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 2, 2, false);
}
double PraatSound::GetPitchEnergyMin(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 2, 3, quant);
}
double PraatSound::GetPitchEnergyMax(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 2, 4, quant);
}
double PraatSound::GetPitchEnergyRange(int segment, double fmin, double fmax, bool quant)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 2, 5, quant);
}
double PraatSound::GetPitchEnergyStdDev(int segment, double fmin, double fmax)
{
	return GetPitchFrameFeature(segment, fmin, fmax, 2, 6, false);
}


double PraatSound::GetPitchVoiced(int segment, double fmin, double fmax)
{
	Pitch pitch = UpdatePitch(segment, fmin, fmax);
	if ((int)pitch == -1) return 0;
	double tmin, tmax; GetRange(segment, tmin, tmax);
	int imin, imax;
	GetSampledBounds((Sampled)pitch, tmin, tmax, imin, imax);
	int voiced = 0; int unvoiced = 0;
	for (int i=imin; i<imax; i++) {
		if (Pitch_isVoiced_i(pitch, i)) voiced++; else unvoiced++;
	}
	if (unvoiced == 0) return 1.0;
	return voiced / unvoiced;
}

double PraatSound::GetPitchTierNumSamples(int segment, double fmin, double fmax)
{
	PitchTier pitcht = UpdatePitchTier(segment, fmin, fmax, true);
	if ((int)pitcht == -1) return numeric_limits<double>::infinity();
	return double(pitcht->points->size);
}
double PraatSound::GetPitchTierMean(int segment, double fmin, double fmax)
{
	PitchTier pitcht = UpdatePitchTier(segment, fmin, fmax);
	if ((int)pitcht == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return RealTier_getMean_curve(pitcht, tmin, tmax); // in "RealTier.c"
}
/*
double PraatSound::GetPitchTierMeanPoints(int segment, double fmin, double fmax)
{
	PitchTier pitcht = UpdatePitchTier(segment, fmin, fmax);
	if ((int)pitcht == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return RealTier_getMean_points(pitcht, tmin, tmax); // in "RealTier.c"
}
*/
double PraatSound::GetPitchTierStdDev(int segment, double fmin, double fmax)
{
	PitchTier pitcht = UpdatePitchTier(segment, fmin, fmax);
	if ((int)pitcht == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return RealTier_getStandardDeviation_curve(pitcht, tmin, tmax); // in "RealTier.c"
}
/*
double PraatSound::GetPitchTierStdDevPoints(int segment, double fmin, double fmax)
{
	PitchTier pitcht = UpdatePitchTier(segment, fmin, fmax);
	if ((int)pitcht == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return RealTier_getStandardDeviation_points(pitcht, tmin, tmax); // in "RealTier.c"
}
*/

double PraatSound::GetPointProcessNumSamples(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax, true);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	return double(pp->nt);
}
double PraatSound::GetPointProcessNumPeriods(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getNumberOfPeriods(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "PointProcess.c"
}
double PraatSound::GetPointProcessPeriodMean(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getMeanPeriod(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "PointProcess.c"
}
double PraatSound::GetPointProcessPeriodStdDev(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getStdevPeriod(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "PointProcess.c"
}

double PraatSound::GetJitterRAP(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getJitter_rap(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "VoiceAnalysis.c"
}
double PraatSound::GetJitterPPQ5(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getJitter_ppq5(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "VoiceAnalysis.c"
}
double PraatSound::GetJitterLocal(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getJitter_local(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "VoiceAnalysis.c"
}
double PraatSound::GetJitterLocalAbs(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getJitter_local_absolute(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "VoiceAnalysis.c"
}
double PraatSound::GetJitterDDP(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor)
{
	PointProcess pp = UpdatePointProcess(segment, fmin, fmax);
	if ((int)pp == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return PointProcess_getJitter_ddp(pp, tmin, tmax, pmin, pmax, maximum_period_factor); // in "VoiceAnalysis.c"
}

double PraatSound::GetShimmerAPQ3(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor, double maximum_amplitude_factor)
{
	AmplitudeTier peaks = UpdateAmplitudeTier(segment, fmin, fmax, pmin, pmax, maximum_period_factor);
	if ((int)peaks == -1) return numeric_limits<double>::infinity();
	return AmplitudeTier_getShimmer_apq3(peaks, pmin, pmax, maximum_amplitude_factor); // in "AmplitudeTier.c"
}
double PraatSound::GetShimmerAPQ5(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor, double maximum_amplitude_factor)
{
	AmplitudeTier peaks = UpdateAmplitudeTier(segment, fmin, fmax, pmin, pmax, maximum_period_factor);
	if ((int)peaks == -1) return numeric_limits<double>::infinity();
	return AmplitudeTier_getShimmer_apq5(peaks, pmin, pmax, maximum_amplitude_factor); // in "AmplitudeTier.c"
}
double PraatSound::GetShimmerAPQ11(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor, double maximum_amplitude_factor)
{
	AmplitudeTier peaks = UpdateAmplitudeTier(segment, fmin, fmax, pmin, pmax, maximum_period_factor);
	if ((int)peaks == -1) return numeric_limits<double>::infinity();
	return AmplitudeTier_getShimmer_apq11(peaks, pmin, pmax, maximum_amplitude_factor); // in "AmplitudeTier.c"
}
double PraatSound::GetShimmerLocal(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor, double maximum_amplitude_factor)
{
	AmplitudeTier peaks = UpdateAmplitudeTier(segment, fmin, fmax, pmin, pmax, maximum_period_factor);
	if ((int)peaks == -1) return numeric_limits<double>::infinity();
	return AmplitudeTier_getShimmer_local(peaks, pmin, pmax, maximum_amplitude_factor); // in "AmplitudeTier.c"
}
double PraatSound::GetShimmerLocalDb(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor, double maximum_amplitude_factor)
{
	AmplitudeTier peaks = UpdateAmplitudeTier(segment, fmin, fmax, pmin, pmax, maximum_period_factor);
	if ((int)peaks == -1) return numeric_limits<double>::infinity();
	return AmplitudeTier_getShimmer_local_dB(peaks, pmin, pmax, maximum_amplitude_factor); // in "AmplitudeTier.c"
}
double PraatSound::GetShimmerDDA(int segment, double fmin, double fmax, double pmin, double pmax, double maximum_period_factor, double maximum_amplitude_factor)
{
	AmplitudeTier peaks = UpdateAmplitudeTier(segment, fmin, fmax, pmin, pmax, maximum_period_factor);
	if ((int)peaks == -1) return numeric_limits<double>::infinity();
	return AmplitudeTier_getShimmer_dda(peaks, pmin, pmax, maximum_amplitude_factor); // in "AmplitudeTier.c"
}

double PraatSound::GetHarmonicityMean(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window)
{
	Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window);
	if ((int)harm == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Harmonicity_getMean(harm, tmin, tmax); // In "Harmonicity.c"
}
double PraatSound::GetHarmonicityMedian(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window)
{
	Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window, true);
	if ((int)harm == -1) return numeric_limits<double>::infinity();
	return Harmonicity_getQuantile(harm, 0.5); // In "Harmonicity.c"
}
double PraatSound::GetHarmonicityMin(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window, bool quant)
{
	if (quant) {
		Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window, true);
		if ((int)harm == -1) return numeric_limits<double>::infinity();
		return Harmonicity_getQuantile(harm, p_QuantMin); // In "Harmonicity.c"
	} else {
		Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window);
		if ((int)harm == -1) return numeric_limits<double>::infinity();
		double tmin, tmax; GetRange(segment, tmin, tmax);
		return Vector_getMinimum(harm, tmin, tmax, NUM_PEAK_INTERPOLATE_PARABOLIC); // In "Vector.c"
	}
}
double PraatSound::GetHarmonicityMax(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window, bool quant)
{
	if (quant) {
		Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window, true);
		if ((int)harm == -1) return numeric_limits<double>::infinity();
		return Harmonicity_getQuantile(harm, p_QuantMax); // In "Harmonicity.c"
	} else {
		Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window);
		if ((int)harm == -1) return numeric_limits<double>::infinity();
		double tmin, tmax; GetRange(segment, tmin, tmax);
		return Vector_getMaximum(harm, tmin, tmax, NUM_PEAK_INTERPOLATE_PARABOLIC); // In "Vector.c"
	}
}
double PraatSound::GetHarmonicityRange(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window, bool quant)
{
	return GetHarmonicityMax(segment, ac, step, fmin, silence_threshold, periods_per_window, quant) - GetHarmonicityMin(segment, ac, step, fmin, silence_threshold, periods_per_window, quant);
}
double PraatSound::GetHarmonicityStdDev(int segment, bool ac, double step, double fmin, double silence_threshold, double periods_per_window)
{
	Harmonicity harm = UpdateHarmonicity(segment, ac, step, fmin, silence_threshold, periods_per_window);
	if ((int)harm == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Harmonicity_getStandardDeviation(harm, tmin, tmax); // In "Harmonicity.c"
}

double PraatSound::GetFormantMean(int segment, int formant, double step, double fmax, double window_length, double preemphasis_frequency)
{
	Formant form = UpdateFormant(segment, step, fmax, window_length, preemphasis_frequency);
	if ((int)form == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Formant_getMean(form, formant, tmin, tmax, 0); // In "Formant.c"
}
double PraatSound::GetFormantMedian(int segment, int formant, double step, double fmax, double window_length, double preemphasis_frequency)
{
	Formant form = UpdateFormant(segment, step, fmax, window_length, preemphasis_frequency);
	if ((int)form == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Formant_getQuantile(form, formant, 0.5, tmin, tmax, 0); // In "Formant.c"
}
double PraatSound::GetFormantMin(int segment, int formant, double step, double fmax, double window_length, double preemphasis_frequency, bool quant)
{
	Formant form = UpdateFormant(segment, step, fmax, window_length, preemphasis_frequency);
	if ((int)form == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (quant) {
		return Formant_getQuantile(form, formant, p_QuantMin, tmin, tmax, 0); // In "Formant.c"
	} else {
		return Formant_getMinimum(form, formant, tmin, tmax, 0, 0); // In "Formant.c"
	}
}
double PraatSound::GetFormantMax(int segment, int formant, double step, double fmax, double window_length, double preemphasis_frequency, bool quant)
{
	Formant form = UpdateFormant(segment, step, fmax, window_length, preemphasis_frequency);
	if ((int)form == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (quant) {
		return Formant_getQuantile(form, formant, p_QuantMax, tmin, tmax, 0); // In "Formant.c"
	} else {
		return Formant_getMaximum(form, formant, tmin, tmax, 0, 0); // In "Formant.c"
	}
}
double PraatSound::GetFormantRange(int segment, int formant, double step, double fmax, double window_length, double preemphasis_frequency, bool quant)
{
	return GetFormantMax(segment, formant, step, fmax, window_length, preemphasis_frequency, quant) - GetFormantMin(segment, formant, step, fmax, window_length, preemphasis_frequency, quant);
}
double PraatSound::GetFormantStdDev(int segment, int formant, double step, double fmax, double window_length, double preemphasis_frequency)
{
	Formant form = UpdateFormant(segment, step, fmax, window_length, preemphasis_frequency);
	if ((int)form == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Formant_getStandardDeviation(form, formant, tmin, tmax, 0); // In "Formant.c"
}

double PraatSound::GetFormantsDispMean(int segment, int formants, double step, double fmax, double window_length, double preemphasis_frequency)
{
	double sum = 0;
	for (int f=formants; f>1; f--) {
		sum += GetFormantMean(segment, f, step, fmax, window_length, preemphasis_frequency) - GetFormantMean(segment, f-1, step, fmax, window_length, preemphasis_frequency);
	}
	return sum / double(formants);
}
double PraatSound::GetFormantsDispMedian(int segment, int formants, double step, double fmax, double window_length, double preemphasis_frequency)
{
	double sum = 0;
	for (int f=formants; f>1; f--) {
		sum += GetFormantMedian(segment, f, step, fmax, window_length, preemphasis_frequency) - GetFormantMedian(segment, f-1, step, fmax, window_length, preemphasis_frequency);
	}
	return sum / double(formants);
}
double PraatSound::GetFormantsDispMin(int segment, int formants, double step, double fmax, double window_length, double preemphasis_frequency, bool quant)
{
	double sum = 0;
	for (int f=formants; f>1; f--) {
		sum += GetFormantMin(segment, f, step, fmax, window_length, preemphasis_frequency, quant) - GetFormantMin(segment, f-1, step, fmax, window_length, preemphasis_frequency, quant);
	}
	return sum / double(formants);
}
double PraatSound::GetFormantsDispMax(int segment, int formants, double step, double fmax, double window_length, double preemphasis_frequency, bool quant)
{
	double sum = 0;
	for (int f=formants; f>1; f--) {
		sum += GetFormantMax(segment, f, step, fmax, window_length, preemphasis_frequency, quant) - GetFormantMax(segment, f-1, step, fmax, window_length, preemphasis_frequency, quant);
	}
	return sum / double(formants);
}
double PraatSound::GetFormantsDispRange(int segment, int formants, double step, double fmax, double window_length, double preemphasis_frequency, bool quant)
{
	return GetFormantsDispMax(segment, formants, step, fmax, window_length, preemphasis_frequency, quant) - GetFormantsDispMin(segment, formants, step, fmax, window_length, preemphasis_frequency, quant);
}

double PraatSound::GetEnergyMean(int segment, double step, double fmin, bool subtract_mean)
{
	Intensity intens = UpdateIntensity(segment, step, fmin, subtract_mean);
	if ((int)intens == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Vector_getMean(intens, tmin, tmax, 1); // In "Vector.c"
}
double PraatSound::GetEnergyMedian(int segment, double step, double fmin, bool subtract_mean)
{
	Intensity intens = UpdateIntensity(segment, step, fmin, subtract_mean);
	if ((int)intens == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Intensity_getQuantile(intens, tmin, tmax, 0.5); // In "Intensity.c"
}
double PraatSound::GetEnergyMin(int segment, double step, double fmin, bool subtract_mean, bool quant)
{
	Intensity intens = UpdateIntensity(segment, step, fmin, subtract_mean);
	if ((int)intens == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (quant) {
		return Intensity_getQuantile(intens, tmin, tmax, p_QuantMin); // In "Intensity.c"
	} else {
		return Vector_getMinimum(intens, tmin, tmax, NUM_PEAK_INTERPOLATE_PARABOLIC); // In "Vector.c"
	}
}
double PraatSound::GetEnergyMax(int segment, double step, double fmin, bool subtract_mean, bool quant)
{
	Intensity intens = UpdateIntensity(segment, step, fmin, subtract_mean);
	if ((int)intens == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	if (quant) {
		return Intensity_getQuantile(intens, tmin, tmax, p_QuantMax); // In "Intensity.c"
	} else {
		return Vector_getMaximum(intens, tmin, tmax, NUM_PEAK_INTERPOLATE_PARABOLIC); // In "Vector.c"
	}
}
double PraatSound::GetEnergyRange(int segment, double step, double fmin, bool subtract_mean, bool quant)
{
	return GetEnergyMax(segment, step, fmin, subtract_mean, quant) - GetEnergyMin(segment, step, fmin, subtract_mean, quant);
}
double PraatSound::GetEnergyStdDev(int segment, double step, double fmin, bool subtract_mean)
{
	Intensity intens = UpdateIntensity(segment, step, fmin, subtract_mean);
	if ((int)intens == -1) return numeric_limits<double>::infinity();
	double tmin, tmax; GetRange(segment, tmin, tmax);
	return Vector_getStandardDeviation(intens, tmin, tmax, 1); // In "Vector.c"
}

double PraatSound::GetLtasEnergyMean(int segment, double fmin, double fmax, double bandwidth)
{
	Ltas ltas = UpdateLtas(segment, bandwidth, true);
	if ((int)ltas == -1) return numeric_limits<double>::infinity();
	return Sampled_getMean_standardUnit(ltas, fmin, fmax, 0, Intensity_units_ENERGY, 0); // In "Sampled.c"
}
double PraatSound::GetLtasEnergyMin(int segment, double fmin, double fmax, double bandwidth)
{
	Ltas ltas = UpdateLtas(segment, bandwidth, true);
	if ((int)ltas == -1) return numeric_limits<double>::infinity();
	return Vector_getMinimum(ltas, fmin, fmax, NUM_PEAK_INTERPOLATE_PARABOLIC); // In "Vector.c"
}
double PraatSound::GetLtasEnergyMax(int segment, double fmin, double fmax, double bandwidth)
{
	Ltas ltas = UpdateLtas(segment, bandwidth, true);
	if ((int)ltas == -1) return numeric_limits<double>::infinity();
	return Vector_getMaximum(ltas, fmin, fmax, NUM_PEAK_INTERPOLATE_PARABOLIC); // In "Vector.c"
}
double PraatSound::GetLtasEnergyRange(int segment, double fmin, double fmax, double bandwidth)
{
	return GetLtasEnergyMax(segment, fmin, fmax, bandwidth) - GetLtasEnergyMin(segment, fmin, fmax, bandwidth);
}
double PraatSound::GetLtasEnergyStdDev(int segment, double fmin, double fmax, double bandwidth)
{
	Ltas ltas = UpdateLtas(segment, bandwidth, true);
	if ((int)ltas == -1) return numeric_limits<double>::infinity();
	return Sampled_getStandardDeviation_standardUnit(ltas, fmin, fmax, 0, Intensity_units_ENERGY, 0); // In "Sampled.c"
}
double PraatSound::GetLtasEnergySlope(int segment, double flow_min, double flow_max, double fhigh_min, double fhigh_max, double bandwidth)
{
	Ltas ltas = UpdateLtas(segment, bandwidth, true);
	if ((int)ltas == -1) return numeric_limits<double>::infinity();
	return Ltas_getSlope(ltas, flow_min, flow_max, fhigh_min, fhigh_max, Intensity_units_ENERGY); // In "Ltas.c"
}
double PraatSound::GetLtasEnergyLocalPeakHeight(int segment, double environment_min, double environment_max, double peak_min, double peak_max, double bandwidth)
{
	Ltas ltas = UpdateLtas(segment, bandwidth, true);
	if ((int)ltas == -1) return numeric_limits<double>::infinity();
	return Ltas_getLocalPeakHeight(ltas, environment_min, environment_max, peak_min, peak_max, Intensity_units_ENERGY); // In "Ltas.c"
}







void PraatSound::SegmentsClear(int capacity)
{
	if (m_segments != NULL) {
		for (int i=0; i<m_num_segments; i++) {
			delete m_segments[i];
		}
		delete[] m_segments;
	}
	if (capacity > 0) {
		m_segments = new SegmentDef*[capacity];
	} else {
		m_segments = NULL;
	}
	m_num_segments = 0;
}

void PraatSound::SegmentsAdd(double start, double end, WindowFunctions func)
{
	if (m_segments == NULL) THROW(1004, "Error - You need to call SegmentsClear() first");
	if (start < whole.start || end > whole.end) THROW(1006, "Error - Segment boundaries out of sound range");
	SegmentDef* seg = new SegmentDef(m_num_segments);
	seg->start = start;
	seg->end = end;
	seg->func = func;
	seg->snd = NULL;
	m_segments[m_num_segments] = seg;
	m_num_segments++;
}

void PraatSound::SegmentsReadRTTM(char* filename, WindowFunctions func)
{
	FILE* pFile;
	pFile = fopen(filename, "r");
	if (pFile == NULL) THROW(5, "Error - Failed to open RTTM file");
	char* buf = new char[1024];
	// Count lines
	int num_lines = 0;
	while (1) {
		if (fgets(buf, 1024, pFile) == NULL) {
			break; // probably EOF
		} else {
			num_lines++;
		}
	}
#ifdef DEBUG
	printf("Reading %i segments...\n", num_lines);
#endif
	fseek(pFile, 0, SEEK_SET);
	// Parse lines
	SegmentsClear(num_lines);
	for (int i=0; i<num_lines; i++) {
		memset(buf, 0, 1024); //needed?
		fgets(buf, 1024, pFile);
		char* pch;
		pch = strtok(buf, " ");
		int col = 0;
		double start = -1.0;
		double duration = -1.0;
		while (pch != NULL) {
			//printf("Match at %i - %i: %s\n", i, col, pch);
			if (col == 3) {
				// start pos.
				start = strtod(pch, NULL);
			} else if (col == 4) {
				// duration
				duration = strtod(pch, NULL);
			}
			pch = strtok(NULL, " ");
			col++;
		}
		if (start < 0 || duration < 0) {
			printf("Error - Failed to parse RTTM file on line %i (0-based)\n", i);
			throw(1005);
		}
		SegmentsAdd(start, start+duration, func);
	}
	delete[] buf;
	fclose(pFile);
}


int PraatSound::GetNumSegments()
{
	if (m_segments == NULL) return 0;
	return m_num_segments;
}

double PraatSound::GetDuration()
{
	return whole.snd->xmax;
}

double PraatSound::ArrGetMean(double* arr, int length)
{
	if (length == 0) return numeric_limits<double>::infinity();
	double sum = 0;
	for (int i=0; i<length; i++) sum += arr[i];
	return sum / double(length);
}
int PraatSound_compare_dbl(const void* a, const void* b)
{
  return (int)( *(double*)a - *(double*)b );
}
double PraatSound::ArrGetQuantile(double* arr, int length, double pos)
{
	if (length == 0) return numeric_limits<double>::infinity();
	double* arr2 = new double[length];
	memcpy(arr2, arr, length * sizeof(double));
	qsort(arr2, length, sizeof(double), PraatSound_compare_dbl);
	int i = int(pos * length) - 1;
	if (i < 0) i = 0;
	if (i > length-1) i = length-1;
	double ret = arr2[i];
	delete[] arr2;
	return ret;
}
double PraatSound::ArrGetMin(double* arr, int length)
{
	if (length == 0) return numeric_limits<double>::infinity();
	double min = arr[0];
	for (int i=1; i<length; i++) {
		if (arr[i] < min) min = arr[i];
	}
	return min;
}
double PraatSound::ArrGetMax(double* arr, int length)
{
	if (length == 0) return numeric_limits<double>::infinity();
	double max = arr[0];
	for (int i=1; i<length; i++) {
		if (arr[i] > max) max = arr[i];
	}
	return max;
}
double PraatSound::ArrGetRange(double* arr, int length)
{
	if (length == 0) return numeric_limits<double>::infinity();
	double max = arr[0]; double min = arr[0];
	for (int i=1; i<length; i++) {
		if (arr[i] > max) max = arr[i];
		if (arr[i] < min) min = arr[i];
	}
	return max - min;
}
double PraatSound::ArrGetStdDev(double* arr, int length)
{
	if (length == 0) return numeric_limits<double>::infinity();
	double mean = ArrGetMean(arr, length);
	double sum  = 0;
	for (int i=1; i<length; i++) sum += pow(arr[i] - mean, 2.0);
	return sqrt(sum / double(length-1));
}

double PraatSound::currenttime()
{
	timeval curtime;
	gettimeofday(&curtime,NULL); 
	return curtime.tv_sec+(curtime.tv_usec/1000000.0);
}

