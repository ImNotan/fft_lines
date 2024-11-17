#include <windows.h>

#include <fftw3.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <mfapi.h>

#include "resource.h"

#include "serialDialog.h"
#include "global.h"
#include "fft_calculate.h"
#include "beatDetector.h"

#include "styleDialog.h"
#include "settingsFile.h"
#include "deviceDialog.h"

#define FILE_ERROR_CODE 0x00000001

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

#define CHECK_ERROR(hr) \
                  if(FAILED(hr))  \
                    { PostMessageW(globalhwnd, WM_ERROR, hr, FILE_ERROR_CODE); return hr; }

//Define wasapi_audio.cpp functions for audio recording with WASAPI
HRESULT initializeRecording();
void uninitializeRecording();
HRESULT GetAudioBuffer(INT16 * audioBufferLeft, INT16 * audioBufferRight, int stereo);

//Define drawBar2D.cpp functions for drawing on screen with Direct2D
void DiscardGraphicsResources();
HRESULT OnPaint(HWND hwnd, int frameRate);
HRESULT PaintStart();
HRESULT Resize(HWND hwnd);

const char g_szClassName[] = "myWindowClass";

//Initialize global handle for use in whole document
HWND globalhwnd;

//Defines a Timer ID
#define ID_TIMER_UPDATE 1
#define ID_TIMER_UPDATE2 2

//average frame rate calculation variables
#define MAXSAMPLES 100
int tickindex = 0;
int ticksum = 0;
int ticklist[MAXSAMPLES];

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
		globalhwnd = hwnd;

		HRESULT hr = S_OK;
		//Initialize Factories in drawBar2D.cpp
		hr = PaintStart();
		CHECK_ERROR(hr);

		//Look if User wants Serial and if it is connected
		if (ignoreSerial)
			doSerial = false;
		else
			doSerial = InitializeSerial(hwnd);

		for (int i = 0; i < MAXSAMPLES; i++)
		{
			ticklist[i] = 0;
		}
		
		//Initialize high frequency counter for frame rate calculation
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&StartingTime);

		//Looks for settings file and applies them
		hr = initializeSettingsFile(hwnd);
		CHECK_ERROR(hr);
		hr = readSettings();
		CHECK_ERROR(hr);
		hr = InitializeMemory();
		CHECK_ERROR(hr);

		//Starts Recording Audio
		hr = initializeRecording();
		CHECK_ERROR(hr);

		//Initializes graphics resources in drawBar2D.cpp
		hr = OnPaint(hwnd, 1);
		CHECK_ERROR(hr);

		//Sets the update Timer to call every 10ms
		SetTimer(hwnd, ID_TIMER_UPDATE, 10, NULL);
		//SetTimer(hwnd, ID_TIMER_UPDATE2, 10, NULL);
	}
	break;
	case WM_TIMER:
	{
		HRESULT hr = S_OK;

		if (dofft)
		{
			CHECK_NULL(barLeft);
			CHECK_NULL(audioBufferLeft);
			if (stereo)
			{
				CHECK_NULL(audioBufferRight);
				CHECK_NULL(barRight);
			}
		}

		if (waveform)
		{
			CHECK_NULL(waveBar);
		}
			

		hr = GetAudioBuffer(audioBufferLeft, audioBufferRight, stereo);
		CHECK_ERROR(hr);

		//Calculates fourier transfor of audio data
		fftwf_complex* input;
		fftwf_complex* output;

		input = (fftwf_complex*)malloc(N * sizeof(fftwf_complex));
		output = (fftwf_complex*)malloc(N * sizeof(fftwf_complex));

		CHECK_NULL(input);
		CHECK_NULL(output);

		for (int i = 0; i < N; i++)
		{
			input[i][REAL] = (float)audioBufferLeft[i];
			input[i][IMAG] = 0;
		}

		fft(input, output);

		//Sets the height of the bars calculated by fourier transfor
		for (int i = 0; i < barCount; i++)
		{
			//Calculates distance to origin with Pythagoras in complex plane
			barLeft[i].height = (int)(sqrt(pow(output[i][REAL], 2) + pow(output[i][IMAG], 2)) * zoom);

			if (circle)
			{
				barLeft[i].height += 10;
			}
		}

		if (stereo)
		{
			for (int i = 0; i < N; i++)
			{
				input[i][REAL] = (float)audioBufferRight[i];
				input[i][IMAG] = 0;
			}

			fft(input, output);

			//Sets the height of the bars calculated by fourier transfor
			for (int i = 0; i < barCount; i++)
			{
				//Calculates distance to origin with Pythagoras in complex plane
				barRight[i].height = (int)(sqrt(pow(output[i][REAL], 2) + pow(output[i][IMAG], 2)) * zoom);

				if (circle)
				{
					barRight[i].height += 10;
				}
			}
		}

		if (beatDetection)
		{
			BassBeatDetector(input, output);
		}

		free(input);
		free(output);
		input = NULL;
		output = NULL;
			
		//Copys audio buffer
		if (waveform)
		{
			RECT windowRect;
			GetClientRect(hwnd, &windowRect);
			for (int i = 0; i < N; i++)
			{
				waveBar[i].height = audioBufferLeft[i] * 0.02f + (windowRect.bottom - bottomBarHeihgt) / 2 + 50;
			}
		}

		//Prints to serial
		if (barLeft[led_bar].height >= 0 && doSerial)
		{
			char line[1];
			line[0] = (char)((float)barLeft[led_bar].height * 0.1f);
			WriteSerial(line, globalhwnd);
		}

		//Calculate fps
		QueryPerformanceCounter(&EndingTime);
		ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

		ElapsedMicroseconds.QuadPart *= 1000000;
		ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

		double averageTick = CalcAverageTick((int)(ElapsedMicroseconds.QuadPart));
		frameRate = 1000000 / averageTick;

		QueryPerformanceCounter(&StartingTime);

		AddBeatSample((barLeft[3].height + barLeft[4].height + barLeft[5].height + barLeft[6].height) / 4);

		hr = OnPaint(hwnd, frameRate);
		CHECK_ERROR(hr);
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			//Creates settings dialog
			case ID_SETTINGS_STYLE:
			{
				DestroyWindow(hwndStyleDialog);
				hwndStyleDialog = CreateDialogW(GetModuleHandle(NULL), MAKEINTRESOURCE(STYLE_DIALOG), hwnd, StyleDialogProc);

				if (hwndStyleDialog != NULL)
				{
					ShowWindow(hwndStyleDialog, SW_SHOW);
				}
				else
				{
					MessageBox(hwnd, L"CreateDialog returned NULL while creating Style Dialog", L"Warning!",
						MB_OK | MB_ICONINFORMATION);
				}
			}
			break;
			//Creates Device select dialog
			case ID_SETTINGS_DEVICES:
			{
				DestroyWindow(hwndDeviceDialog);
				hwndDeviceDialog = CreateDialogW(GetModuleHandle(NULL), MAKEINTRESOURCE(DEVICE_DIALOG), hwnd, DeviceDialogProc);

				if (hwndDeviceDialog != NULL)
				{
					ShowWindow(hwndDeviceDialog, SW_SHOW);
				}
				else
				{
					MessageBox(hwnd, L"CreateDialog returned NULL while creating Device Dialog", L"Warning!", 
						MB_OK | MB_ICONINFORMATION);
				}
			}
			break;
			//Creates serial dialog
			case ID_SETTINGS_SERIAL:
			{
				DestroyWindow(hwndSerialDialog);
				hwndSerialDialog = CreateDialogW(GetModuleHandle(NULL), MAKEINTRESOURCE(SERIAL_DIALOG), hwnd, SerialDialogProc);

				if (hwndSerialDialog != NULL)
				{
					ShowWindow(hwndSerialDialog, SW_SHOW);
				}
				else
				{
					MessageBox(hwnd, L"CreateDialog returned NULL while creating Serial Dialog", L"Warning!",
						MB_OK | MB_ICONINFORMATION);
				}
			}
			break;
		}
		break;
	case WM_SIZE:
	{
		HRESULT hr = S_OK;

		if (dofft)
		{
			ResizeBars(hwnd, barLeft, barCount, 0, stereo);
			if (stereo)
			{
				ResizeBars(hwnd, barRight, barCount, 1, stereo);
			}
		}
		if (waveform)
			ResizeBars(hwnd, waveBar, N, 0, 0);

		hr = Resize(hwnd);
		CHECK_ERROR(hr);
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
	case WM_ERROR:
	{
		KillTimer(hwnd, ID_TIMER_UPDATE);
		DWORD error = GetLastError();
		WCHAR* message[100];
		wsprintfW(message, L"An Error has occured\nError Code: %x\nFile Code: %x\nLast Documented Error: %x", (int)wParam, (int)lParam, (int)error);
		MessageBoxW(NULL, message, L"An Error has occured", MB_OK | MB_ICONERROR | MB_APPLMODAL);

		DestroyWindow(hwnd);
	}
	break;
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);
	}
	break;
	case WM_DESTROY:
	{
		DiscardGraphicsResources();

		//write settings to a file
		writeSettings();
		uninitializeSettingsFile();

		//stops recording
		uninitializeRecording();

		//stops Serial transfer
		if (doSerial)
			CloseSerial(hwnd);

		KillTimer(hwnd, ID_TIMER_UPDATE);
		//KillTimer(hwnd, ID_TIMER_UPDATE2);

		//Destroy dialog if open
		DestroyWindow(hwndStyleDialog);
		DestroyWindow(hwndDeviceDialog);

		//memory
		UninitializeMemory();

		PostQuitMessage(0);
	}
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
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON_AUDIO_VISUALIZER32));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON_AUDIO_VISUALIZER16));
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_FFT_LINES);

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

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
