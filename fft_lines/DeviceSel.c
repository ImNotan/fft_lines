#include <Windows.h>
#include <stdbool.h>

#include "DeviceSel.h"
#include "resource.h"

//Initialize cpp audio functions
HRESULT getAudioDeviceNames(unsigned int deviceNumber, wchar_t* deviceName);
HRESULT getAudioDeviceCount(unsigned int* count);
HRESULT startRecording();
HRESULT stopRecording();
HRESULT initializeRecording();
HRESULT ChangeAudioStream(unsigned int deviceNumber);

HWND DeviceSelDlg = NULL;

LRESULT CALLBACK DeviceSelProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		//Looks for all audio devices
		int count;
		HRESULT hr;
		hr = getAudioDeviceCount(&count);
		ShowError(hr, hwnd);
		if (count != 0)
		{
			for (int i = 0; i < count; i++)
			{
				wchar_t deviceName[100];
				hr = getAudioDeviceNames(i, &deviceName);
				ShowError(hr, hwnd);
				SendMessageW(GetDlgItem(hwnd, IDC_LIST1), LB_ADDSTRING, 0, deviceName);
			}
		}

	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
			DestroyWindow(DeviceSelDlg);
			break;

		case IDCANCEL:
			DestroyWindow(DeviceSelDlg);
			break;
		
		case IDC_BUTTON_CHANGE:
		{
			//Change audio device
			stopRecording();
			unsigned int deviceSelected = SendMessageW(GetDlgItem(hwnd, IDC_LIST1), LB_GETCURSEL, 0, 0);
			ChangeAudioStream(deviceSelected);
			startRecording();
			break;
		}
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}