#include <Windows.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <commctrl.h>
#include <stdbool.h>
#include <Shlwapi.h>

#include "variables.h"
#include "global.h"
#include "settingsFile.h"

#define FILE_ERROR_CODE 0x00000005

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

#define CHECK_ERROR(hr) \
                  if(FAILED(hr))  \
                    { PostMessageW(globalhwnd, WM_ERROR, hr, FILE_ERROR_CODE); return hr; }



/*-----------------------------------------------
Internal Handles

	File:
		hSettingsFile
-----------------------------------------------*/
HANDLE hSettingsFile;

/*-----------------------------------------------
Public Handles and Variables

defined in settings.h

	Pointers:
		Heap memory:
			bar
			wavebar

		other:
			pGradients (points to color gradient used in drawBar2D - CreateBarBrush

	Variables (settings):
		zoom
		barCount
		colorSel
		dofft
		background
		gradient
		ignoreSerial
		circle
		waveform

	Variable (information used in drawBar2D):
		redrawAll

	Const Variables:
		bottomBarheight
		led_bar
-----------------------------------------------*/
BARINFO* bar;
BARINFO* waveBar;

double* pGradients = &plasma;

float zoom = DEFAULT_ZOOM;
int barCount = DEFAULT_BARCOUNT;
LRESULT colorSel = DEFAULT_COLORSEL;
bool dofft = DEFAULT_FFT;
bool background = DEFAULT_BACKGROUND;
bool gradient = DEFAULT_GRADIENT;
bool ignoreSerial = DEFAULT_IGNORESERIAL;
bool circle = DEFAULT_CIRCLE;
bool waveform = DEFAULT_WAVEFORM;

bool redrawAll = false;

const int bottomBarHeihgt = DEFAULT_BOTTOMBARHEIGHT;
const int led_bar = DEFAULT_LEDBAR;

/*-----------------------------------------------
Public functions

	Initialization:
		initializeSettingsFile
		uninitializeSettingsFile

	I/O SettingsFile:
		readSettings
		writeSettings

	manipulation of barsize:
		ResizeBars
-----------------------------------------------*/
void initializeSettingsFile(HWND hwnd);
void uninitializeSettingsFile();

void readSettings();
void writeSettings();

void ResizeBars(HWND hwnd, BARINFO* bars, int size);

/*-----------------------------------------------
Internal functions

	manipulation of variables:
		setColor
-----------------------------------------------*/
void setVariable(char* value, int variableNumber);
void setColor();


/*-----------------------------------------------
	Initialization of the settings file

	Called in:
	fft_lines - WndProc - WM_CREATE
-----------------------------------------------*/
void initializeSettingsFile(HWND hwnd)
{
	hSettingsFile = CreateFileW(L"Settings.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hSettingsFile == INVALID_HANDLE_VALUE)
	{
		hSettingsFile = CreateFileW(L"Settings.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hSettingsFile == INVALID_HANDLE_VALUE)
		{
			MessageBoxW(hwnd, L"fail", L"fail", MB_OK);
			return;
		}
	}
}

/*-----------------------------------------------
	Uninitialization of the settings file

	Called in:
	fft_lines - WndProc - WM_DESTROY
-----------------------------------------------*/
void uninitializeSettingsFile()
{
	CloseHandle(hSettingsFile);
}

/*-----------------------------------------------
	Reads settings from settings file
	applies them to the variables

	Called in:
	fft_lines - WndProc - WM_CREATE
	styleDialog - StyleDialogProc - WM_COMMANd - IDCANCEL
-----------------------------------------------*/
void readSettings()
{
	LARGE_INTEGER PfileSize;
	GetFileSizeEx(hSettingsFile, &PfileSize);
	INT64 fileSize = PfileSize.QuadPart;

	if (fileSize > 1000)
		return 0;

	DWORD bytes_read = 0;
	LARGE_INTEGER move;
	move.LowPart = 0;
	move.HighPart = 0;
	SetFilePointerEx(hSettingsFile, move, NULL, FILE_BEGIN);

	char* buffer = malloc(sizeof(char) * fileSize);
	CHECK_NULL(buffer);
	char* pNextNumber;

	if (!ReadFile(hSettingsFile, buffer, fileSize, bytes_read, NULL))
	{
		free(buffer);
		return 0;
	}

	barCount = strtol(buffer, &pNextNumber, 10);
	if (barCount < MIN_BARCOUNT || barCount > MAX_BARCOUNT)
		barCount = DEFAULT_BARCOUNT;

	zoom = (float)strtof(pNextNumber, &pNextNumber);
	if (zoom < MIN_ZOOM || zoom > MAX_ZOOM)
		zoom = DEFAULT_ZOOM;

	colorSel = _strtoi64(pNextNumber, &pNextNumber, 10);
	if (colorSel < 0 || colorSel >= NUMOF_GRADIENTS)
		colorSel = DEFAULT_COLORSEL;
	setColor();

	dofft = strtol(pNextNumber, &pNextNumber, 10);
	if (dofft < 0 || dofft > 1)
		dofft = DEFAULT_FFT;

	background = strtol(pNextNumber, &pNextNumber, 10);
	if (background < 0 || background > 1)
		background = DEFAULT_BACKGROUND;

	gradient = strtol(pNextNumber, &pNextNumber, 10);
	if (gradient < 0 || gradient > 1)
		gradient = DEFAULT_GRADIENT;

	ignoreSerial = strtol(pNextNumber, &pNextNumber, 10);
	if (ignoreSerial < 0 || ignoreSerial > 1)
		ignoreSerial = DEFAULT_IGNORESERIAL;

	circle = strtol(pNextNumber, &pNextNumber, 10);
	if (circle < 0 || circle > 1)
		circle = DEFAULT_CIRCLE;

	waveform = strtol(pNextNumber, NULL, 10);
	if (waveform < 0 || waveform > 1)
		waveform = DEFAULT_WAVEFORM;

	if (waveform)
	{
		BARINFO* tmp = (BARINFO*)malloc(N * sizeof(BARINFO));
		if (tmp)
		{
			waveBar = tmp;
			ResizeBars(globalhwnd, waveBar, N);
		}
		else
		{
			MessageBoxA(globalhwnd, "Failed to allocate memory for bar", "Warning", MB_OK);
			SendMessageW(globalhwnd, WM_DESTROY, 0, 0);
		}
	}

	free(buffer);
	return 1;
}

/*-----------------------------------------------
	Writes settings to settings file

	Called in:
	fft_lines - WndProc - WM_DESTROY
	styleDialog - StyleDialogProc - WM_COMMANd - IDOK
-----------------------------------------------*/
void writeSettings()
{
	LARGE_INTEGER move;
	move.LowPart = 0;
	move.HighPart = 0;
	SetFilePointerEx(hSettingsFile, move, NULL, FILE_BEGIN);

	char str[1000];
	sprintf_s(str, 1000, "%d %0.6f %I64d %d %d %d %d %d %d\0", barCount, zoom, colorSel, dofft, background, gradient, ignoreSerial, circle, waveform);
	WriteFile(hSettingsFile, str, 1000, NULL, NULL);
}


/*-----------------------------------------------
	Sets the pointer to color gradient to new color

	Called in:
	settingsFile - readSettings
-----------------------------------------------*/
void setColor()
{
	switch (colorSel)
	{
		case 0:
			pGradients = &plasma;
			break;
		case 1:
			pGradients = &magma;
			break;
		case 2:
			pGradients = &inferno;
			break;
		case 3:
			pGradients = &viridis;
			break;
		case 4:
			pGradients = &cividis;
			break;
		case 5:
			pGradients = &turbo;
			break;
	}
}

/*-----------------------------------------------
	Resizes the bars to new window size

	Called in:
	fft_lines - WndProc - WM_SIZE
-----------------------------------------------*/
void ResizeBars(HWND hwnd, BARINFO* bars, int size)
{
	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	if (size > 0 && windowRect.right > 1)
	{
		//For every bar set width
		for (int i = 0; i < size; i++)
		{
			bars[i].width = (unsigned int)((windowRect.right / size) + 1);
			bars[i].x = i * (bars[i].width - 1);
		}

		//Calculates how many bar have to be larger

		//Can't divide by 0
		if (windowRect.right % size == 0)
			return;

		int shift = 0;
		float i;
		//iterates over number of bars divided by how many bars need to be bigger
		for (i = 0; i < size; i += (float)size / (float)(windowRect.right % size))
		{
			//End loop if enough bars have been moved edge case
			if (shift > windowRect.right % size)
			{
				break;
			}
			//Increases width and shift
			bars[(int)i].width += 1;
			bars[(int)i].x += shift;

			//iterates over bars between two larger bars and shifts them
			for (int j = (int)(i - (float)size / (float)(windowRect.right % size)) + 1; j < (int)i; j++)
			{
				bars[j].x += shift;
			}

			shift++;
		}

		//shifts over the last bars which haven't been shifted yet
		i -= (float)size / (float)(windowRect.right % size);

		for (int j = (int)i + 1; j < size; j++)
		{
			bars[j].x += shift;
		}
	}
}