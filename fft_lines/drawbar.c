#include <Windows.h>
#include <fftw3.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "settings.h"
#include "global.h"
#include "serial.h"
#include "fft_calculate.h"
#include "drawbar.h"

HDC globalhdc = NULL;
HDC globalhdcBuffer = NULL;
HBITMAP globalhbmBuffer = NULL;

void DrawBar()
{
	RECT windowRect;

	//Get size of User Window
	GetClientRect(globalhwnd, &windowRect);

	//Puts bitmap in buffer so graphics print of bitmap
	SelectObject(globalhdcBuffer, globalhbmBuffer);

	//Sets the background to dark gray if background effect is deactivated
	if(!background)
		FillRect(globalhdcBuffer, &windowRect, GetStockObject(DKGRAY_BRUSH));

	//adjusts the window size for the bottomBar in which the frequencies are displayed
	windowRect.bottom -= bottomBarHeihgt;

	if (background)
	{
		RECT background;

		//Creates the Background effect by copying the last frame to the new frame and moving it 5 to the right and 5 up
		//after which the new bars are drawn on top of it
		BitBlt(globalhdcBuffer, 0, 0, windowRect.right, windowRect.bottom - 5, globalhdc, -5, 5, SRCCOPY);

		//Sets the bottom 5 pixel to black without there is color smear
		SetRect(&background, windowRect.left, windowRect.bottom - 5, windowRect.right, windowRect.bottom);
		FillRect(globalhdcBuffer, &background, GetStockObject(BLACK_BRUSH));

		//Sets the right 5 pixel to white without it prints stuff from desktop
		SetRect(&background, windowRect.left, windowRect.top, windowRect.left + 5, windowRect.bottom - 5);
		FillRect(globalhdcBuffer, &background, GetStockObject(WHITE_BRUSH));
	}


	TRIVERTEX vertex[2];
	RECT barRect;
	for (int i = 0; i < barCount; i++)
	{
		//makes the bottom of the bar darker for a 3d effect
		if (gradient)
		{
			if (bar[i].height <= 5)
			{
				barRect.left = bar[i].x;
				barRect.right = bar[i].x + bar[i].width;
				barRect.top = windowRect.bottom - bar[i].height;
				barRect.bottom = windowRect.bottom;

				FillRect(globalhdcBuffer, &barRect, barBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
			}
			else
			{
				setColor((unsigned int)((((float)i / ((float)barCount - 1.0))) * 254.0), rgb);
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

				GradientFill(globalhdcBuffer, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
			}
		}

		if (!gradient || border)
		{
			//Calculates the bar size for use in border mode or no gradient mode
			barRect.left = bar[i].x;
			barRect.right = bar[i].x + bar[i].width;
			barRect.top = windowRect.bottom - bar[i].height;
			barRect.bottom = windowRect.bottom;

			if (!gradient)
				FillRect(globalhdcBuffer, &barRect, barBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);



			if (border)
				FrameRect(globalhdcBuffer, &barRect, GetStockObject(DKGRAY_BRUSH));
		}	
	}

	//copys buffer to window
	BitBlt(globalhdc, 0, 0, windowRect.right, windowRect.bottom, globalhdcBuffer, 0, 0, SRCCOPY);
}