#include <windows.h>
#include <d2d1.h>

extern "C" 
{
    #include "settings.h"
    #include "variables.h"
}


#pragma comment(lib, "d2d1.lib")

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}


ID2D1Factory* pFactory;
ID2D1HwndRenderTarget* pRenderTarget;
ID2D1SolidColorBrush* pBrush;
ID2D1SolidColorBrush* backgroundBrush;
ID2D1SolidColorBrush* pbarBrush[256];

extern "C" HRESULT CreateGraphicsResources(HWND hwnd);
extern "C" void    DiscardGraphicsResources();
extern "C" void    OnPaint(HWND hwnd);
extern "C" HRESULT PaintStart();
extern "C" void    Resize(HWND hwnd);


// Recalculate drawing layout when the size of the window changes.

void CreateBarBrush()
{

}

HRESULT PaintStart()
{
    return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
}

HRESULT CreateGraphicsResources(HWND hwnd)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);

        D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Blue);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
            color = D2D1::ColorF(D2D1::ColorF::Gray);
            hr = pRenderTarget->CreateSolidColorBrush(color, &backgroundBrush);

            for (int i = 0; i < 255; i++)
            {
                color = D2D1::ColorF((float)plasma[i][0], (float)plasma[i][1], (float)plasma[i][2], 1.0f);
                pRenderTarget->CreateSolidColorBrush(color, &pbarBrush[i]);
            }
        }
    }
    return hr;
}

void DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
    SafeRelease(&pFactory);
}

void OnPaint(HWND hwnd)
{
    HRESULT hr = CreateGraphicsResources(hwnd);
    if (SUCCEEDED(hr))
    {
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        windowRect.bottom -= bottomBarHeihgt;
        PAINTSTRUCT ps;
        D2D1_RECT_F barRect;
        BeginPaint(hwnd, &ps);

        pRenderTarget->BeginDraw();

        //pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::DarkGray));
        D2D1_RECT_F background = D2D1::Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
        pRenderTarget->FillRectangle(&background, backgroundBrush);

        for (int i = 0; i < barCount; i++)
        {
            barRect = D2D1::Rect(bar[i].x, (int)windowRect.bottom - bar[i].height, bar[i].x + bar[i].width, (int)windowRect.bottom);
            pRenderTarget->FillRectangle(&barRect, pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
        }

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(hwnd, &ps);
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

void Resize(HWND hwnd)
{
    if (pRenderTarget != NULL)
    {
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);

        D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);

        pRenderTarget->Resize(size);
        //CalculateLayout();
        InvalidateRect(hwnd, NULL, FALSE);
    }
}