#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

extern "C" 
{
    #include "settings.h"
    #include "variables.h"
    #include "global.h"
}


#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

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
ID2D1Bitmap* pBufferBitmap;
ID2D1SolidColorBrush* pBrush;
ID2D1SolidColorBrush* backgroundBrush;
ID2D1LinearGradientBrush* pbarBrush[256];

IDWriteTextFormat* pTextFormat;
IDWriteFactory* pWriteFactory;

extern "C" HRESULT CreateGraphicsResources(HWND hwnd);
extern "C" void    DiscardGraphicsResources();
extern "C" void    OnPaint(HWND hwnd, int framerate);
extern "C" HRESULT PaintStart();
extern "C" void    Resize(HWND hwnd, DWORD nSamplesPerSec);


// Recalculate drawing layout when the size of the window changes.

void CreateBarBrush()
{

}

HRESULT PaintStart()
{
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pWriteFactory), reinterpret_cast<IUnknown**>(&pWriteFactory));
    pWriteFactory->CreateTextFormat(
        L"Verdana",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12,
        L"",
        &pTextFormat);

    pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);


    return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
}

HRESULT CreateGraphicsResources(HWND hwnd)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        //windowRect.bottom -= bottomBarHeihgt;

        D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size, D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF(0.5f, 0.5f, 0.5f));
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
            color = D2D1::ColorF(D2D1::ColorF(D2D1::ColorF(0.25f, 0.25f, 0.25f)));
            hr = pRenderTarget->CreateSolidColorBrush(color, &backgroundBrush);

            hr = pRenderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBufferBitmap);

            ID2D1GradientStopCollection* pGradientStops = NULL;

            D2D1_GRADIENT_STOP gradientStops[2];
            

            for (int i = 0; i < 255; i++)
            {
                gradientStops[0].color = D2D1::ColorF((float)turbo[i][0], (float)turbo[i][1], (float)turbo[i][2], 1.0f);
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF((float)turbo[i][0] / 2, (float)turbo[i][1] / 2, (float)turbo[i][2] / 2, 1.0f);
                gradientStops[1].position = 1.0f;

                hr = pRenderTarget->CreateGradientStopCollection(
                    gradientStops,
                    2,
                    D2D1_GAMMA_2_2,
                    D2D1_EXTEND_MODE_CLAMP,
                    &pGradientStops
                );

                hr = pRenderTarget->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(
                        D2D1::Point2F(0, 0),
                        D2D1::Point2F(0, 1)),
                    pGradientStops,
                    &pbarBrush[i]
                );

                //color = D2D1::ColorF((float)turbo[i][0], (float)turbo[i][1], (float)turbo[i][2], 1.0f);
                //pRenderTarget->CreateSolidColorBrush(color, &pbarBrush[i]);
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

void OnPaint(HWND hwnd, int framerate)
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

        D2D1_POINT_2U upperLeft = D2D1::Point2U(0, 0);
        D2D1_RECT_U d2d1windowRectU = D2D1::RectU(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
        D2D1_RECT_F d2d1windowRectF = D2D1::RectF(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

        pRenderTarget->BeginDraw();

        D2D1_RECT_F background = D2D1::Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
        pBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f));
        pRenderTarget->FillRectangle(&background, pBrush);

        if (pBufferBitmap != NULL)
        {
            pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(5, -5));
            pRenderTarget->DrawBitmap(pBufferBitmap, d2d1windowRectF, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, d2d1windowRectF);
        }

        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, 0));

        //pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::DarkGray));

        for (int i = 0; i < barCount; i++)
        {
            pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetStartPoint(D2D1::Point2F(0, windowRect.bottom - bar[i].height));
            pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetEndPoint(D2D1::Point2F(0, windowRect.bottom));
            barRect = D2D1::Rect(bar[i].x, (int)windowRect.bottom - bar[i].height, bar[i].x + bar[i].width, (int)windowRect.bottom);
            pRenderTarget->FillRectangle(&barRect, pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
        }

        D2D1_RECT_F textRect;

        windowRect.bottom += bottomBarHeihgt;
        windowRect.top = windowRect.bottom - bottomBarHeihgt;

        textRect = D2D1::Rect(windowRect.left, windowRect.top, windowRect.left + 45, windowRect.bottom);
        wchar_t buffer[15] = L"       ";
        wsprintfW(buffer, L"%d", framerate);

        pBrush->SetColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
        pRenderTarget->FillRectangle(&textRect, pBrush);
        pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
        pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);

        pBufferBitmap->CopyFromRenderTarget(&upperLeft, pRenderTarget, &d2d1windowRectU);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(hwnd, &ps);
        //InvalidateRect(hwnd, NULL, FALSE);
        
    }
}

void Resize(HWND hwnd, DWORD nSamplesPerSec)
{
    if (pRenderTarget != NULL)
    {
        HRESULT hr;
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        //windowRect.bottom -= bottomBarHeihgt;

        D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);

        pRenderTarget->Resize(size);
        pRenderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBufferBitmap);

        pRenderTarget->BeginDraw();

        windowRect.bottom -= bottomBarHeihgt;

        D2D1_RECT_F barRect;
        //BeginPaint(hwnd, &ps);

        //pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::DarkGray));
        D2D1_RECT_F background = D2D1::Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
        pRenderTarget->FillRectangle(&background, backgroundBrush);

        for (int i = 0; i < barCount; i++)
        {
            pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetStartPoint(D2D1::Point2F(0, windowRect.bottom - bar[i].height));
            pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetEndPoint(D2D1::Point2F(0, windowRect.bottom));
            barRect = D2D1::Rect(bar[i].x, (int)windowRect.bottom - bar[i].height, bar[i].x + bar[i].width, (int)windowRect.bottom);
            pRenderTarget->FillRectangle(&barRect, pbarBrush[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
        }

        windowRect.bottom += bottomBarHeihgt;
        windowRect.top = windowRect.bottom - bottomBarHeihgt;

        D2D1_RECT_F bottomBar = D2D1::Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
        
        pBrush->SetColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
        pRenderTarget->FillRectangle(bottomBar, pBrush);
        pBrush->SetColor(D2D1::ColorF(0, 0, 0));
        pRenderTarget->DrawRectangle(bottomBar, pBrush);

        D2D1_RECT_F textRect;
        pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));

        for (int i = 0; i < 10; i++)
        {
            textRect = D2D1::Rect(i * (windowRect.right / 10), windowRect.bottom, i * (windowRect.right / 10) + (windowRect.right / 10), windowRect.bottom - bottomBarHeihgt);
            int freq = (i * (barCount / 10) + (barCount / 20)) * (nSamplesPerSec / N);
            wchar_t buffer[9] = L"        ";
            wsprintfW(buffer, L"%dHz ", freq);
            pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
        }

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        //CalculateLayout();
        //InvalidateRect(hwnd, NULL, FALSE);
    }
}