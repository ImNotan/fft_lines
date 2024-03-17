#include <windows.h>
#include <d2d1.h>

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

extern "C" HRESULT CreateGraphicsResources(HWND hwnd);
extern "C" void    DiscardGraphicsResources();
extern "C" void    OnPaint(HWND hwnd);
extern "C" HRESULT PaintStart();
void    Resize(HWND hwnd);


// Recalculate drawing layout when the size of the window changes.

HRESULT PaintStart()
{
    return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
}

HRESULT CreateGraphicsResources(HWND hwnd)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);


        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
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
        PAINTSTRUCT ps;
        D2D1_RECT_F rectangle = D2D1::Rect(5, 5, 10, 10);
        BeginPaint(hwnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
        pRenderTarget->FillRectangle(&rectangle, pBrush);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(hwnd, &ps);
    }
}

void Resize(HWND hwnd)
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);
        //CalculateLayout();
        InvalidateRect(hwnd, NULL, FALSE);
    }
}