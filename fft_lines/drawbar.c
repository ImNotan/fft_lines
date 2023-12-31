#include <Windows.h>
#include <fftw3.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "settings.h"
#include "global.h"
#include "serial.h"
#include "fft_calculate.h"
#include "drawbar.h"

#include "sgfilter.h"

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

void DrawBar()
{
	//writes height of one bar to serial if activated
	if (bar[led_bar].height >= 0 && doSerial)
	{
		char line[1];
		line[0] = (char)((float)bar[led_bar].height * 0.1f);
		WriteSerial(line, globalhwnd);
	}

	RECT windowRect;
	HDC hdc = GetDC(globalhwnd);

	//Get size of User Window
	GetClientRect(globalhwnd, &windowRect);

	HDC hdcBuffer = CreateCompatibleDC(hdc);
	HBITMAP hbmBuffer = CreateCompatibleBitmap(hdc, windowRect.right, windowRect.bottom);
	HBITMAP hbmOldBuffer = SelectObject(hdcBuffer, hbmBuffer);

	//Sets the background to dark gray if background effect is deactivated
	if(!background)
		FillRect(hdcBuffer, &windowRect, GetStockObject(DKGRAY_BRUSH));

	//adjusts the window size for the bottomBar in which the frequencies are displayed
	windowRect.bottom -= bottomBarHeihgt;

	//Creates the Background effect by copying the last frame to the new frame and moving it 5 to the right and 5 up
	//after which the new bars are drawn on top of it
	if(background)
		BitBlt(hdcBuffer, 0, 0, windowRect.right, windowRect.bottom - 5, hdc, -5, 5, SRCCOPY);


	TRIVERTEX vertex[2];
	RECT barRect;
	for (int i = 0; i < barCount; i++)
	{
		//makes the bottom of the bar darker for a 3d effect
		if (gradient)
		{
			setColor(1 / (float)(255) * (float)((((float)i / ((float)barCount - 1.0))) * 254.0), rgb);
			vertex[0].x = bar[i].x;
			vertex[0].y = windowRect.bottom - bar[i].height;
			vertex[0].Red = rgb[0] * 256;
			vertex[0].Green = rgb[1] * 256;
			vertex[0].Blue = rgb[2] * 256;
			vertex[0].Alpha = 0x0000;

			vertex[1].x = bar[i].x + bar[i].width;
			vertex[1].y = windowRect.bottom;
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
			barRect.top = windowRect.bottom - bar[i].height;
			barRect.bottom = windowRect.bottom;

			if (!gradient)
				FillRect(hdcBuffer, &barRect, barBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);



			if (border)
				FrameRect(hdcBuffer, &barRect, GetStockObject(DKGRAY_BRUSH));
		}	
	}

	//copys buffer to window
	BitBlt(hdc, 0, 0, windowRect.right, windowRect.bottom, hdcBuffer, 0, 0, SRCCOPY);

	SelectObject(hdcBuffer, hbmOldBuffer);
	DeleteObject(hbmOldBuffer);
	DeleteDC(hdcBuffer);
	DeleteObject(hbmBuffer);
	ReleaseDC(globalhwnd, hdc);
}