#include <fftw3.h>
#include "global.h"
#include "settingsFile.h"
//#define N 1024

fftwf_plan plan;
fftwf_plan plan256;

/*
void fft(fftwf_complex* in, fftwf_complex* out)
{
	//create an IFFT plan
	fftwf_plan plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	//execute the plan
	fftwf_execute(plan);
	//do some cleaning
	fftwf_destroy_plan(plan);
	fftwf_cleanup();
}
*/

void initializefft(fftwf_complex* in, fftwf_complex* out)
{
	plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

void executefft(fftwf_complex* in, fftwf_complex* out)
{
	fftwf_execute(plan);
}

void cleanupfft()
{
	fftwf_destroy_plan(plan);
	plan = NULL;
}

void initializefft256(fftwf_complex* in, fftwf_complex* out)
{
	plan256 = fftwf_plan_dft_1d(DEFAULT_BEATBBUFFERSIZE, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

void executefft256(fftwf_complex* in, fftwf_complex* out)
{
	fftwf_execute(plan256);
}

void cleanupfft256()
{
	fftwf_destroy_plan(plan256);
	plan256 = NULL;
}

/*
void fft256(fftwf_complex * in, fftwf_complex * out)
{
	//create an IFFT plan
	fftwf_plan plan = fftwf_plan_dft_1d(DEFAULT_BEATBBUFFERSIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	//execute the plan
	fftwf_execute(plan);
	//do some cleaning
	fftwf_destroy_plan(plan);
	fftwf_cleanup();
}
*/