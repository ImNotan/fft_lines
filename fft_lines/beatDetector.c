#define NUMBER_OF_SAMPLES 120

int beatValues[NUMBER_OF_SAMPLES];
int currentBeatValue = 0;
int sumOfBeatValues = 0;

int BeatThreshold = 0;

int beatValue = 0;

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