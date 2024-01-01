#include <Windows.h>
#include <fftw3.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <process.h>

#include "settings.h"
#include "global.h"
#include "serial.h"
#include "fft_calculate.h"
#include "drawbar.h"

#include "sgfilter.h"

RECT barWindowRect[2];

HANDLE hThreads[2] = { NULL };
int threadNr = 0;

void SGS_smothing()
{
	int nl = 6;  //DEFAULT_NL;
	int nr = 6;  //DEFAULT_NR;
	int ld = DEFAULT_LD;
	int m = 8;
	long int k = 0;
	long int mm = barCount;
	double* yr, * yf;

	yr = dvector(1, mm);
	yf = dvector(1, mm);

	for (k = 1; k <= mm; k++)
	{
		yr[k] = (double)bar[k - 1].height;
	}

	sgfilter(yr, yf, mm, nl, nr, ld, m);

	for (k = 1; k <= mm; k++)
	{
		bar[k - 1].height = (int)yf[k];
	}

	free_dvector(yr, 1, mm);
	free_dvector(yf, 1, mm);
}

void BarProc(void* pMyID)
{
	int MyID = (int)(uintptr_t)pMyID;

	HDC hdc = GetDC(globalhwnd);

	HDC hdcBuffer = CreateCompatibleDC(hdc);
	HBITMAP hbmBuffer = CreateCompatibleBitmap(hdc, barWindowRect[MyID].right, barWindowRect[MyID].bottom);
	HBITMAP hbmOldBuffer = SelectObject(hdcBuffer, hbmBuffer);

	if (!background)
		FillRect(hdcBuffer, &barWindowRect[MyID], GetStockObject(DKGRAY_BRUSH));

	//adjusts the window size for the bottomBar in which the frequencies are displayed
	//barWindowRect[MyID].bottom -= bottomBarHeihgt;

	//Creates the Background effect by copying the last frame to the new frame and moving it 5 to the right and 5 up
	//after which the new bars are drawn on top of it
	if (background)
		BitBlt(hdcBuffer, 0, 0, barWindowRect[MyID].right, barWindowRect[MyID].bottom - 5 - bottomBarHeihgt, hdc, -5, 5, SRCCOPY);


	TRIVERTEX vertex[2];
	RECT barRect;
	int localBarCount = 0;
	int startBar;
	if (MyID == 0)
	{
		startBar = 0;
		localBarCount = barCount / 2;
	}
	else
	{
		if ((barCount % 2) != 0)
			startBar = (barCount / 2) - 1;
		else
			startBar = barCount / 2;

		localBarCount = barCount;
	}
	for (int i = startBar; i < localBarCount; i++)
	{
		//makes the bottom of the bar darker for a 3d effect
		if (gradient)
		{
			setColor(1 / (float)(255) * (float)((((float)i / ((float)barCount - 1.0))) * 254.0), rgb);
			vertex[0].x = bar[i].x;
			vertex[0].y = barWindowRect[MyID].bottom - bar[i].height - bottomBarHeihgt;
			vertex[0].Red = rgb[0] * 256;
			vertex[0].Green = rgb[1] * 256;
			vertex[0].Blue = rgb[2] * 256;
			vertex[0].Alpha = 0x0000;

			vertex[1].x = bar[i].x + bar[i].width;
			vertex[1].y = barWindowRect[MyID].bottom - bottomBarHeihgt;
			vertex[1].Red = (COLOR16)((float)vertex[0].Red * 0.5f);
			vertex[1].Green = (COLOR16)((float)vertex[0].Green * 0.5f);
			vertex[1].Blue = (COLOR16)((float)vertex[0].Blue * 0.5f);
			vertex[1].Alpha = 0x0000;

			GRADIENT_RECT gRect;
			gRect.LowerRight = 1;
			gRect.UpperLeft = 0;

			GradientFill(hdcBuffer, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
		}

		//Calculates the bar size for use in border mode or no gradient mode

		if (!gradient || border)
		{
			barRect.left = bar[i].x;
			barRect.right = bar[i].x + bar[i].width;
			barRect.top = barWindowRect[MyID].bottom - bar[i].height - bottomBarHeihgt;
			barRect.bottom = barWindowRect[MyID].bottom - bottomBarHeihgt;

			if (!gradient)
				FillRect(hdcBuffer, &barRect, barBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);



			if (border)
				FrameRect(hdcBuffer, &barRect, GetStockObject(DKGRAY_BRUSH));
		}
	}

	//copys buffer to window
	BitBlt(hdc, 0, 0, barWindowRect[MyID].right, barWindowRect[MyID].bottom - bottomBarHeihgt, hdcBuffer, 0, 0, SRCCOPY);

	SelectObject(hdcBuffer, hbmOldBuffer);
	DeleteObject(hbmOldBuffer);
	DeleteDC(hdcBuffer);
	DeleteObject(hbmBuffer);
	ReleaseDC(globalhwnd, hdc);
}

void DrawBar()
{
	//writes height of one bar to serial if activated
	if (bar[led_bar].height >= 0 && doSerial)
	{
		char line[1];
		line[0] = (char)((float)bar[led_bar].height * 0.1f);
		WriteSerial(line, globalhwnd);
	}

	threadNr = 0;
	hThreads[threadNr] = (HANDLE)_beginthread(BarProc, 0, (void*)(uintptr_t)threadNr);
	threadNr = 1;
	hThreads[threadNr] = (HANDLE)_beginthread(BarProc, 0, (void*)(uintptr_t)threadNr);

	//WaitForMultipleObjects(2, *hThreads, TRUE, INFINITE);
	WaitForSingleObject(hThreads[0], INFINITE);
	WaitForSingleObject(hThreads[1], INFINITE);
}