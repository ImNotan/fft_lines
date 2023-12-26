#include <windows.h>

#include <fftw3.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "resource.h"
#include "serial.h"
#include "global.h"
#include "fft_calculate.h"
#include "settings.h"
#include "drawbar.h"
#include "DeviceSel.h"
#include "showerror.h"

//Define .cpp functions for audio recording with WASAPI
HRESULT initializeRecording();
void uninitializeRecording();
HRESULT GetAudioBuffer(int16_t* buffer, BOOL* bdone);
HRESULT startRecording();
void Exit();

const char g_szClassName[] = "myWindowClass";

//Initialize global handle for use in whole document
HWND globalhwnd;


#define ID_TIMER_UPDATE 1

int16_t largeBuffer[2048];


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		//Defines Colors for the Bars
		for (int i = 0; i < 255; i++)
		{
			setColor((1 / (float)(255)) * i, &rgb);
			barBrush[i] = CreateSolidBrush(RGB(rgb[0], rgb[1], rgb[2]));
		}

		//Look if User wants Serial and if it is connected
		if (ignoreSerial)
			doSerial = false;
		else
			doSerial = InitializeSerial(hwnd);

		//Looks for settings and applies them
		initializeSettingsFile(hwnd);
		readSettings();

		bar = (BARINFO*)malloc(barCount * sizeof(BARINFO));

		//Starts Recording Audio
		initializeRecording();
		startRecording();

		//Sets the update Timer to call every 5ms
		if (SetTimer(hwnd, ID_TIMER_UPDATE, 5, NULL) == 0)
		{
			MessageBox(hwnd, L"Could not SetTimer()", L"Error", MB_OK | MB_ICONINFORMATION);
		}
	}
	break;
	case WM_TIMER:
	{
		switch(wParam)
		{
			case ID_TIMER_UPDATE:
			{

				BOOL bdone = false;
				GetAudioBuffer(&largeBuffer, &bdone);

				if (bdone)
				{
					RECT rcClient;
					HDC hdc = GetDC(hwnd);

					//Get size of User Window
					GetClientRect(hwnd, &rcClient);

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
							bar[i].height = (int)(sqrt(pow(output[i][REAL], 2) + pow(output[i][IMAG], 2)) * zoom);
							//multiplies it by function to lower low frequncies and boost high frequencies
							bar[i].height *= sqrt((float)i + 1);
						}

						free(input);
						free(output);
					}
					else
					{
						MessageBoxA(hwnd, "Failed to allocate memory for input", "Warning", MB_OK);
						SendMessageW(hwnd, WM_DESTROY, NULL, NULL);
					}

					//smoothing
					SGS_smothing();

					DrawBar(hdc, &rcClient);

					ReleaseDC(hwnd, hdc);
				}
				break;
			}
		}
		
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			//Creates settings dialouges from top bar
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
		RECT clientRect;

		GetClientRect(hwnd, &clientRect);
		//adjusts the window size for the bottomBar in which the frequencies are displayed
		clientRect.top = clientRect.bottom - bottomBarHeihgt;

		HGDIOBJ  original = NULL;
		original = SelectObject(hdc, GetStockObject(DC_BRUSH));

		SetDCBrushColor(hdc, RGB(50, 50, 50));

		//sets background to gray
		FillRect(hdc, &clientRect, GetStockObject(DKGRAY_BRUSH));

		//set new size of bars
		if (barCount > 0)
		{
			for (int i = 0; i < barCount; i++)
			{
				bar[i].width = (unsigned int)((clientRect.right / barCount) + 1);
				bar[i].x = i * (bar[i].width - 1);
			}

			for (int i = 0; i < clientRect.right % barCount; i++)
			{
				bar[i].width += 2;
				bar[i].x += i;
			}

			for (int i = clientRect.right % barCount; i < barCount; i++)
			{
				bar[i].x += clientRect.right % barCount;
			}
		}

		SelectObject(hdc, original);

		ReleaseDC(hwnd, hdc);
	}
	break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 960;
		lpMMI->ptMinTrackSize.y = 480;
	}
	break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		free(bar);
		writeSettings();
		uninitializeRecording();
		DeleteObject(barBrush);
		if(doSerial)
			CloseSerial(hwnd);
		KillTimer(hwnd, ID_TIMER_UPDATE);
		Exit();
		DestroyWindow(SettingsDlg);
		DestroyWindow(DeviceSelDlg);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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
