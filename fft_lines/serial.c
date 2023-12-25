#include <windows.h>
#include "serial.h"

HANDLE hSerial;
DCB dcbSerialParams = { 0 };
COMMTIMEOUTS timeouts = { 0 };

bool doSerial;

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

int CloseSerial(HWND hwnd)
{
	if (CloseHandle(hSerial) == 0)
	{
		MessageBox(hwnd, L"Error closing hSerial", L"Error", MB_OK);
		return 0;
	}
	return 1;
}

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