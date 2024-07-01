#include <Windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <time.h>
#include <iostream>
#include <Functiondiscoverykeys_devpkey.h>
#include <strsafe.h>
#include "global.h"

#include "wasapi_audio.h"

using namespace std;

#pragma comment(lib, "Winmm.lib")

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  500000

#define EXIT_ON_ERROR(hres)  \
                  if (FAILED(hres)) { return hr; }

#define SAFE_RELEASE(punk)  \
                  if ((punk) != NULL)  \
                    { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
REFERENCE_TIME hnsActualDuration;
UINT32 bufferFrameCount;

IMMDeviceEnumerator* pEnumerator = NULL;
IMMDevice* pDevice = NULL;
IAudioClient* pAudioClient = NULL;
IAudioCaptureClient* pCaptureClient = NULL;
WAVEFORMATEX* pwfx = NULL;

/*-----------------------------------------------
Public functions

    Initialization:
        initializeRecording
        uninitializeRecording
        startRecording
        stopRecording

    Get Endpoint Information:
        getAudioDeviceCount
        getAudioDeviceNames
        ChangeAudioStream
        getWaveFormat

    Retrieve Audio Buffer:
        GetAudioBuffer
-----------------------------------------------*/

extern "C" HRESULT  initializeRecording();
extern "C" void     uninitializeRecording();
extern "C" HRESULT  startRecording();
extern "C" HRESULT  stopRecording();

extern "C" HRESULT  getAudioDeviceCount(unsigned int* count);
extern "C" HRESULT  getAudioDeviceNames(unsigned int deviceNumber, wchar_t* deviceName);
extern "C" HRESULT  ChangeAudioStream(unsigned int deviceNumber);
void                getWaveFormat(WAVEFORMATEX * waveformat);

extern "C" HRESULT  GetAudioBuffer(int16_t * buffer);

/*-----------------------------------------------
Internal functions

    Initialization:
        InitializeAudioStream

    Data manipulation:
        MoveArray
-----------------------------------------------*/

HRESULT InitializeAudioStream();
HRESULT MoveArray(int16_t * destination, int addCount, float* source);



/*-----------------------------------------------
    Initialization of wasapi_audio

    Called in:
    fft_lines - WndProc - WM_CREATE
-----------------------------------------------*/
HRESULT initializeRecording()
{
    HRESULT hr = S_OK;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    EXIT_ON_ERROR(hr);
    hr = InitializeAudioStream();
    EXIT_ON_ERROR(hr);
    startRecording();
    return hr;
}

/*-----------------------------------------------
    Uninitialization of wasapi_audio

    Called in:
    fft_lines - WndProc - WM_DESTROY
-----------------------------------------------*/
void uninitializeRecording()
{
    CoUninitialize();
    stopRecording();
}

/*-----------------------------------------------
    Starts Audio Recording

    Called in:
    wasapi_audio - initializeRecording
    DeviceSel - DeviceSelProc - IDC_BUTTON_CHANGE
-----------------------------------------------*/
HRESULT startRecording()
{
    HRESULT hr = S_OK;
    hr = pAudioClient->Start();
    return hr;
}

/*-----------------------------------------------
    Stops Audio Recording

    Called in:
    wasapi_audio - uninitializeRecording
    DeviceSel - DeviceSelProc - IDC_BUTTON_CHANGE
-----------------------------------------------*/
HRESULT stopRecording()
{
    HRESULT hr = S_OK;
    hr = pAudioClient->Stop();
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
        SAFE_RELEASE(pDevice)
        SAFE_RELEASE(pAudioClient)
        SAFE_RELEASE(pCaptureClient)
        return hr;
}

/*-----------------------------------------------
    Initializes Audio Stream

    Called in:
    wasapi_audio - initializeRecording
-----------------------------------------------*/
HRESULT InitializeAudioStream()
{
    HRESULT hr = S_OK;
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

/*-----------------------------------------------
    Retrieves Number of Connected Audio Devices

    Called in:
    DeviceSel - DeviceSelProc - WM_INITDIALOG
-----------------------------------------------*/
HRESULT getAudioDeviceCount(unsigned int* count)
{
    HRESULT hr = S_OK;
    IMMDeviceCollection* allDevices;

    hr = pEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &allDevices);
    EXIT_ON_ERROR(hr);
    hr = allDevices->GetCount(count);
    return hr;
}

/*-----------------------------------------------
    Retrieves Names of Connected Audio Devices

    Called in:
    DeviceSel - DeviceSelProc - WM_INITDIALOG
-----------------------------------------------*/
HRESULT getAudioDeviceNames(unsigned int deviceNumber, wchar_t* deviceName)
{
    HRESULT hr = S_OK;

    IMMDeviceCollection* allDevices;
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

/*-----------------------------------------------
    Changes Audio Device being recorded

    Called in:
    DeviceSel - DeviceSelProc - IDC_BUTTON_CHANGE
-----------------------------------------------*/
HRESULT ChangeAudioStream(unsigned int deviceNumber)
{
    HRESULT hr = S_OK;
    unsigned int count;

    IMMDeviceCollection* allDevices;

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
    }
    return hr;
}

/*-----------------------------------------------
    Retrieves information about audio settings from recorded Device

    Called in:
    drawBar2D - DrawBottomBar
-----------------------------------------------*/
void getWaveFormat(WAVEFORMATEX* waveformat)
{
    *waveformat = *pwfx;
}

/*-----------------------------------------------
    Moves data in an array
    to have the most recent datapoints in order

    Called in:
    wasapi_audio - GetAudioBuffer
-----------------------------------------------*/
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

/*-----------------------------------------------
    Retrieves Audio Buffer or selected Audio Stream

    Called in:
    fft_lines - WndProc - WM_TIMER
-----------------------------------------------*/
HRESULT GetAudioBuffer(int16_t* buffer)
{
    HRESULT hr = S_OK;

    BYTE* pData;
    DWORD flags;
    UINT32 numFramesAvailable;
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
            int i = 0;
            LONG lBytesToWrite = numFramesAvailable * pwfx->nBlockAlign;
            float* f = (float*)malloc(N * sizeof(float));
            if (f == NULL)
                return 1;
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
            free(f);
        }

        hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
        EXIT_ON_ERROR(hr);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr);
    }

    return hr;
}