#include <fftw3.h>
#include "fft_calculate.h"
#include "settingsFile.h"
#include "global.h"

#define NUMBER_OF_SAMPLES 160

#define FILE_ERROR_CODE 0x00000010

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

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
	bassBeatBuffer[beatFramesRecorded] = (int)sqrt(pow(output[4][REAL], 2) + pow(output[4][IMAG], 2));
	beatFramesRecorded++;

	if (beatFramesRecorded >= 256)
	{
		beatFramesRecorded = 0;
	}

	for (int i = 0; i < DEFAULT_BEATBBUFFERSIZE; i++)
	{
		input[i][REAL] = (float)bassBeatBuffer[i];
		input[i][IMAG] = 0;
	}

	executefft256(input, output);

	int heightBuffer[NUMBER_OF_SAMPLES];
	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		//Calculates distance to origin with Pythagoras in complex plane
		heightBuffer[i] = (int)sqrt(pow(output[i][REAL], 2) + pow(output[i][IMAG], 2));
		//barLeft[i].height = (int)((float)heightBuffer[i] * zoom);
	}


	int diffnew = 0;
	int diffold = 0;
	int peaks[30];
	int peaknumber = 0;
	int highestpeakvalue = 0;
	int highestpeaknumber = 0;
	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		diffold = diffnew;
		diffnew = heightBuffer[i + 1] - heightBuffer[i];

		if (diffold >= 0 && diffnew <= 0)
		{
			peaks[peaknumber] = i;
			peaknumber++;
			if (heightBuffer[i] > highestpeakvalue && i > 30)
			{
				highestpeakvalue = heightBuffer[i];
				highestpeaknumber = peaknumber - 1;
			}
			if (peaknumber >= 30)
			{
				break;
			}
		}
	}

	beatMovementPerSec = N * timeDelta * peaks[highestpeaknumber];
}