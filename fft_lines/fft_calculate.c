#include <fftw3.h>
#include "global.h"
//#define N 1024

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