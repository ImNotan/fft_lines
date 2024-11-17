#include <Windows.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <commctrl.h>
#include <stdbool.h>
#include <Shlwapi.h>

#include "variables.h"
#include "resource.h"
#include "styleDialog.h"
#include "global.h"

#include "settingsFile.h"

#define FILE_ERROR_CODE 0x00000002

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

#define CHECK_ERROR(hr) \
                  if(FAILED(hr))  \
                    { PostMessageW(globalhwnd, WM_ERROR, hr, FILE_ERROR_CODE); return hr; }


/*-----------------------------------------------
Define Control IDs
-----------------------------------------------*/
#define IDC_LEFT_BARCOUNT_LABEL			(HMENU)1000
#define IDC_RIGHT_BARCOUNT_LABEL		(HMENU)1001
#define IDC_SLIDER_BARCOUNT				(HMENU)1002
#define IDC_SLIDER_ZOOM					(HMENU)1003

#define IDC_SET_BARCOUNT_DESCRIBTION	(HMENU)1007
#define IDC_EDIT_BARCOUNT				(HMENU)1008

#define IDC_BUTTON_FFT					(HMENU)1010
#define IDC_BUTTON_BACKGROUND			(HMENU)1011
#define IDC_BUTTON_GRADIENT				(HMENU)1012
#define IDC_BUTTON_CIRCLE				(HMENU)1015
#define IDC_BUTTON_WAVEFORM				(HMENU)1016
#define IDC_BUTTON_STEREO				(HMENU)1017
#define IDC_BUTTON_BEATDETECTION		(HMENU)1018

#define IDC_COMBO_COLORS				(HMENU)1020

/*-----------------------------------------------
Internal Handles

	Controls:
		hBarCountSlider
		hZoomSlider
		hwndComboBox
-----------------------------------------------*/
HWND hBarcountSlider;
HWND hZoomSlider;
HWND hwndComboBox;

/*-----------------------------------------------
Public Handles and Variables

	Handle:
		hwndStyleDialog
		(created in fft_lines - WndProc - WM_COMMAND - ID_SETTINGS_STYLE)
-----------------------------------------------*/
HWND hwndStyleDialog = NULL;


/*-----------------------------------------------
Internal function
	CreateControls

	Creates Dialog Controls for changing style settings

	Called in:
	StyleDialogProc - WM_INITDIALOG
-----------------------------------------------*/
HRESULT CreateControls(HWND hwnd)
{
	//Text for sliders
	HWND hLeftLabel = CreateWindowW(L"Static", L"100",
		WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, IDC_LEFT_BARCOUNT_LABEL, NULL, NULL);
	CHECK_NULL(hLeftLabel);

	HWND hRightLabel = CreateWindowW(L"Static", L"500",
		WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, IDC_RIGHT_BARCOUNT_LABEL, NULL, NULL);
	CHECK_NULL(hRightLabel);

	HWND EditBarsDesc = CreateWindowW(L"Static", L"Set Number of Bars:",
		WS_CHILD | WS_VISIBLE, 400, 20, 150, 15, hwnd, IDC_SET_BARCOUNT_DESCRIBTION, NULL, NULL);
	CHECK_NULL(EditBarsDesc);


	//Controls for Settings
	HWND ButtonBorder = CreateWindowW(L"Button", L"FFT",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 140, 150, 15, hwnd, IDC_BUTTON_FFT, NULL, NULL);
	CHECK_NULL(ButtonBorder);

	if (dofft)
		SendMessageW(ButtonBorder, BM_SETCHECK, BST_CHECKED, 0);

	HWND ButtonBackground = CreateWindowW(L"Button", L"3D Background",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 160, 150, 15, hwnd, IDC_BUTTON_BACKGROUND, NULL, NULL);
	CHECK_NULL(ButtonBackground);

	if (background)
		SendMessageW(ButtonBackground, BM_SETCHECK, BST_CHECKED, 0);

	HWND ButtonGradient = CreateWindowW(L"Button", L"Color Gradient",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 180, 150, 15, hwnd, IDC_BUTTON_GRADIENT, NULL, NULL);
	CHECK_NULL(ButtonGradient);

	if (gradient)
		SendMessageW(ButtonGradient, BM_SETCHECK, BST_CHECKED, 0);

	HWND ButtonCircle = CreateWindowW(L"Button", L"Circle effect",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 200, 150, 15, hwnd, IDC_BUTTON_CIRCLE, NULL, NULL);
	CHECK_NULL(ButtonCircle);

	if (circle)
		SendMessageW(ButtonCircle, BM_SETCHECK, BST_CHECKED, 0);

	HWND ButtonWaveform = CreateWindowW(L"Button", L"Waveform",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 220, 150, 15, hwnd, IDC_BUTTON_WAVEFORM, NULL, NULL);
	CHECK_NULL(ButtonWaveform);

	if (waveform)
		SendMessageW(ButtonWaveform, BM_SETCHECK, BST_CHECKED, 0);

	HWND ButtonStereo = CreateWindowW(L"Button", L"Stereo",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 240, 150, 15, hwnd, IDC_BUTTON_STEREO, NULL, NULL);
	CHECK_NULL(ButtonStereo);

	if (stereo)
		SendMessageW(ButtonStereo, BM_SETCHECK, BST_CHECKED, 0);

	HWND ButtonBeatDetection = CreateWindowW(L"Button", L"Beat Detection",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 260, 150, 15, hwnd, IDC_BUTTON_BEATDETECTION, NULL, NULL);
	CHECK_NULL(ButtonBeatDetection);

	if (beatDetection)
		SendMessageW(ButtonBeatDetection, BM_SETCHECK, BST_CHECKED, 0);


	WCHAR strBarCount[5];
	wsprintfW(strBarCount, L"%d", barCount);
	HWND EditBars = CreateWindowW(L"Edit", strBarCount,
		WS_CHILD | WS_VISIBLE, 535, 20, 30, 15, hwnd, IDC_EDIT_BARCOUNT, NULL, NULL);
	CHECK_NULL(EditBars);


	//Creates Slider Controls
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	hBarcountSlider = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
		WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
		50, 20, 300, 30, hwnd, IDC_SLIDER_BARCOUNT, NULL, NULL);
	CHECK_NULL(hBarcountSlider);

	hZoomSlider = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
		WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
		50, 60, 300, 30, hwnd, IDC_SLIDER_ZOOM, NULL, NULL);
	CHECK_NULL(hZoomSlider);

	SendMessageW(hBarcountSlider, TBM_SETRANGE, TRUE, MAKELONG(MIN_BARCOUNT, MAX_BARCOUNT));
	SendMessageW(hBarcountSlider, TBM_SETPAGESIZE, 0, 10);
	SendMessageW(hBarcountSlider, TBM_SETTICFREQ, 10, 0);
	SendMessageW(hBarcountSlider, TBM_SETPOS, FALSE, barCount);
	SendMessageW(hBarcountSlider, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabel);
	SendMessageW(hBarcountSlider, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabel);

	SendMessageW(hZoomSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
	SendMessageW(hZoomSlider, TBM_SETPAGESIZE, 0, 10);
	SendMessageW(hZoomSlider, TBM_SETTICFREQ, 10, 0);
	SendMessageW(hZoomSlider, TBM_SETPOS, TRUE, (int)(zoom * 100000));

	hwndComboBox = CreateWindow(WC_COMBOBOX, TEXT(""),
		CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
		50, 100, 150, 150, hwnd, IDC_COMBO_COLORS, NULL,
		NULL);
	CHECK_NULL(hwndComboBox);

	TCHAR Planets[6][10] =
	{
		TEXT("Plasma"), TEXT("Magma"), TEXT("Inferno"), TEXT("Viridis"),
		TEXT("Cividis"), TEXT("Turbo")
	};

	TCHAR A[16];
	int  k = 0;

	memset(&A, 0, sizeof(A));
	for (k = 0; k < 6; k += 1)
	{
		wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Planets[k]);

		// Add string to combobox.
		SendMessageW(hwndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
	}

	// Send the CB_SETCURSEL message to display an initial item 
	//  in the selection field  
	SendMessageW(hwndComboBox, CB_SETCURSEL, (WPARAM)colorSel, (LPARAM)0);
}

/*-----------------------------------------------
	Process for Style Dialog

	Handled messages:
		WM_INITDIALOG
		WM_COMMAND
			IDOK
			IDCANCEL
			IDC_BUTTON_CONNECTSERIAL
			IDC_BUTTON_IGNORESERIAL
			IDC_BUTTON_FFT
			IDC_BUTTON_BACKGROUND
			IDC_BUTTON_GRADIENT
			IDC_BUTTON_CIRCLE
			IDC_BUTTON_WAVEFORM
			IDC_BUTTON_STEREO
			IDC_BUTTON_BEATDETECTION
			IDC_EDIT_BARCOUNT
				EN_CHANGE
			IDC_COMBO_COLORS
				CBN_SELCHANGE
		WM_HSCROLL
-----------------------------------------------*/
LRESULT CALLBACK StyleDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		/*-----------------------------------------------
			WM_INITDIALOG

			Creates all setting Controls
		-----------------------------------------------*/
		case WM_INITDIALOG:
		{
			CreateControls(hwnd);
		}
		break;
		/*-----------------------------------------------
			WM_COMMAND

			Handles messages from control items
		-----------------------------------------------*/
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				/*-----------------------------------------------
					IDOK

					saves changes
					closes window
				-----------------------------------------------*/
				case IDOK:
				{
					writeSettings();
					DestroyWindow(hwndStyleDialog);
				}
				break;

				/*-----------------------------------------------
					IDCANCEL

					resets settings according to settings File
					closes window
				-----------------------------------------------*/
				case IDCANCEL:
				{
					HRESULT hr = S_OK;
					hr = readSettings();
					CHECK_ERROR(hr);
					DestroyWindow(hwndStyleDialog);
				}
				break;

				/*-----------------------------------------------
					IDC_BUTTON_FFT
					IDC_BUTTON_BACKGROUND
					IDC_BUTTON_GRADIENT
					IDC_BUTTON_CIRCLE
					IDC_BUTTON_WAVEFORM
					IDC_BUTTON_STEREO
					IDC_BUTTON_BEATDETECTION

					change variable determening the drawing style
				-----------------------------------------------*/
				case IDC_BUTTON_FFT:
					dofft = !dofft;
					break;
				case IDC_BUTTON_BACKGROUND:
					background = !background;
					break;
				case IDC_BUTTON_GRADIENT:
					gradient = !gradient;
					break;
				case IDC_BUTTON_CIRCLE:
					circle = !circle;
					break;
				case IDC_BUTTON_WAVEFORM:
				{
					waveform = !waveform;
					if (waveform)
					{
						BARINFO* tmp = (BARINFO*)realloc(waveBar, N * sizeof(BARINFO));
						CHECK_NULL(tmp);
						waveBar = tmp;
						ResizeBars(globalhwnd, waveBar, N, 0, 0);
					}
					else
					{
						free(waveBar);
						waveBar = NULL;
					}
				}
				break;
				case IDC_BUTTON_STEREO:
				{
					stereo = !stereo;
					if (stereo)
					{
						ResizeBars(globalhwnd, barLeft, barCount, 0, stereo);
						INT16* tmpint16 = (INT16*)realloc(audioBufferRight, N * sizeof(INT16));
						CHECK_NULL(tmpint16);
						audioBufferRight = tmpint16;

						BARINFO* tmpbarinfo = (BARINFO*)realloc(barRight, barCount * sizeof(BARINFO));
						CHECK_NULL(tmpbarinfo);
						barRight = tmpbarinfo;
						ResizeBars(globalhwnd, barRight, barCount, 1, stereo);
					}
					else
					{
						ResizeBars(globalhwnd, barLeft, barCount, 0, stereo);
						free(audioBufferRight);
						free(barRight);
						audioBufferRight = NULL;
						barRight = NULL;
					}
				}
				break;
				case IDC_BUTTON_BEATDETECTION:
				{
					beatDetection = !beatDetection;
					if (beatDetection)
					{
						int* tmp = (int*)realloc(bassBeatBuffer, N * sizeof(int));
						CHECK_NULL(tmp);
						bassBeatBuffer = tmp;
						for (int i = 0; i < N; i++)
						{
							bassBeatBuffer[i] = 0;
						}

						BARINFO* bartmp = (BARINFO*)realloc(beatBar, N * sizeof(BARINFO));
						CHECK_NULL(bartmp);
						beatBar = bartmp;
						ResizeBars(globalhwnd, beatBar, N, 0, 0);
					}
					else
					{
						free(waveBar);
						waveBar = NULL;
						free(bassBeatBuffer);
						bassBeatBuffer = NULL;
					}
				}
				break;

				/*-----------------------------------------------
					IDC_EDIT_BARCOUNT

					if EN_CHANGE:

					gets text from edit control
					changes barcount accordingly
				-----------------------------------------------*/
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
							if (dwTextLength == 0)
								break;

							LPSTR editText;
							DWORD dwBufferSize = dwTextLength + 1;

							editText = (LPSTR)GlobalAlloc(GPTR, dwBufferSize);
							if (editText == NULL)
								break;

							if (GetWindowTextA(hEdit, editText, dwBufferSize) == 0)
							{
								GlobalFree(editText);
								break;
							}

							int editBarCount;
							if (StrToIntExA(editText, STIF_DEFAULT, &editBarCount) == 0)
							{
								GlobalFree(editText);
								break;
							}

							GlobalFree(editText);

							if (editBarCount < MIN_BARCOUNT || editBarCount > MAX_BARCOUNT)
								break;

							SendMessageW(hBarcountSlider, TBM_SETPOS, TRUE, editBarCount);

							barCount = editBarCount;

							BARINFO* tmp = (BARINFO*)realloc(barLeft, barCount * sizeof(BARINFO));
							CHECK_NULL(tmp);
							barLeft = tmp;
							ResizeBars(globalhwnd, barLeft, barCount, 0, stereo);

							if (stereo)
							{
								BARINFO* tmp = (BARINFO*)realloc(barRight, barCount * sizeof(BARINFO));
								CHECK_NULL(tmp);
								barRight = tmp;
								ResizeBars(globalhwnd, barRight, barCount, 1, stereo);
							}
							redrawAll = true;
						}
						break;
					}
				}
				break;

				/*-----------------------------------------------
				IDC_COMBO_COLOR

				if CBN_SELCHANGE:

				changes selected Color
				-----------------------------------------------*/
				case IDC_COMBO_COLORS:
				{
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE:
						{
							HRESULT hr = S_OK;
							colorSel = SendMessageW((HWND)lParam, (UINT)CB_GETCURSEL,
								(WPARAM)0, (LPARAM)0);

							setColor();
							hr = ChangeBarBrush();
							CHECK_ERROR(hr);
						}
						break;
					}
				}
				break;
			}
		}
		break;
		/*-----------------------------------------------
			WM_HSCROLL

			called when sliders move

			changes barcount or zoom
		-----------------------------------------------*/
		case WM_HSCROLL:
		{
			LRESULT posBarcount = SendMessageW(hBarcountSlider, TBM_GETPOS, 0, 0);
			barCount = posBarcount;

			BARINFO* tmp = (BARINFO*)realloc(barLeft, barCount * sizeof(BARINFO));
			CHECK_NULL(tmp);
			barLeft = tmp;
			ResizeBars(globalhwnd, barLeft, barCount, 0, stereo);

			if (stereo)
			{
				BARINFO* tmp = (BARINFO*)realloc(barRight, barCount * sizeof(BARINFO));
				CHECK_NULL(tmp);
				barRight = tmp;
				ResizeBars(globalhwnd, barRight, barCount, 1, stereo);
			}
			redrawAll = true;

			WCHAR strBarCount[5];
			wsprintfW(strBarCount, L"%d", barCount);
			SetDlgItemTextW(hwnd, IDC_EDIT_BARCOUNT, strBarCount);

			LRESULT posZoom = SendMessageW(hZoomSlider, TBM_GETPOS, 0, 0);
			zoom = (float)posZoom * 0.00001f;
		}
		break;
		default:
			return FALSE;
	}
	return 0;
}