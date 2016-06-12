#include "scriptmain.h"


using namespace std;

int main(int argc, char* argv[])
{

	printf("PraatLib Prosodic Features Library Sample\n");
	
	if (argc < 2 || argc > 3) {
		printf("   Usage: praatsample <file> [<rttmfile>] \n");
		printf("\n");
		printf("   <file>: Name of audio file to process. Supported formats are SPH, WAV,\n");
		printf("           and a few others (see Praat help)\n");
		printf("   <rttmfile>: Name of RTTM file defining segments to use.\n");
		printf("               If omitted, the whole wave is used as a single segment.\n");
		printf("\n");
		return 1;
	}
	
	printf("Using file: %s \n", argv[1]);
	
 	PraatSound sound(argv[1]);
	double d;
	//d = sound.GetPitchMean(); printf("Pitch Mean: %e \n", d);

	const int FEAT_MAX = 128;
	char** feats = new char*[FEAT_MAX];
	for (int i=0; i<FEAT_MAX; i++) feats[i] = new char[32];
	int c = 0;
	sprintf(feats[c], "jitt_ppq5"); c++;
	sprintf(feats[c], "pp_samples"); c++;
	sprintf(feats[c], "f0_mean"); c++;
	sprintf(feats[c], "f0_med"); c++;
	sprintf(feats[c], "f0_stddev"); c++;
	sprintf(feats[c], "shim_apq5"); c++;
	sprintf(feats[c], "harm_max"); c++;
	sprintf(feats[c], "form_disp_med"); c++;
	sprintf(feats[c], "f1_mean"); c++;
	sprintf(feats[c], "en_med"); c++;
	sprintf(feats[c], "ltas_min"); c++;
	sprintf(feats[c], "ltas_slope"); c++;
	sprintf(feats[c], "ltas_lph"); c++;
	//sprintf(feats[c], "f0_mas"); c++;

	
	if (argc >= 3) {
	  printf("Reading segments from: %s \n", argv[2]);
		sound.SegmentsReadRTTM(argv[2], PraatSound::wfRectangular);
	}
	
	int num = sound.GetNumSegments();
	if (num == 0) num = 1;
	double* benchmark = new double[num * c];
	double* res = sound.GetMultiple(feats, c, -2, benchmark);
	for (int j=0; j<num; j++) {
		for (int i=0; i<c; i++) {
			int t = j*c+i;
			printf(" #%i - Seg %i Feat %i (%s) = %e  \t[time=%fs]\n", t, j+1, i+1, feats[i], res[t], benchmark[t]);
		}
	}
	
	delete[] res;
	delete[] benchmark;
	for (int i=0; i<FEAT_MAX; i++) delete[] feats[i];
	delete[] feats;
	
	
	printf("\n");
	return 0;
}

