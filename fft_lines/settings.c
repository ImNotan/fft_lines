#include <Windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <stdbool.h>
#include <Shlwapi.h>

#include "variables.h"
#include "resource.h"
#include "settings.h"
#include "global.h"
#include "serial.h"

#define IDC_LEFT_BARCOUNT_LABEL			(HMENU)1000
#define IDC_RIGHT_BARCOUNT_LABEL		(HMENU)1001
#define IDC_SLIDER_BARCOUNT				(HMENU)1002
#define IDC_SLIDER_ZOOM					(HMENU)1003

#define IDC_SET_BARCOUNT_DESCRIBTION	(HMENU)1007
#define IDC_EDIT_BARCOUNT				(HMENU)1008

#define IDC_BUTTON_BORDER				(HMENU)1010
#define IDC_BUTTON_BACKGROUND			(HMENU)1011
#define IDC_BUTTON_GRADIENT				(HMENU)1012
#define IDC_BUTTON_CONNECTSERIAL		(HMENU)1013
#define IDC_BUTTON_IGNORESERIAL			(HMENU)1014

#define IDC_COMBO_COLORS				(HMENU)1020

HWND hBarcountSlider;
HWND hZoomSlider;
HWND hwndComboBox;

HWND SettingsDlg = NULL;

float zoom = 0.0001f;
LRESULT colorSel = 0;

int barCount = 500;
BARINFO* bar;

HBRUSH barBrush[255];
unsigned int rgb[3] = { 0,0,0 };


bool border = false;
bool background = true;
bool gradient = true;
bool ignoreSerial = true;

const int bottomBarHeihgt = 30;
const int led_bar = 4;

HANDLE hSettingsFile;

//Creates Settings file for saving settings
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

void setVariable(char* value, int variableNumber)
{
	switch (variableNumber)
	{
	case 0:
		barCount = atoi(value);
		break;

	case 1:
		zoom = (float)atof(value);
		break;

	case 2:
		colorSel = _atoi64(value);
		break;

	case 3:
		border = atoi(value);
		break;

	case 4:
		background = atoi(value);
		break;

	case 5:
		gradient = atoi(value);
		break;
	
	case 6:
		ignoreSerial = atoi(value);
		break;
	}
}

void readSettings()
{
	char nextchar[1];
	DWORD bytes_read = 0;
	LARGE_INTEGER PfileSize;
	GetFileSizeEx(hSettingsFile, &PfileSize);
	INT64 fileSize = PfileSize.QuadPart;
	LARGE_INTEGER move;
	move.LowPart = 0;
	move.HighPart = 0;
	SetFilePointerEx(hSettingsFile, move, NULL, FILE_BEGIN);
	int k = 0;

	for (int i = 0; i < fileSize; i++)
	{
		if (ReadFile(hSettingsFile, &nextchar, 1, &bytes_read, NULL))
		{
			if (nextchar[0] == ';')
			{
				char value[10];
				for (int j = 0; j < 10; j++)
				{
					i++;
					if (ReadFile(hSettingsFile, &nextchar, 1, &bytes_read, NULL))
					{
						if (nextchar[0] != ';')
						{
							value[j] = nextchar[0];
						}
						else
						{
							break;
						}
					}
				}
				setVariable(value, k);
				k++;
			}
		}
	}

	for (int i = 0; i < 255; i++)
	{
		setColor(i, rgb);
		barBrush[i] = CreateSolidBrush(RGB(rgb[0], rgb[1], rgb[2]));
	}
}

void writeSettings()
{
	LARGE_INTEGER move;
	move.LowPart = 0;
	move.HighPart = 0;
	SetFilePointerEx(hSettingsFile, move, NULL, FILE_BEGIN);

	char str[1000];
	float floatingpoint = 0.1f;
	sprintf_s(str, 1000, ";%d;;%0.6ff;;%I64d;;%d;;%d;;%d;;&d;;%d", barCount, zoom, colorSel, border, background, gradient, ignoreSerial);
	WriteFile(hSettingsFile, str, 1000, NULL, NULL);
}

void setColor(unsigned int floatBetweenZeroAndOne, int* rgb)
{
	switch (colorSel)
	{
	case 0:
		rgb[0] = (unsigned int)(plasma[(unsigned int)(floatBetweenZeroAndOne)][0] * 255);
		rgb[1] = (unsigned int)(plasma[(unsigned int)(floatBetweenZeroAndOne)][1] * 255);
		rgb[2] = (unsigned int)(plasma[(unsigned int)(floatBetweenZeroAndOne)][2] * 255);
		break;
	case 1:
		rgb[0] = (unsigned int)(magma[(unsigned int)(floatBetweenZeroAndOne)][0] * 255);
		rgb[1] = (unsigned int)(magma[(unsigned int)(floatBetweenZeroAndOne)][1] * 255);
		rgb[2] = (unsigned int)(magma[(unsigned int)(floatBetweenZeroAndOne)][2] * 255);
		break;
	case 2:
		rgb[0] = (unsigned int)(inferno[(unsigned int)(floatBetweenZeroAndOne)][0] * 255);
		rgb[1] = (unsigned int)(inferno[(unsigned int)(floatBetweenZeroAndOne)][1] * 255);
		rgb[2] = (unsigned int)(inferno[(unsigned int)(floatBetweenZeroAndOne)][2] * 255);
		break;
	case 3:
		rgb[0] = (unsigned int)(viridis[(unsigned int)(floatBetweenZeroAndOne)][0] * 255);
		rgb[1] = (unsigned int)(viridis[(unsigned int)(floatBetweenZeroAndOne)][1] * 255);
		rgb[2] = (unsigned int)(viridis[(unsigned int)(floatBetweenZeroAndOne)][2] * 255);
		break;
	case 4:
		rgb[0] = (unsigned int)(cividis[(unsigned int)(floatBetweenZeroAndOne)][0] * 255);
		rgb[1] = (unsigned int)(cividis[(unsigned int)(floatBetweenZeroAndOne)][1] * 255);
		rgb[2] = (unsigned int)(cividis[(unsigned int)(floatBetweenZeroAndOne)][2] * 255);
		break;
	case 5:
		rgb[0] = (unsigned int)(twilight[(unsigned int)(floatBetweenZeroAndOne * 2)][0] * 255);
		rgb[1] = (unsigned int)(twilight[(unsigned int)(floatBetweenZeroAndOne * 2)][1] * 255);
		rgb[2] = (unsigned int)(twilight[(unsigned int)(floatBetweenZeroAndOne * 2)][2] * 255);
		break;
	case 6:
		rgb[0] = (unsigned int)(turbo[(unsigned int)(floatBetweenZeroAndOne)][0] * 255);
		rgb[1] = (unsigned int)(turbo[(unsigned int)(floatBetweenZeroAndOne)][1] * 255);
		rgb[2] = (unsigned int)(turbo[(unsigned int)(floatBetweenZeroAndOne)][2] * 255);
		break;

	}
}

BOOL CALLBACK SettingsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		//Create Controls

		//Controls for Number of Bars
		HWND hLeftLabel = CreateWindowW(L"Static", L"100",
			WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, IDC_LEFT_BARCOUNT_LABEL, NULL, NULL);

		HWND hRightLabel = CreateWindowW(L"Static", L"500",
			WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, IDC_RIGHT_BARCOUNT_LABEL, NULL, NULL);

		HWND EditBarsDesc = CreateWindowW(L"Static", L"Set Number of Bars:",
			WS_CHILD | WS_VISIBLE, 400, 20, 150, 15, hwnd, IDC_SET_BARCOUNT_DESCRIBTION, NULL, NULL);

		//Controls for Settings
		HWND ButtonBorder = CreateWindowW(L"Button", L"Border",
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 140, 150, 15, hwnd, IDC_BUTTON_BORDER, NULL, NULL);

		if (border)
			SendMessageW(ButtonBorder, BM_SETCHECK, BST_CHECKED, 0);

		HWND ButtonBackground = CreateWindowW(L"Button", L"3D Background",
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 160, 150, 15, hwnd, IDC_BUTTON_BACKGROUND, NULL, NULL);

		if(background)
			SendMessageW(ButtonBackground, BM_SETCHECK, BST_CHECKED, 0);

		HWND ButtonGradient = CreateWindowW(L"Button", L"Color Gradient",
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 180, 150, 15, hwnd, IDC_BUTTON_GRADIENT, NULL, NULL);

		if(gradient)
			SendMessageW(ButtonGradient, BM_SETCHECK, BST_CHECKED, 0);

		HWND ButtonConnectSerial = CreateWindowW(L"Button", L"Connect Serial",
			WS_CHILD | WS_VISIBLE, 50, 250, 100, 20, hwnd, IDC_BUTTON_CONNECTSERIAL, NULL, NULL);

		HWND ButtonIgnoreSerial = CreateWindowW(L"BUTTON", L"Ignore Serial",
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 280, 100, 20, hwnd, IDC_BUTTON_IGNORESERIAL, NULL, NULL);

		if (ignoreSerial)
			SendMessageW(ButtonIgnoreSerial, BM_SETCHECK, BST_CHECKED, 0);

		WCHAR strBarCount[4];
		wsprintfW(strBarCount, L"%d", barCount);
		HWND EditBars = CreateWindowW(L"Edit", strBarCount,
			WS_CHILD | WS_VISIBLE, 535, 20, 30, 15, hwnd, IDC_EDIT_BARCOUNT, NULL, NULL);


		//Creates Slider Controls
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icex);

		hBarcountSlider = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
			WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
			50, 20, 300, 30, hwnd, IDC_SLIDER_BARCOUNT, NULL, NULL);

		hZoomSlider = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
			WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
			50, 60, 300, 30, hwnd, IDC_SLIDER_ZOOM, NULL, NULL);

		SendMessageW(hBarcountSlider, TBM_SETRANGE, TRUE, MAKELONG(100, 1000));
		SendMessageW(hBarcountSlider, TBM_SETPAGESIZE, 0, 10);
		SendMessageW(hBarcountSlider, TBM_SETTICFREQ, 10, 0);
		SendMessageW(hBarcountSlider, TBM_SETPOS, FALSE, barCount);
		SendMessageW(hBarcountSlider, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabel);
		SendMessageW(hBarcountSlider, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabel);

		SendMessageW(hZoomSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
		SendMessageW(hZoomSlider, TBM_SETPAGESIZE, 0, 10);
		SendMessageW(hZoomSlider, TBM_SETTICFREQ, 10, 0);
		SendMessageW(hZoomSlider, TBM_SETPOS, TRUE, (int)(zoom * 1000000));

		hwndComboBox = CreateWindow(WC_COMBOBOX, TEXT(""),
			CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
			50, 100, 150, 150, hwnd, IDC_COMBO_COLORS, NULL,
			NULL);

		TCHAR Planets[7][10] =
		{
			TEXT("Plasma"), TEXT("Magma"), TEXT("Inferno"), TEXT("Viridis"),
			TEXT("Cividis"), TEXT("Twilight"), TEXT("Turbo")
		};

		TCHAR A[16];
		int  k = 0;

		memset(&A, 0, sizeof(A));
		for (k = 0; k <= 6; k += 1)
		{
			wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Planets[k]);

			// Add string to combobox.
			SendMessage(hwndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
		}

		// Send the CB_SETCURSEL message to display an initial item 
		//  in the selection field  
		SendMessage(hwndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_BARCOUNT:
		{
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
			{
				//Gets text from manual input of bars in Edit box
				//HWND hEdit = GetDlgItem(hwnd, (int)IDC_EDIT_BARCOUNT);
				HWND hEdit = GetDlgItem(hwnd, 1008);
				DWORD dwTextLength = GetWindowTextLengthA(hEdit);
				if (dwTextLength > 0)
				{
					LPSTR editText;
					DWORD dwBufferSize = dwTextLength + 1;

					editText = (LPSTR)GlobalAlloc(GPTR, dwBufferSize);
					if (editText)
					{
						if (GetWindowTextA(hEdit, editText, dwBufferSize));
						{
							int editbarcount;
							if (StrToIntExA(editText, STIF_DEFAULT, &editbarcount))
							{
								if (editbarcount <= 500 && editbarcount >= 100)
								{
									SendMessageW(hBarcountSlider, TBM_SETPOS, TRUE, editbarcount);
									barCount = editbarcount;
									BARINFO* tmp = (BARINFO*)realloc(bar, barCount * sizeof(BARINFO));
									if (tmp)
									{
										bar = tmp;
										SendMessageW(globalhwnd, WM_SIZE, 0, 0);
									}
									else
									{
										MessageBoxA(hwnd, "Failed to allocate memory for bar", "Warning", MB_OK);
										SendMessageW(globalhwnd, WM_DESTROY, 0, 0);
									}
								}
							}
						}
					}
					GlobalFree(editText);
				}
			}
			break;
			}
		}
		break;
		case IDC_BUTTON_CONNECTSERIAL:
		{
			doSerial = InitializeSerial(hwnd);
			break;
		}
		case IDC_BUTTON_IGNORESERIAL:
		{
			ignoreSerial = !ignoreSerial;
			if (!ignoreSerial)
				doSerial = InitializeSerial(hwnd);
			break;
		}
		case IDOK:
			writeSettings();
			DestroyWindow(SettingsDlg);
			break;
		case IDCANCEL:
			readSettings();
			BARINFO* tmp = (BARINFO*)realloc(bar, barCount * sizeof(BARINFO));
			if (tmp)
			{
				bar = tmp;
				SendMessageW(globalhwnd, WM_SIZE, 0, 0);
			}
			else
			{
				MessageBoxA(hwnd, "Failed to allocate memory for bar", "Warning", MB_OK);
				SendMessageW(globalhwnd, WM_DESTROY, 0, 0);
			}
			DestroyWindow(SettingsDlg);
			break;
		case IDC_BUTTON_BORDER:
			border = !border;
			break;
		case IDC_BUTTON_BACKGROUND:
			background = !background;
			break;
		case IDC_BUTTON_GRADIENT:
			gradient = !gradient;
			break;
		case IDC_COMBO_COLORS:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				colorSel = SendMessageW((HWND)lParam, (UINT)CB_GETCURSEL,
					(WPARAM)0, (LPARAM)0);

				DeleteObject(barBrush);

				for (int i = 0; i < 255; i++)
				{
					setColor(i, &rgb);
					barBrush[i] = CreateSolidBrush(RGB(rgb[0], rgb[1], rgb[2]));
				}
			}
			break;
			}
		}
		break;
	case WM_HSCROLL:
	{
		LRESULT posBarcount = SendMessageW(hBarcountSlider, TBM_GETPOS, 0, 0);
		barCount = posBarcount;
		BARINFO* tmp = (BARINFO*)realloc(bar, barCount * sizeof(BARINFO));
		if (tmp)
		{
			bar = tmp;
			SendMessageW(globalhwnd, WM_SIZE, 0, 0);
		}
		else
		{
			MessageBoxA(hwnd, "Failed to allocate memory for bar", "Warning", MB_OK);
			SendMessageW(globalhwnd, WM_DESTROY, 0, 0);
		}
		WCHAR strBarCount[5];
		wsprintfW(strBarCount, L"%d", barCount);
		SetDlgItemTextW(hwnd, IDC_EDIT_BARCOUNT, strBarCount);

		LRESULT posZoom = SendMessageW(hZoomSlider, TBM_GETPOS, 0, 0);
		zoom = (float)posZoom * 0.000001f;
	}
	break;
	default:
		return FALSE;
	}
	return TRUE;
}