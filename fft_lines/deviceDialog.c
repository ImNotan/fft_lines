#include <Windows.h>
#include <stdbool.h>

#include "deviceDialog.h"
#include "resource.h"
#include "global.h"

#define FILE_ERROR_CODE 0x00000007

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

#define CHECK_ERROR(hr) \
                  if(FAILED(hr))  \
                    { PostMessageW(globalhwnd, WM_ERROR, hr, FILE_ERROR_CODE); return hr; }

/*-----------------------------------------------
Included Functions

	wasapi_audio.cpp:
		Information about Audiostream:
			getAudioDeviceNames
			getAudioDeviceCount

		Audiostream manipulation:
			startRecording
			stopRecording
			ChangeAudioStream
-----------------------------------------------*/

HRESULT getAudioDeviceNames(unsigned int deviceNumber, wchar_t* deviceName);
HRESULT getAudioDeviceCount(unsigned int* count);

HRESULT startRecording();
HRESULT stopRecording();
HRESULT ChangeAudioStream(unsigned int deviceNumber);

/*-----------------------------------------------
	Public Handle for Device Selection Dialog
	created in fft_lines - WndProc - WM_COMMAND - ID_SETTING_DEVICES
-----------------------------------------------*/
HWND hwndDeviceDialog = NULL;



/*-----------------------------------------------
	Process for Device Select Dialog

	Handled messages:
		WM_INITDIALOG
		WM_COMMAND
			IDOK
			IDCANCEL
			IDC_BUTTON_CHANGE
-----------------------------------------------*/
LRESULT CALLBACK DeviceDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		/*-----------------------------------------------
			WM_INITDIALOG 

			retrieves Names of connected Devices
			Adds them to IDC_LIST1
		-----------------------------------------------*/
		case WM_INITDIALOG:
		{
			//Looks for all audio devices
			int count;
			HRESULT hr = S_OK;
			hr = getAudioDeviceCount(&count);

			if (count != 0)
			{
				for (int i = 0; i < count; i++)
				{
					wchar_t deviceName[100];
					hr = getAudioDeviceNames(i, &deviceName);
					CHECK_ERROR(hr);

					SendMessageW(GetDlgItem(hwnd, IDC_LIST_DEVICES), LB_ADDSTRING, 0, deviceName);
				}
			}
		}
		break;

		/*-----------------------------------------------
			WM_COMMAND

			Handles messages from control items

			IDOK
			IDCANCEL
			IDC_BUTTON_CHANGE
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
					DestroyWindow(hwndDeviceDialog);
					break;

				/*-----------------------------------------------
					IDC_BUTTON_CHANGE

					retrieves selected Device
					changes to this device
				-----------------------------------------------*/
				case IDC_BUTTON_CHANGE:
				{
					HRESULT hr = S_OK;

					//Change audio device
					hr = stopRecording();
					CHECK_ERROR(hr);

					unsigned int deviceSelected = SendMessageW(GetDlgItem(hwnd, IDC_LIST_DEVICES), LB_GETCURSEL, 0, 0);
					hr = ChangeAudioStream(deviceSelected);
					CHECK_ERROR(hr);
					hr = startRecording();
					CHECK_ERROR(hr);
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