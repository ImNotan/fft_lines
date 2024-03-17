#include <windows.h>

#include <fftw3.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <mfapi.h>

#include "resource.h"
#include "serial.h"
#include "global.h"
#include "fft_calculate.h"
#include "settings.h"
#include "drawbar.h"
#include "DeviceSel.h"
#include "showerror.h"
#include "sgfilter.h"

//Define .cpp functions for audio recording with WASAPI
HRESULT initializeRecording();
void uninitializeRecording();
HRESULT GetAudioBuffer(int16_t* buffer, BOOL* bdone);
HRESULT startRecording();
void Exit();
void getWaveFormat(WAVEFORMATEX* waveformat);

const char g_szClassName[] = "myWindowClass";

//Initialize global handle for use in whole document
HWND globalhwnd;

//Defines a Timer ID
#define ID_TIMER_UPDATE 1
#define ID_TIMER_UPDATE2 2

//Audio Buffer
int16_t largeBuffer[2048];

#define MAXSAMPLES 100
int tickindex = 0;
int ticksum = 0;
int ticklist[MAXSAMPLES];

SYSTEMTIME start, stop;
FILETIME startF, stopF;
ULARGE_INTEGER startI, stopI;

LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
LARGE_INTEGER Frequency;

int frameRate = 0;

double CalcAverageTick(int newtick)
{
	ticksum -= ticklist[tickindex];				/* subtract value falling off */
	ticksum += newtick;							/* add new value */
	ticklist[tickindex] = newtick;				/* save new value so it can be subtracted later */
	tickindex = (tickindex + 1) % MAXSAMPLES;   /* inc buffer index */

	/* return average */
	return((double)ticksum / MAXSAMPLES);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		//Look if User wants Serial and if it is connected
		if (ignoreSerial)
			doSerial = false;
		else
			doSerial = InitializeSerial(hwnd);

		for (int i = 0; i < MAXSAMPLES; i++)
		{
			ticklist[i] = 0;
		}
		
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&StartingTime);

		//Looks for settings and applies them
		initializeSettingsFile(hwnd);
		readSettings();

		//the bar array saves data about every bar
		bar = (BARINFO*)malloc(barCount * sizeof(BARINFO));

		//Starts Recording Audio
		initializeRecording();
		startRecording();

		//Sets the update Timer to call every 5ms
		//if (SetTimer(hwnd, ID_TIMER_UPDATE, 1, NULL) == 0)
		//{
		//	MessageBox(hwnd, L"Could not SetTimer()", L"Error", MB_OK | MB_ICONINFORMATION);
		//}
		//SetTimer(hwnd, ID_TIMER_UPDATE2, 1, NULL);
	}
	break;
	case WM_PAINT:
	{
		QueryPerformanceCounter(&EndingTime);
		ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

		ElapsedMicroseconds.QuadPart *= 1000000;
		ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

		double averageTick = CalcAverageTick((int)(ElapsedMicroseconds.QuadPart));
		frameRate = 1000000 / averageTick;

		RECT windowRect;

		GetClientRect(hwnd, &windowRect);

		RECT textRect;

		textRect.top = windowRect.bottom;
		textRect.bottom = windowRect.bottom - bottomBarHeihgt;
		textRect.right = windowRect.left + 100;
		textRect.left = windowRect.left;

		wchar_t buffer[10];
		wsprintfW(buffer, L"%d", (int)frameRate);

		DrawTextW(globalhdc, buffer, 10, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);


		QueryPerformanceCounter(&StartingTime);

		BOOL bdone = false;
		GetAudioBuffer(&largeBuffer, &bdone);

		if (bdone)
		{
			//Calculates fourier transfor of audio data
			fftwf_complex *input;
			fftwf_complex *output;

			input = (fftwf_complex*)malloc(N * sizeof(fftwf_complex));
			output = (fftwf_complex*)malloc(N * sizeof(fftwf_complex));

			if (input && output)
			{
				for (int i = 0; i < N; i++)
				{
					input[i][REAL] = (float)largeBuffer[i];
					input[i][IMAG] = 0;
				}

				fft(input, output);

				//Sets the height of the bars calculated by fourier transfor
				for (int i = 0; i < barCount; i++)
				{
					//Calculates distance to origin with Pythagoras in complex plane
					bar[i].height = (int)(sqrt(pow(output[i][REAL], 2) + pow(output[i][IMAG], 2)) * zoom);

					//multiplies it by function to lower low frequncies and boost high frequencies
					//bar[i].height *= 0.5 * sqrt((float)0.25 * i + 1);
				}

				free(input);
				free(output);
			}
			else
			{
				MessageBoxA(hwnd, "Failed to allocate memory for input or output", "Warning", MB_OK);
				SendMessageW(hwnd, WM_DESTROY, NULL, NULL);
			}

			//smoothing with Savitzky-Golay
			//SGS_smothing();

			//Prints to serial
			if (bar[led_bar].height >= 0 && doSerial)
			{
				char line[1];
				line[0] = (char)((float)bar[led_bar].height * 0.1f);
				WriteSerial(line, globalhwnd);
			}

			//Draws bars on screen
			DrawBar();
		}
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			//Creates settings dialog
			case ID_SETTINGS_SETTINGS:
			{
				DestroyWindow(SettingsDlg);
				SettingsDlg = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), hwnd, SettingsDlgProc);

				if (SettingsDlg != NULL)
				{
					ShowWindow(SettingsDlg, SW_SHOW);
				}
				else
				{
					MessageBox(hwnd, L"CreateDialog returned NULL SettingsDlg", L"Warning!",
						MB_OK | MB_ICONINFORMATION);
				}
			}
			break;
			//Creates Device select dialog
			case ID_SETTINGS_DEVICES:
			{
				DestroyWindow(DeviceSelDlg);
				DeviceSelDlg = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), hwnd, DeviceSelProc);

				if (DeviceSelDlg != NULL)
				{
					ShowWindow(DeviceSelDlg, SW_SHOW);
				}
				else
				{
					MessageBox(hwnd, L"CreateDialog returned NULL DeviceSelDlg", L"Warning!", 
						MB_OK | MB_ICONINFORMATION);
				}
			}
			break;
		}
		break;
	case WM_SIZE:
	{
		HDC hdc = GetDC(hwnd);

		DeleteDC(globalhdcBuffer);
		ReleaseDC(hwnd, globalhdc);
		globalhdc = GetDC(hwnd);
		globalhdcBuffer = CreateCompatibleDC(globalhdc);

		RECT windowRect;

		GetClientRect(hwnd, &windowRect);

		//adjusts the window size for the bottomBar in which the frequencies are displayed
		windowRect.top = windowRect.bottom - bottomBarHeihgt;

		globalhbmBuffer = CreateCompatibleBitmap(globalhdc, windowRect.right, windowRect.bottom);

		//sets bottombar to gray
		FillRect(hdc, &windowRect, GetStockObject(DKGRAY_BRUSH));
		FrameRect(hdc, &windowRect, GetStockObject(BLACK_BRUSH));

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));
		RECT textRect;
		//Get information about audio stream
		WAVEFORMATEX wfx;
		getWaveFormat(&wfx);

		for (int i = 0; i < 10; i++)
		{
			textRect.top = windowRect.bottom;
			textRect.bottom = windowRect.bottom - bottomBarHeihgt;
			textRect.left = i * (windowRect.right / 10);
			textRect.right = i * (windowRect.right / 10) + (windowRect.right / 10);

			//Gets frequencies from audio function
			int freq = (i * (barCount / 10) + (barCount / 20)) * (wfx.nSamplesPerSec / N);
			wchar_t buffer[11];
			wsprintfW(buffer, L"%dHz ", freq);

			DrawText(hdc, buffer, 7, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		//set new size of bars
		if (barCount > 0)
		{
			//For every bar set width
			for (int i = 0; i < barCount; i++)
			{
				bar[i].width = (unsigned int)((windowRect.right / barCount) + 1);
				bar[i].x = i * (bar[i].width - 1);
			}

			//Calculates how many bar have to be larger
			//Shifts bar to the right by how many bars already made bigger
			for (int i = 0; i < windowRect.right % barCount; i++)
			{
				bar[i].width += 1;
				bar[i].x += i;
			}

			//Shifts all bars that weren't made larger to the right
			for (int i = windowRect.right % barCount; i < barCount; i++)
			{
				bar[i].x += windowRect.right % barCount;
			}
		}

		ReleaseDC(hwnd, hdc);
	}
	break;
	case WM_GETMINMAXINFO:
	{
		//Sets minimum window size
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 960;
		lpMMI->ptMinTrackSize.y = 480;
	}
	break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		//Free Device Context used by Drawbar
		DeleteObject(globalhbmBuffer);
		ReleaseDC(hwnd, globalhdc);
		DeleteDC(globalhdcBuffer);
		DeleteObject(barBrush);

		//memory
		free(bar);

		//write settings to a file
		writeSettings();

		//stops recording
		uninitializeRecording();
		Exit();

		//stops Serial transfer
		if(doSerial)
			CloseSerial(hwnd);

		KillTimer(hwnd, ID_TIMER_UPDATE);
		
		//Destroy dialog if open
		DestroyWindow(SettingsDlg);
		DestroyWindow(DeviceSelDlg);

		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		NULL,
		g_szClassName,
		L"Audio Visualizer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 960, 480,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	globalhwnd = hwnd;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
