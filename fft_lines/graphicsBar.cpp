#include <windows.h>
#include <d2d1.h>

#include "global.h"

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

extern "C" HRESULT CreateGraphicsResources();
extern "C" void    DiscardGraphicsResources();
extern "C" void    OnPaint();
void    Resize();


// Recalculate drawing layout when the size of the window changes.

HRESULT CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(globalhwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(globalhwnd, size),
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
}

void OnPaint()
{
    HRESULT hr;
    //HRESULT hr = CreateGraphicsResources();
    //if (SUCCEEDED(hr))
    //{
        PAINTSTRUCT ps;
        D2D1_RECT_F rectangle = D2D1::Rect(5, 5, 10, 10);
        BeginPaint(globalhwnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
        pRenderTarget->FillRectangle(&rectangle, pBrush);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(globalhwnd, &ps);
    //}
}

void Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(globalhwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);
        //CalculateLayout();
        InvalidateRect(globalhwnd, NULL, FALSE);
    }
}

//LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//    switch (uMsg)
//    {
//    case WM_CREATE:
//        if (FAILED(D2D1CreateFactory(
//            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
//        {
//            return -1;  // Fail CreateWindowEx.
//        }
//        return 0;
//
//    case WM_DESTROY:
//        DiscardGraphicsResources();
//        SafeRelease(&pFactory);
//        PostQuitMessage(0);
//        return 0;
//
//    case WM_PAINT:
//        OnPaint();
//        return 0;
//
//
//
//    case WM_SIZE:
//        Resize();
//        return 0;
//    }
//    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
//}