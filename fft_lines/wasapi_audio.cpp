#include <Windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <time.h>
#include <iostream>
#include <Functiondiscoverykeys_devpkey.h>
#include <strsafe.h>
#include "global.h"

using namespace std;

#pragma comment(lib, "Winmm.lib")

//#define N 1024

BOOL bDone = FALSE;
HMMIO hFile = NULL;

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  500000
#define REFTIMES_PER_MILLISEC  500

#define EXIT_ON_ERROR(hres)  \
                  if (FAILED(hres)) { return hr; }
#define SAFE_RELEASE(punk)  \
                  if ((punk) != NULL)  \
                    { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

HRESULT hr;
REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
REFERENCE_TIME hnsActualDuration;
UINT32 bufferFrameCount;
UINT32 numFramesAvailable;
IMMDeviceEnumerator* pEnumerator = NULL;
IMMDevice* pDevice = NULL;
IAudioClient* pAudioClient = NULL;
IAudioCaptureClient* pCaptureClient = NULL;
WAVEFORMATEX* pwfx = NULL;

BYTE* pData;
DWORD flags;

MMCKINFO ckRIFF = { 0 };
MMCKINFO ckData = { 0 };

IMMDeviceCollection* allDevices;

extern "C" HRESULT initializeRecording();
extern "C" void uninitializeRecording();
extern "C" HRESULT GetAudioBuffer(int16_t* buffer, BOOL* bdone);
extern "C" HRESULT startRecording();
extern "C" HRESULT stopRecording();
extern "C" void Exit();
extern "C" void getWaveFormat(WAVEFORMATEX * waveformat);
extern "C" HRESULT getAudioDeviceCount(unsigned int* count);
extern "C" HRESULT getAudioDeviceNames(unsigned int deviceNumber, wchar_t* deviceName);
extern "C" HRESULT ChangeAudioStream(unsigned int deviceNumber);

HRESULT RecordAudioStream();

HRESULT initializeRecording()
{
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    EXIT_ON_ERROR(hr);
    hr = RecordAudioStream();
    return hr;
}

void uninitializeRecording()
{
    CoUninitialize();
}

int i = 0;

HRESULT getAudioDeviceCount(unsigned int* count)
{
    hr = pEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &allDevices);
    EXIT_ON_ERROR(hr);
    hr = allDevices->GetCount(count);
    return hr;
}

HRESULT getAudioDeviceNames(unsigned int deviceNumber, wchar_t* deviceName)
{
    IMMDevice* OneDevice;
    IPropertyStore* pProps = NULL;

    unsigned int count;

    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &allDevices);
    EXIT_ON_ERROR(hr);
    hr = allDevices->GetCount(&count);
    EXIT_ON_ERROR(hr);
    if (deviceNumber < count)
    {
        hr = allDevices->Item(deviceNumber, &OneDevice);
        EXIT_ON_ERROR(hr);

        hr = OneDevice->OpenPropertyStore(STGM_READ, &pProps);
        EXIT_ON_ERROR(hr);

        PROPVARIANT varName;
        PropVariantInit(&varName);

        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        EXIT_ON_ERROR(hr);

		if (varName.vt != VT_EMPTY)
		{
            wstring mystring_w(varName.pwszVal);
            wstring LPCWSTR_stdstr = mystring_w + L" (output)";
            lstrcpyW(deviceName, LPCWSTR_stdstr.c_str());
		}
    }
    else if (deviceNumber >= count)
    {
        hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &allDevices);
        EXIT_ON_ERROR(hr);
        deviceNumber -= count;

        hr = allDevices->Item(deviceNumber, &OneDevice);
        EXIT_ON_ERROR(hr);

        hr = OneDevice->OpenPropertyStore(STGM_READ, &pProps);
        EXIT_ON_ERROR(hr);

        PROPVARIANT varName;
        PropVariantInit(&varName);

        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        EXIT_ON_ERROR(hr);

        if (varName.vt != VT_EMPTY)
        {
            wstring mystring_w(varName.pwszVal);
            wstring LPCWSTR_stdstr = mystring_w + L" (input)";
            lstrcpyW(deviceName, LPCWSTR_stdstr.c_str());
        }
    }
    return hr;
}

HRESULT ChangeAudioStream(unsigned int deviceNumber)
{
    unsigned int count;

    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &allDevices);
    EXIT_ON_ERROR(hr);
    hr = allDevices->GetCount(&count);
    EXIT_ON_ERROR(hr);
    if (deviceNumber < count)
    {
        hr = allDevices->Item(deviceNumber, &pDevice);
        EXIT_ON_ERROR(hr);

        hr = pDevice->Activate(
            IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient);
        EXIT_ON_ERROR(hr);

        hr = pAudioClient->GetMixFormat(&pwfx);
        EXIT_ON_ERROR(hr);

        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL);
        EXIT_ON_ERROR(hr);

        // Get the size of the allocated buffer.
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
        EXIT_ON_ERROR(hr);

        hr = pAudioClient->GetService(
            IID_IAudioCaptureClient,
            (void**)&pCaptureClient);
        EXIT_ON_ERROR(hr);

        // Calculate the actual duration of the allocated buffer.
        //hnsActualDuration = (double)REFTIMES_PER_SEC *
        //    bufferFrameCount / pwfx->nSamplesPerSec;

    }
    else if (deviceNumber >= count)
    {
        hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &allDevices);
        EXIT_ON_ERROR(hr);
        deviceNumber -= count;

        hr = allDevices->Item(deviceNumber, &pDevice);
        EXIT_ON_ERROR(hr);

        hr = pDevice->Activate(
            IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient);
        EXIT_ON_ERROR(hr);

        hr = pAudioClient->GetMixFormat(&pwfx);
        EXIT_ON_ERROR(hr);

        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            NULL,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL);
        EXIT_ON_ERROR(hr);

        // Get the size of the allocated buffer.
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
        EXIT_ON_ERROR(hr);

        hr = pAudioClient->GetService(
            IID_IAudioCaptureClient,
            (void**)&pCaptureClient);
        EXIT_ON_ERROR(hr);

        // Calculate the actual duration of the allocated buffer.
        //hnsActualDuration = (double)REFTIMES_PER_SEC *
        //    bufferFrameCount / pwfx->nSamplesPerSec;
    }
    return hr;
}

HRESULT RecordAudioStream()
{
	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr);

	hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender, eConsole, &pDevice);
	EXIT_ON_ERROR(hr);

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr);

	hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr);

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
	EXIT_ON_ERROR(hr);

	// Get the size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr);

	hr = pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&pCaptureClient);
    EXIT_ON_ERROR(hr);

	// Calculate the actual duration of the allocated buffer.
	//hnsActualDuration = (double)REFTIMES_PER_SEC *
	//	bufferFrameCount / pwfx->nSamplesPerSec;

	return hr;
}

void Exit()
{
    stopRecording();
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
        SAFE_RELEASE(pDevice)
        SAFE_RELEASE(pAudioClient)
        SAFE_RELEASE(pCaptureClient)
}

HRESULT startRecording()
{
    hr = pAudioClient->Start();  // Start recording.
    return hr;
}

HRESULT stopRecording()
{
    hr = pAudioClient->Stop();  // Stop recording.
    return hr;
}

void getWaveFormat(WAVEFORMATEX* waveformat)
{
    *waveformat = *pwfx;
}

HRESULT MoveArray(int16_t* destination, int addCount, float* source)
{
    for (int k = 0; k < N - addCount; k++)
    {
        destination[k] = destination[k + addCount - 1];
    }
    for (int k = 0; k < addCount; k++)
    {
        destination[k + (N - addCount)] = (int16_t)(source[k] * 65536);
    }
    return S_OK;
}

HRESULT GetAudioBuffer(int16_t* buffer, BOOL* bdone)
{
    UINT packetLength = 0;
    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    EXIT_ON_ERROR(hr);
    while(packetLength != 0)
    {
        // Get the available data in the shared buffer.
        hr = pCaptureClient->GetBuffer(
            &pData,
            &numFramesAvailable,
            &flags, NULL, NULL);
        EXIT_ON_ERROR(hr);

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
        {
            pData = NULL;  // Tell CopyData to write silence.
        }
        else
        {
            LONG lBytesToWrite = numFramesAvailable * pwfx->nBlockAlign;
            float f[N];
            //To explain what the fuck is happening here:
            //GetBuffer fills pData an Byte array with data
            //4 Bytes of pData create an float
            //pData is alternating between left and right channel
            //so to get the float of one data point every 8th pData is iterated and 4 Bytes are coppied to an float array
            //the other 4 Bytes are discarded because we only want one channel
            for (int j = 0; j < lBytesToWrite; j += (pwfx->wBitsPerSample / 4))
            {
                memcpy(&f[i], &pData[j], sizeof(f[i]));
                i++;
            }
            //After that the new data is appended to the previous data which is moved forward to make space for the new data at the end
            MoveArray(buffer, i, f);
            i = 0;
            *bdone = true;
        }

        hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
        EXIT_ON_ERROR(hr);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr);
    }

    return hr;
}