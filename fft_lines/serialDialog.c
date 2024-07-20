#include <windows.h>
#include "serialDialog.h"
#include "global.h"
#include "settingsFile.h"

#define FILE_ERROR_CODE 0x00000010

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

#define CHECK_ERROR(hr) \
                  if(FAILED(hr))  \
                    { PostMessageW(globalhwnd, WM_ERROR, hr, FILE_ERROR_CODE); return hr; }


/*-----------------------------------------------
Define Control IDs
-----------------------------------------------*/
#define IDC_BUTTON_CONNECTSERIAL		(HMENU)1000
#define IDC_BUTTON_IGNORESERIAL			(HMENU)1001

/*-----------------------------------------------
Internal Handles and Variables

	Handle:
		hSerial

	Variables:
		dcbSerialParams
		timeouts
-----------------------------------------------*/
HANDLE hSerial;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };

/*-----------------------------------------------
Public Handles and Variables

	Handle:
		hwndSerialDialog

	Variables:
		doSerial
-----------------------------------------------*/
HWND hwndSerialDialog = NULL;

bool doSerial;

/*-----------------------------------------------
Public functions

	Initialization:
		InitializeSerial
		CloseSerial

	Endpoint:
		WriteSerial

	Process:
		SerialDialogProc
-----------------------------------------------*/
int InitializeSerial(HWND hwnd);
int CloseSerial(HWND hwnd);

int WriteSerial(char byte_to_send[1], HWND hwnd);

LRESULT CALLBACK SerialDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);


/*-----------------------------------------------
	Opens serial port

	Called in:
	fft_lines - WndProc - WM_CREATE
	serialDialog - SerialDialogProc - WM_COMMAND - IDC_BUTTON_CONNECTSERIAL
	serialDialog - SerialDialogProc - WM_COMMAND - IDC_BUTTON_IGNORESERIAL
-----------------------------------------------*/
int InitializeSerial(HWND hwnd)
{
	hSerial = CreateFile(TEXT("COM3"), GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		MessageBox(hwnd, L"Error opening Serial port", L"Error", MB_OK);
		return 0;
	}

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (GetCommState(hSerial, &dcbSerialParams) == 0)
	{
		MessageBox(hwnd, L"Error getting device state", L"Error", MB_OK);
		CloseHandle(hSerial);
		return 0;
	}

	//sets Serial settings make sure to use 19200 baudRate on arduino
	dcbSerialParams.BaudRate = CBR_19200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (SetCommState(hSerial, &dcbSerialParams) == 0)
	{
		MessageBox(hwnd, L"Error setting device parameters", L"Error", MB_OK);
		CloseHandle(hSerial);
		return 0;
	}

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (SetCommTimeouts(hSerial, &timeouts) == 0)
	{
		MessageBox(hwnd, L"Error setting timeouts", L"Error", MB_OK);
		CloseHandle(hSerial);
		return 0;
	}
	return 1;
}

/*-----------------------------------------------
	Closes serial port

	Called in:
	fft_lines - WndProc - WM_DESTROY
	serialDialog - SerialDialogProc - WM_COMMAND - IDC_BUTTON_IGNORESERIAL
-----------------------------------------------*/
int CloseSerial(HWND hwnd)
{
	if (CloseHandle(hSerial) == 0)
	{
		MessageBox(hwnd, L"Error closing hSerial", L"Error", MB_OK);
		return 0;
	}
	return 1;
}

/*-----------------------------------------------
	Writes to serial port

	Called in:
	fft_lines - WndProc - WM_TIMER
-----------------------------------------------*/
int WriteSerial(char byte_to_send[1], HWND hwnd)
{
	DWORD bytes_written = 0;
	if (!WriteFile(hSerial, byte_to_send, 1, &bytes_written, NULL))
	{
		doSerial = false;
		MessageBox(hwnd, L"Error Writing to Serial", L"Error", MB_OK);
		return 0;
	}
	return 1;
}

/*-----------------------------------------------
	Process for Serial Dialog

	Handled messages:
		WM_INITDIALOG
		WM_COMMAND
			IDOK
			IDCANCEL
			IDCBUTTON_CONNECTSERIAL
			IDC_BUTTON_IGNORESERIAL
-----------------------------------------------*/
LRESULT CALLBACK SerialDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		/*-----------------------------------------------
			WM_INITDIALOG

			Creates all setting Controls
		-----------------------------------------------*/
		case WM_INITDIALOG:
		{
			HWND ButtonConnectSerial = CreateWindowW(L"Button", L"Connect Serial",
				WS_CHILD | WS_VISIBLE, 50, 10, 100, 20, hwnd, IDC_BUTTON_CONNECTSERIAL, NULL, NULL);
			CHECK_NULL(ButtonConnectSerial);

			HWND ButtonIgnoreSerial = CreateWindowW(L"BUTTON", L"Ignore Serial",
				WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 50, 40, 100, 20, hwnd, IDC_BUTTON_IGNORESERIAL, NULL, NULL);
			CHECK_NULL(ButtonIgnoreSerial);

			if (ignoreSerial)
				SendMessageW(ButtonIgnoreSerial, BM_SETCHECK, BST_CHECKED, 0);
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
					IDCANCEL

					close the window no difference right now
				-----------------------------------------------*/
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndSerialDialog);
					break;

				/*-----------------------------------------------
					IDC_BUTTON_CONNECTSERIAL

					tries to establish a serial connection
				-----------------------------------------------*/
				case IDC_BUTTON_CONNECTSERIAL:
				{
					doSerial = InitializeSerial(hwnd);
				}
				break;

				/*-----------------------------------------------
					IDC_BUTTON_IGNORESERIAL

					changes variable if it tries to connect to serial on startup
				-----------------------------------------------*/
				case IDC_BUTTON_IGNORESERIAL:
				{
					ignoreSerial = !ignoreSerial;
					if (ignoreSerial)
						doSerial = !CloseSerial(hwnd);
					if (!ignoreSerial)
						doSerial = InitializeSerial(hwnd);
				}
				break;
			}
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}