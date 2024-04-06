#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "wasapi_audio.h"

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
ID2D1LinearGradientBrush* pbarBrushGradient[256];
ID2D1SolidColorBrush* pbarBrushSolid[256];

IDWriteTextFormat* pTextFormat;
IDWriteFactory* pWriteFactory;

D2D1_SIZE_U previousSize;
D2D1_RECT_U previousWindowRect;

extern "C" HRESULT CreateGraphicsResources(HWND hwnd);
extern "C" void    DiscardGraphicsResources();
extern "C" void    OnPaint(HWND hwnd, int frameRate);
extern "C" HRESULT PaintStart();
extern "C" void    Resize(HWND hwnd);
extern "C" void    CreateBarBrush();
extern "C" void    Redraw(HWND hwnd);

//Creates Factorys for Drawing
//Called once in fft_lines.c - WndProc - WM_CREATE
HRESULT PaintStart()
{
    previousSize = D2D1::SizeU(0, 0);
    previousWindowRect = D2D1::RectU(0, 0, 0, 0);

    HRESULT hr = S_OK;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pWriteFactory), reinterpret_cast<IUnknown**>(&pWriteFactory));
    hr = pWriteFactory->CreateTextFormat(
        L"Verdana",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12,
        L"",
        &pTextFormat);

    hr = pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    hr = pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);

    if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
    {
        WCHAR message[23] = L"Failed to start paint";
        SendMessageW(globalhwnd, WM_ERROR, (WPARAM)&message, NULL);
        DiscardGraphicsResources();
    }

    return hr;
}


void DiscardGraphicsResources()
{
    SafeRelease(&pBrush);
    SafeRelease(&pBufferBitmap);
    for (int i = 0; i < 256; i++)
    {
        SafeRelease(&pbarBrushGradient[i]);
        SafeRelease(&pbarBrushSolid[i]);
    }
    SafeRelease(&pRenderTarget);
    SafeRelease(&pFactory);
    SafeRelease(&pWriteFactory);
}

void CreateBarBrush()
{
    //Creates 256 different colored Brushes
    //in solid color and in a gradient
    for (int i = 0; i < 256; i++)
    {
        SafeRelease(&pbarBrushGradient[i]);
        SafeRelease(&pbarBrushSolid[i]);
    }

    D2D1_COLOR_F color;
    ID2D1GradientStopCollection* pGradientStops;
    D2D1_GRADIENT_STOP gradientStops[2];
    HRESULT hr = S_OK;

    int j = 0;
    for (int i = 0; i < 255; i++)
    {
        //Gradient
        gradientStops[0].color = D2D1::ColorF((float)pGradients[j], (float)pGradients[j + 1], (float)pGradients[j + 2], 1.0f);
        gradientStops[0].position = 0.0f;
        gradientStops[1].color = D2D1::ColorF((float)pGradients[j] / 2, (float)pGradients[j + 1] / 2, (float)pGradients[j + 2] / 2, 1.0f);
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
            &pbarBrushGradient[i]
        );

        //Solid
        color = D2D1::ColorF((float)pGradients[j], (float)pGradients[j + 1], (float)pGradients[j + 2], 1.0f);
        hr = pRenderTarget->CreateSolidColorBrush(color, &pbarBrushSolid[i]);

        j += 3;

        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            WCHAR message[24] = L"Failed to create brush";
            SendMessageW(globalhwnd, WM_ERROR, (WPARAM)&message, NULL);
            DiscardGraphicsResources();
        }
    }

    SafeRelease(&pGradientStops);
}

HRESULT CreateGraphicsResources(HWND hwnd)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        //Create render target
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);

        D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size, D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS),
            &pRenderTarget);

        //Create resources
        if (SUCCEEDED(hr))
        {
            //General Brush
            D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF(0.5f, 0.5f, 0.5f));
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

            //Bitmap for background effect
            hr = pRenderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBufferBitmap);

            CreateBarBrush();
        }
    }
    
    if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
    {
        WCHAR message[37] = L"Failed to create graphics resources";
        SendMessageW(globalhwnd, WM_ERROR, (WPARAM)&message, NULL);
        DiscardGraphicsResources();
    }

    return hr;
}

void DrawBackground(RECT windowRect)
{
    if (background)
    {
        D2D1_RECT_F backgroundRect = D2D1::RectF(windowRect.left, windowRect.top, 5, windowRect.bottom);
        pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
        pRenderTarget->FillRectangle(&backgroundRect, pBrush);

        backgroundRect = D2D1::RectF(windowRect.left + 5, windowRect.top, windowRect.right, windowRect.bottom);
        pBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f));
        pRenderTarget->FillRectangle(&backgroundRect, pBrush);

        backgroundRect = D2D1::RectF(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

        if (pBufferBitmap != NULL)
        {
            pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(5, -5));
            pRenderTarget->DrawBitmap(pBufferBitmap, backgroundRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, backgroundRect);
        }

        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, 0));
    }
    else
    {
        D2D1_RECT_F backgroundRect = D2D1::RectF(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
        pBrush->SetColor(D2D1::ColorF(0.3f, 0.3f, 0.3f));
        pRenderTarget->FillRectangle(&backgroundRect, pBrush);
    }
}

void DrawBars(RECT windowRect)
{
    D2D1_RECT_F barRect;
    //waveform circle gradient
    //0bwcg
    switch (waveform << 2 | circle << 1 | gradient)
    {
        case 0b000: //normal & no circle
        {
            for (int i = 0; i < barCount; i++)
            {
                barRect = D2D1::Rect(bar[i].x, (int)windowRect.bottom - bar[i].height, bar[i].x + bar[i].width, (int)windowRect.bottom);
                pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
            }
        }
        break;

        case 0b001: //gradient & no circle
        {
            for (int i = 0; i < barCount; i++)
            {
                pbarBrushGradient[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetStartPoint(D2D1::Point2F(0, windowRect.bottom - bar[i].height));
                pbarBrushGradient[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetEndPoint(D2D1::Point2F(0, windowRect.bottom));
                barRect = D2D1::Rect(bar[i].x, (int)windowRect.bottom - bar[i].height, bar[i].x + bar[i].width, (int)windowRect.bottom);
                pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
            }
        }
        break;

        case 0b010: //no gradient & circle
        {
            float rotation = 0.0f;
            int radius = 100;
            for (int i = 0; i < barCount; i++)
            {
                pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(windowRect.right / 2, windowRect.bottom / 2)));
                rotation += 360.0f / barCount;

                barRect = D2D1::Rect(windowRect.right / 2 - 2, windowRect.bottom / 2 + radius + bar[i].height, windowRect.right / 2 + 2, windowRect.bottom / 2 + radius);
                pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
            }

            pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
        }
        break;

        case 0b011: //gradient & circle
        {
            float rotation = 0.0f;
            int radius = 100;
            for (int i = 0; i < barCount; i++)
            {
                pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(windowRect.right / 2, windowRect.bottom / 2)));
                rotation += 360.0f / barCount;

                pbarBrushGradient[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetStartPoint(D2D1::Point2F(windowRect.right / 2, windowRect.bottom / 2 + radius + bar[i].height));
                pbarBrushGradient[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]->SetEndPoint(D2D1::Point2F(windowRect.right / 2, windowRect.bottom / 2 + radius));

                barRect = D2D1::Rect(windowRect.right / 2 - 2, windowRect.bottom / 2 + radius + bar[i].height, windowRect.right / 2 + 2, windowRect.bottom / 2 + radius);
                pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
            }

            pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
        }
        break;

        case 0b100:
        {
            for (int i = 0; i < barCount; i++)
            {
                barRect = D2D1::Rect(bar[i].x, (int)windowRect.bottom - bar[i].height, bar[i].x + bar[i].width, (int)windowRect.bottom - bar[i].height + 20);
                pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[(unsigned int)((float)(((float)i / ((float)barCount - 1.0)) * 254.0))]);
            }
        }
        break;
    }
}

void DrawFrameRate(RECT windowRect, int frameRate)
{
    D2D1_RECT_F textRect;

    textRect = D2D1::Rect(windowRect.left, windowRect.top, windowRect.left + 45, windowRect.bottom);
    wchar_t buffer[15] = L"       ";
    wsprintfW(buffer, L"%d", frameRate);

    pBrush->SetColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    pRenderTarget->FillRectangle(&textRect, pBrush);
    pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
    pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
}

void OnPaint(HWND hwnd, int frameRate)
{
    HRESULT hr = CreateGraphicsResources(hwnd);
    if (SUCCEEDED(hr))
    {
        //windowRect adjusted for bar space
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        windowRect.bottom -= bottomBarHeihgt;
        
        pRenderTarget->BeginDraw();

        //Solid or with Background effect
        DrawBackground(windowRect);
        DrawBars(windowRect);

        //windowRect adjusted for bottom bar
        windowRect.bottom += bottomBarHeihgt;
        windowRect.top = windowRect.bottom - bottomBarHeihgt;

        DrawFrameRate(windowRect, frameRate);

        if (background)
        {
            //windowRect adjusted for bar space
            GetClientRect(hwnd, &windowRect);
            windowRect.bottom -= bottomBarHeihgt;

            //Creates Bitmap with the current bars which is drawn on next frame with a transform
            D2D1_POINT_2U upperLeft = D2D1::Point2U(0, 0);
            D2D1_RECT_U d2d1windowRectU = D2D1::RectU(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
            pBufferBitmap->CopyFromRenderTarget(&upperLeft, pRenderTarget, &d2d1windowRectU);
        }

        hr = pRenderTarget->EndDraw();

        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            WCHAR message[22] = L"Failed to draw frame";
            SendMessageW(globalhwnd, WM_ERROR, (WPARAM)&message, NULL);
            DiscardGraphicsResources();
        }
    }
}

void Redraw(HWND hwnd)
{
    WAVEFORMATEX wfx;
    getWaveFormat(&wfx);

    HRESULT hr = S_OK;
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);

    pRenderTarget->BeginDraw();

    //Same as in OnPaint() done here for new size (no flickering)
    windowRect.bottom -= bottomBarHeihgt;

    DrawBackground(windowRect);
    DrawBars(windowRect);

    //windowRect adjusted for bottom bar
    windowRect.bottom += bottomBarHeihgt;
    windowRect.top = windowRect.bottom - bottomBarHeihgt;

    D2D1_RECT_F bottomBar = D2D1::Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

    //Fill bottom bar
    pBrush->SetColor(D2D1::ColorF(0.15f, 0.15f, 0.15f));
    pRenderTarget->FillRectangle(bottomBar, pBrush);
    //Border bottom bar
    pBrush->SetColor(D2D1::ColorF(0, 0, 0));
    pRenderTarget->DrawRectangle(bottomBar, pBrush);

    D2D1_RECT_F textRect;
    pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));

    //Calculate frequency at 10 spots and displays them
    for (int i = 0; i < 10; i++)
    {
        textRect = D2D1::Rect(i * (windowRect.right / 10), windowRect.bottom, i * (windowRect.right / 10) + (windowRect.right / 10), windowRect.bottom - bottomBarHeihgt);
        int freq = (i * (barCount / 10) + (barCount / 20)) * (wfx.nSamplesPerSec / N);
        wchar_t buffer[9] = L"        ";
        wsprintfW(buffer, L"%dHz ", freq);
        pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
    }

    hr = pRenderTarget->EndDraw();

    if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
    {
        WCHAR message[20] = L"Failed to redraw";
        SendMessageW(globalhwnd, WM_ERROR, (WPARAM)&message, NULL);
        DiscardGraphicsResources();
    }
}

void Resize(HWND hwnd)
{
    if (pRenderTarget != NULL)
    {
        HRESULT hr = S_OK;
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        D2D1_RECT_U windowRectU = D2D1::RectU(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

        //Resize / Creation of resized Bitmap
        D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);
        hr = pRenderTarget->Resize(size);

        ID2D1Bitmap* pPreviousBitmap;
        D2D1_POINT_2U destPoint = D2D1::Point2U(0, 0);
        hr = pRenderTarget->CreateBitmap(previousSize, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pPreviousBitmap);
        hr = pPreviousBitmap->CopyFromBitmap(&destPoint, pBufferBitmap, &previousWindowRect);

        SafeRelease(&pBufferBitmap);
        hr = pRenderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBufferBitmap);

        hr = pBufferBitmap->CopyFromBitmap(&destPoint, pPreviousBitmap, &windowRectU);
        SafeRelease(&pPreviousBitmap);
        
        Redraw(hwnd);

        previousWindowRect = windowRectU;
        previousSize = size;

        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            WCHAR message[20] = L"Failed to resize";
            SendMessageW(globalhwnd, WM_ERROR, (WPARAM)&message, NULL);
            DiscardGraphicsResources();
        }
    }
}