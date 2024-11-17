#pragma once
#include "fftw3.h"
extern void AddBeatSample(int weightedAverage);
extern int GetBeatValue();

extern void BassBeatDetector(fftwf_complex* input, fftwf_complex* output);

extern float beatMovementPerSec;
extern int beatposition;
extern float timeDelta;
extern int direction;