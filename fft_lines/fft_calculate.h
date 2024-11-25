#include <fftw3.h>
#define REAL 0
#define IMAG 1

void initializefft(fftwf_complex* in, fftwf_complex* out);
void executefft(fftwf_complex* in, fftwf_complex* out);
void cleanupfft();

void initializefft256(fftwf_complex* in, fftwf_complex* out);
void executefft256(fftwf_complex* in, fftwf_complex* out);
void cleanupfft256();