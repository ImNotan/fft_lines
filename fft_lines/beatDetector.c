#include <fftw3.h>
#include "fft_calculate.h"
#include "settingsFile.h"
#include "global.h"

#define NUMBER_OF_SAMPLES 120

int beatValues[NUMBER_OF_SAMPLES];
int currentBeatValue = 0;
int sumOfBeatValues = 0;

int BeatThreshold = 0;

int beatValue = 0;


int beatFramesRecorded = 0;

float beatMovementPerSec = 0;
int beatposition = 0;
float timeDelta = 0;
int direction;

void AddBeatSample(int weightedAverage)
{
	sumOfBeatValues += weightedAverage;
	sumOfBeatValues -= beatValues[currentBeatValue];
	beatValues[currentBeatValue] = weightedAverage;
	currentBeatValue++;
	if (currentBeatValue == NUMBER_OF_SAMPLES)
	{
		currentBeatValue = 0;
	}
	
	BeatThreshold = sumOfBeatValues / NUMBER_OF_SAMPLES;

	beatValue = (int)(((float)weightedAverage / (float)BeatThreshold) * 128);
}

int GetBeatValue()
{
	return beatValue;
}

void BassBeatDetector(fftwf_complex* input, fftwf_complex* output)
{
	bassBeatBuffer[beatFramesRecorded] = (int)sqrt(pow(output[3][REAL], 2) + pow(output[3][IMAG], 2));
	beatFramesRecorded++;

	if (beatFramesRecorded >= 250)
	{
		beatFramesRecorded = 0;
	}

	for (int i = 0; i < N; i++)
	{
		input[i][REAL] = (float)bassBeatBuffer[i];
		input[i][IMAG] = 0;
	}

	fft(input, output);

	for (int i = 0; i < barCount; i++)
	{
		//Calculates distance to origin with Pythagoras in complex plane
		barLeft[i].height = (int)(sqrt(pow(output[i][REAL], 2) + pow(output[i][IMAG], 2)) * zoom);
	}


	int diffnew = 0;
	int diffold = 0;
	int peaks[30];
	int peaknumber = 0;
	for (int i = 0; i < 200; i++)
	{
		diffold = diffnew;
		diffnew = barLeft[i + 1].height - barLeft[i].height;

		if (diffold >= 0 && diffnew <= 0)
		{
			//barLeft[i].height = 100;
			peaks[peaknumber] = i;
			peaknumber++;
			if (peaknumber >= 30)
				break;
		}
		else
		{
			//barLeft[i].height = 0;
		}
	}

	int highestpeak = 0;
	for (int i = 8; i < 30; i++)
	{
		if (barLeft[i].height > highestpeak)
		{
			highestpeak = barLeft[i].height;
			peaknumber = i;
		}
	}
	beatMovementPerSec = N * timeDelta * peaks[peaknumber];
}