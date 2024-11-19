#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "wasapi_audio.h"

extern "C"
{
#include "settingsFile.h"
#include "variables.h"
#include "global.h"

#include "beatDetector.h"
}

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#define SAFE_RELEASE(ppT)  \
                  if ((ppT) != NULL)  \
                    { (ppT)->Release(); (ppT) = NULL; }

#define FILE_ERROR_CODE 0x00000101

#define CHECK_NULL(ppT) \
                  if((ppT) == NULL)  \
                    { PostMessageW(globalhwnd, WM_ERROR, E_FAIL, FILE_ERROR_CODE); return E_FAIL; }

#define CHECK_ERROR(hr) \
                  if(FAILED(hr))  \
                    { PostMessageW(globalhwnd, WM_ERROR, hr, FILE_ERROR_CODE); return hr; }

#define RANGE255(number, maxValueOfNumber) (unsigned int)((float)(((float)number / ((float)maxValueOfNumber - 1.0)) * 254.0))

#define doStereo 0b100
#define doCircle 0b010
#define doGradient 0b001

/*-----------------------------------------------
Internal Objects

	Rendering Objects:
		Factory
		Render Target

	Resources:
		Buffer Bitmap (for 3D background)
		Brush
		bar Brusch :
			Solid (gradient horizontal)
			Gradient (gradient vertical and horizontal)
		Radial Brush (for circle mode)

	Text Objects:
		Factory
		Text Format

	Buffer bitmap size (for 3D background):
		previous Size
		previous WindowRect
-----------------------------------------------*/

ID2D1Factory* pFactory;
ID2D1HwndRenderTarget* pRenderTarget;

ID2D1Bitmap* pBufferBitmap;
ID2D1SolidColorBrush* pBrush;
ID2D1LinearGradientBrush* pbarBrushGradient[256];
ID2D1SolidColorBrush* pbarBrushSolid[256];
ID2D1RadialGradientBrush* pradialBrush;

IDWriteFactory* pWriteFactory;
IDWriteTextFormat* pTextFormat;

D2D1_SIZE_U previousSize;
D2D1_RECT_U previousWindowRect;

/*-----------------------------------------------
Public functions

	Initialization:
		PaintStart
		DiscardGraphicsResources
		ChangeBarBrush

	Drawing:
		OnPaint
		Resize
-----------------------------------------------*/

extern "C" HRESULT PaintStart();
extern "C" void DiscardGraphicsResources();
extern "C" HRESULT ChangeBarBrush();

extern "C" HRESULT OnPaint(HWND hwnd, int frameRate);
extern "C" HRESULT Resize(HWND hwnd);

/*-----------------------------------------------
Internal functions

	Initialization:
		CreateGraphicsResources
		CreateBarBrush

	Drawing:
		DrawBottomBar
		DrawBackground
		DrawBars
		DrawFrameRate
		CallDraws
-----------------------------------------------*/

HRESULT CreateGraphicsResources(HWND hwnd);
HRESULT CreateBarBrush();

void DrawBottomBar(HWND hwnd);
void DrawBackground(RECT windowRect);
void DrawBars(RECT windowRect);
void DrawFrameRate(RECT windowRect, int frameRate);
HRESULT CallDraws(HWND hwnd, int frameRate);

/*-----------------------------------------------
	Initialization of drawBar2D

	Creates Factories for Drawing
	Called in:
	fft_lines - WndProc - WM_CREATE
-----------------------------------------------*/
HRESULT PaintStart()
{
	HRESULT hr = S_OK;

	D2D1_SIZE_U previousSize = D2D1::SizeU(0, 0);
	D2D1_RECT_U previousWindowRect = D2D1::RectU(0, 0, 0, 0);

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pWriteFactory), reinterpret_cast<IUnknown**>(&pWriteFactory));
	CHECK_ERROR(hr);

	hr = pWriteFactory->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		12,
		L"",
		&pTextFormat);
	CHECK_ERROR(hr);

	hr = pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	CHECK_ERROR(hr);

	hr = pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	CHECK_ERROR(hr);

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	CHECK_ERROR(hr);

	return hr;
}

/*-----------------------------------------------
	Unitialization of drawBar2D

	Called in:
	drawBar2D - HandleError_Graphics
	fft_lines - WndProc - WM_DESTROY
-------------------------------------------------*/
void DiscardGraphicsResources()
{
	SAFE_RELEASE(pBrush);
	SAFE_RELEASE(pBufferBitmap);
	for (int i = 0; i < 256; i++)
	{
		SAFE_RELEASE(pbarBrushGradient[i]);
		SAFE_RELEASE(pbarBrushSolid[i]);
	}
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pFactory);
	SAFE_RELEASE(pWriteFactory);
}

/*-----------------------------------------------
	Public wrapper for CreateBarBrush with Error Handling

	Called in:
	settings - SettingsDlgProc - WM_COMMAND - IDC_COMBO_COLORS
-----------------------------------------------*/
HRESULT ChangeBarBrush()
{
	HRESULT hr = S_OK;
	hr = CreateBarBrush();
	CHECK_ERROR(hr);

	return hr;
}

/*-----------------------------------------------
	Creation of Brushes for drawing Bars

	Called in:
	drawBar2D - CreateGraphicsResources
-----------------------------------------------*/
HRESULT CreateBarBrush()
{
	//Creates 256 different colored Brushes
	//in solid color and in a gradient
	for (int i = 0; i < 256; i++)
	{
		SAFE_RELEASE(pbarBrushGradient[i]);
		SAFE_RELEASE(pbarBrushSolid[i]);
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
		CHECK_ERROR(hr);
		CHECK_NULL(pGradientStops);

		hr = pRenderTarget->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(
				D2D1::Point2F(0, 0),
				D2D1::Point2F(0, 1)),
			pGradientStops,
			&pbarBrushGradient[i]
		);
		CHECK_ERROR(hr);

		//Solid
		color = D2D1::ColorF((float)pGradients[j], (float)pGradients[j + 1], (float)pGradients[j + 2], 1.0f);
		hr = pRenderTarget->CreateSolidColorBrush(color, &pbarBrushSolid[i]);
		CHECK_ERROR(hr);

		j += 3;
	}

	SAFE_RELEASE(pGradientStops);

	return hr;
}

/*-----------------------------------------------
	Creates Render Target and Resources

	Called in:
	drawBar2D - OnPaint
-----------------------------------------------*/
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
			D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE)),
			D2D1::HwndRenderTargetProperties(hwnd, size, D2D1_PRESENT_OPTIONS_NONE),
			&pRenderTarget);
		CHECK_ERROR(hr);

		pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

		//Create resources
		if (SUCCEEDED(hr))
		{
			//General Brush
			D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF(0.5f, 0.5f, 0.5f));
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
			CHECK_ERROR(hr);

			//Bitmap for background effect
			hr = pRenderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBufferBitmap);
			CHECK_ERROR(hr);

			ID2D1GradientStopCollection* pGradientStops;
			D2D1_GRADIENT_STOP gradientStops[2];

			gradientStops[0].color = D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.0f);
			gradientStops[0].position = 0.0f;
			gradientStops[1].color = D2D1::ColorF(0.25f, 0.25f, 0.25f, 1.0f);
			gradientStops[1].position = 1.0f;

			hr = pRenderTarget->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			CHECK_ERROR(hr);
			CHECK_NULL(pGradientStops);

			hr = pRenderTarget->CreateRadialGradientBrush(
				D2D1::RadialGradientBrushProperties(
					D2D1::Point2F(windowRect.right / 2, ((windowRect.bottom - bottomBarHeihgt) / 2)),
					D2D1::Point2F(0, 0),
					200,
					200),
				pGradientStops,
				&pradialBrush
			);
			CHECK_ERROR(hr);

			hr = CreateBarBrush();
		}
	}

	return hr;
}

/*-----------------------------------------------
	Drawing
-----------------------------------------------*/
void DrawBottomBar(HWND hwnd)
{
	WAVEFORMATEX wfx;
	getWaveFormat(&wfx);

	RECT windowRect;

	GetClientRect(hwnd, &windowRect);

	windowRect.top = windowRect.bottom - bottomBarHeihgt;

	D2D1_RECT_F bottomBar = D2D1::Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

	//Fill bottom bar
	pBrush->SetColor(D2D1::ColorF(0.10f, 0.10f, 0.10f));
	pRenderTarget->FillRectangle(bottomBar, pBrush);
	//Border bottom bar
	pBrush->SetColor(D2D1::ColorF(0, 0, 0));
	pRenderTarget->DrawRectangle(bottomBar, pBrush);

	D2D1_RECT_F textRect;
	pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));

	//Calculate frequency at 10 spots and displays them
	if (stereo == 1)
	{
		windowRect.left = windowRect.right / 2;
		long horizontalSize = windowRect.right - windowRect.left;
		long horizontalSizeSplit = horizontalSize / 10;
		int barCountSplit10 = barCount / 10;
		int barCountSplit20 = barCountSplit10 / 2;
		int j = 9;
		for (int i = 0; i < 10; i++)
		{
			textRect = D2D1::Rect(windowRect.left + i * horizontalSizeSplit, windowRect.bottom, windowRect.left + i * horizontalSizeSplit + horizontalSizeSplit, windowRect.bottom - bottomBarHeihgt);
			int freq = (j * barCountSplit10 + barCountSplit20) * (wfx.nSamplesPerSec / N);
			wchar_t buffer[9] = L"        ";
			wsprintfW(buffer, L"%dHz ", freq);
			pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
			j--;
		}

		windowRect.left = 0;
		windowRect.right = windowRect.right / 2;
		for (int i = 0; i < 10; i++)
		{
			textRect = D2D1::Rect(windowRect.left + i * horizontalSizeSplit, windowRect.bottom, windowRect.left + i * horizontalSizeSplit + horizontalSizeSplit, windowRect.bottom - bottomBarHeihgt);
			int freq = (i * barCountSplit10 + barCountSplit20) * (wfx.nSamplesPerSec / N);
			wchar_t buffer[9] = L"        ";
			wsprintfW(buffer, L"%dHz ", freq);
			pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
		}
	}
	else if (stereo == 0)
	{
		long horizontalSizeSplit = windowRect.right / 10;
		int barCountSplit10 = barCount / 10;
		int barCountSplit20 = barCountSplit10 / 2;
		for (int i = 0; i < 10; i++)
		{
			textRect = D2D1::Rect(i * horizontalSizeSplit, windowRect.bottom, i * horizontalSizeSplit + horizontalSizeSplit, windowRect.bottom - bottomBarHeihgt);
			int freq = (i * barCountSplit10 + barCountSplit20) * (wfx.nSamplesPerSec / N);
			wchar_t buffer[9] = L"        ";
			wsprintfW(buffer, L"%dHz ", freq);
			pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
		}
	}
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
		pBrush->SetColor(D2D1::ColorF(0.2f, 0.2f, 0.2f));
		pRenderTarget->FillRectangle(&backgroundRect, pBrush);

		if (circle)
		{
			int beatValue = GetBeatValue();
			D2D1_ELLIPSE backgroundEllipse = D2D1::Ellipse(D2D1::Point2F(windowRect.right / 2, (windowRect.bottom / 2) - 40), beatValue, beatValue);
			pradialBrush->SetCenter(D2D1::Point2F(windowRect.right / 2, (windowRect.bottom / 2) - 40));
			pradialBrush->SetRadiusX(beatValue);
			pradialBrush->SetRadiusY(beatValue);
			pRenderTarget->FillEllipse(backgroundEllipse, pradialBrush);
		}
	}
}

void DrawBars(RECT windowRect)
{
	D2D1_RECT_F barRect;
	//circle gradient
	//0bcg

	if (waveform)
	{
		for (int i = 0; i < DEFAULT_WAVEBARBOUNT; i++)
		{
			barRect = D2D1::Rect(waveBar[i].x, (int)windowRect.bottom - waveBar[i].height, waveBar[i].x + waveBar[i].width, (int)windowRect.bottom - waveBar[i].height + 20);
			pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[RANGE255(i, DEFAULT_WAVEBARBOUNT)]);
		}
	}

	if (dofft)
	{
		long centerX = windowRect.right / 2;
		long centerY = windowRect.bottom / 2;

		switch (stereo << 2 | circle << 1 | gradient)
		{
			case doStereo: //draws stereo first and then does no stereo & no gradient & no cricle
			{
				for (int i = 0; i < barCount; i++)
				{
					barRect = D2D1::Rect(barRight[i].x, (int)windowRect.bottom - barRight[i].height, barRight[i].x + barRight[i].width, (int)windowRect.bottom);
					pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[RANGE255(i, barCount)]);
				}
			}
			case 0b000: //no stereo & no gradient & no circle
			{
				for (int i = 0; i < barCount; i++)
				{
					barRect = D2D1::Rect(barLeft[i].x, (int)windowRect.bottom - barLeft[i].height, barLeft[i].x + barLeft[i].width, (int)windowRect.bottom);
					pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[RANGE255(i, barCount)]);
				}
			}
			break;

			case (doStereo | doGradient): //draws stereo first and then does no stereo & gradient & no circle
			{
				unsigned int selectedBrush = 0;
				for (int i = 0; i < barCount; i++)
				{
					selectedBrush = RANGE255(i, barCount);
					pbarBrushGradient[selectedBrush]->SetStartPoint(D2D1::Point2F(0, windowRect.bottom - barRight[i].height));
					pbarBrushGradient[selectedBrush]->SetEndPoint(D2D1::Point2F(0, windowRect.bottom));
					barRect = D2D1::Rect(barRight[i].x, (int)windowRect.bottom - barRight[i].height, barRight[i].x + barRight[i].width, (int)windowRect.bottom);
					pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[selectedBrush]);
				}
			}
			case doGradient: //no stereo & gradient & no circle
			{
				unsigned int selectedBrush = 0;
				for (int i = 0; i < barCount; i++)
				{
					selectedBrush = RANGE255(i, barCount);
					pbarBrushGradient[selectedBrush]->SetStartPoint(D2D1::Point2F(0, windowRect.bottom - barLeft[i].height));
					pbarBrushGradient[selectedBrush]->SetEndPoint(D2D1::Point2F(0, windowRect.bottom));
					barRect = D2D1::Rect(barLeft[i].x, (int)windowRect.bottom - barLeft[i].height, barLeft[i].x + barLeft[i].width, (int)windowRect.bottom);
					pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[selectedBrush]);
				}
			}
			break;

			case (doStereo | doCircle):
			{
				float rotation = 0.0f;
				int radius = 200;
				centerY -= 40;
				for (int i = 0; i < barCount; i++)
				{
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(centerX, centerY)));
					rotation -= 180.0f / barCount;

					barRect = D2D1::Rect(centerX - 3, centerY + radius + barLeft[i].height, centerX + 3, centerY + radius);
					pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[RANGE255(i, barCount)]);
				}

				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
				rotation = 0.0f;
				radius = 200;
				for (int i = 0; i < barCount; i++)
				{
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(centerX, centerY)));
					rotation += 180.0f / barCount;

					barRect = D2D1::Rect(centerX - 3, centerY + radius + barLeft[i].height, centerX + 3, centerY + radius);
					pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[RANGE255(i, barCount)]);
				}

				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
			}
			break;
			case doCircle: //no stereo & no gradient & circle
			{
				float rotation = 0.0f;
				int radius = 200;
				centerY -= 40;
				for (int i = 0; i < barCount; i++)
				{
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(centerX, centerY)));
					rotation += 360.0f / barCount;

					barRect = D2D1::Rect(centerX - 3, centerY + radius + barLeft[i].height, centerX + 3, centerY + radius);
					pRenderTarget->FillRectangle(&barRect, pbarBrushSolid[RANGE255(i, barCount)]);
				}

				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
			}
			break;

			case (doStereo | doCircle | doGradient): //first does stereo & gradient & circle and then no stereo & gradient & circle
			{
				float rotation = 0.0f;
				int radius = 200;
				unsigned int selectedBrush = 0;
				centerY -= 40;
				for (int i = 0; i < barCount; i++)
				{
					selectedBrush = RANGE255(i, barCount);
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(centerX, centerY)));
					rotation -= 180.0f / barCount;

					pbarBrushGradient[selectedBrush]->SetStartPoint(D2D1::Point2F(centerX, centerY + radius + barRight[i].height));
					pbarBrushGradient[selectedBrush]->SetEndPoint(D2D1::Point2F(centerX, centerY + radius));

					barRect = D2D1::Rect(centerX - 3, centerY + radius + barRight[i].height, centerX + 3, centerY + radius);
					pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[selectedBrush]);
				}

				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));

				rotation = 0.0f;
				for (int i = 0; i < barCount; i++)
				{
					selectedBrush = RANGE255(i, barCount);
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(centerX, centerY)));
					rotation += 180.0f / barCount;

					pbarBrushGradient[selectedBrush]->SetStartPoint(D2D1::Point2F(centerX, centerY + radius + barLeft[i].height));
					pbarBrushGradient[selectedBrush]->SetEndPoint(D2D1::Point2F(centerX, centerY + radius));

					barRect = D2D1::Rect(centerX - 3, centerY + radius + barLeft[i].height, centerX + 3, centerY + radius);
					pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[selectedBrush]);
				}

				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
			}
			break;
			case (doCircle | doGradient): //no stereo & gradient & circle
			{
				float rotation = 0.0f;
				int radius = 200;
				unsigned int selectedBrush = 0;
				centerY -= 40;
				for (int i = 0; i < barCount; i++)
				{
					selectedBrush = RANGE255(i, barCount);
					pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, D2D1::Point2F(centerX, centerY)));
					rotation += 360.0f / barCount;

					pbarBrushGradient[selectedBrush]->SetStartPoint(D2D1::Point2F(centerX, centerY + radius + barLeft[i].height));
					pbarBrushGradient[selectedBrush]->SetEndPoint(D2D1::Point2F(centerX, centerY + radius));

					barRect = D2D1::Rect(centerX - 3, centerY + radius + barLeft[i].height, centerX + 3, centerY + radius);
					pRenderTarget->FillRectangle(&barRect, pbarBrushGradient[selectedBrush]);
				}

				pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(0.0f, D2D1::Point2F(0, 0)));
			}
			break;
		}
	}
}

void DrawFrameRate(RECT windowRect, int frameRate)
{
	D2D1_RECT_F textRect;

	textRect = D2D1::Rect(windowRect.left, windowRect.top, windowRect.left + 45, windowRect.bottom);
	wchar_t buffer[15] = L"       ";
	wsprintfW(buffer, L"%d", frameRate);

	pBrush->SetColor(D2D1::ColorF(0.10f, 0.10f, 0.10f));
	pRenderTarget->FillRectangle(&textRect, pBrush);
	pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
	pRenderTarget->DrawTextW(buffer, 8, pTextFormat, textRect, pBrush);
}

HRESULT CallDraws(HWND hwnd, int frameRate)
{
	HRESULT hr = S_OK;
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

	if (circle || redrawAll)
	{
		DrawBottomBar(hwnd);
	}

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
	CHECK_ERROR(hr);

	return hr;
}

/*-----------------------------------------------
	Updates Frame

	Called in:
	fft_lines - WndProc - WM_TIMER
-----------------------------------------------*/
HRESULT OnPaint(HWND hwnd, int frameRate)
{
	HRESULT hr = S_OK;
	hr = CreateGraphicsResources(hwnd);
	CHECK_ERROR(hr);

	hr = CallDraws(hwnd, frameRate);
	CHECK_ERROR(hr);

	return hr;
}

/*-----------------------------------------------
	Resize Window and background Bitmap

	Called in:
	fft_lines - WndProc - WM_SIZE
-----------------------------------------------*/
HRESULT Resize(HWND hwnd)
{
	CHECK_NULL(pRenderTarget);

	HRESULT hr = S_OK;

	RECT windowRect;
	GetClientRect(hwnd, &windowRect);
	D2D1_RECT_U windowRectU = D2D1::RectU(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

	//Resize render target
	D2D1_SIZE_U size = D2D1::SizeU(windowRect.right, windowRect.bottom);
	hr = pRenderTarget->Resize(size);
	CHECK_ERROR(hr);

	// Creation of resized Bitmap
	ID2D1Bitmap* pPreviousBitmap;
	D2D1_POINT_2U destPoint = D2D1::Point2U(0, 0);
	hr = pRenderTarget->CreateBitmap(previousSize, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pPreviousBitmap);
	CHECK_ERROR(hr);
	CHECK_NULL(pPreviousBitmap);
	hr = pPreviousBitmap->CopyFromBitmap(&destPoint, pBufferBitmap, &previousWindowRect);
	CHECK_ERROR(hr);

	SAFE_RELEASE(pBufferBitmap);
	hr = pRenderTarget->CreateBitmap(size, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &pBufferBitmap);
	CHECK_ERROR(hr);
	CHECK_NULL(pBufferBitmap);

	hr = pBufferBitmap->CopyFromBitmap(&destPoint, pPreviousBitmap, &windowRectU);
	SAFE_RELEASE(pPreviousBitmap);
	CHECK_ERROR(hr);

	previousWindowRect = windowRectU;
	previousSize = size;

	//redraw bottombar on next call in CallDraws
	redrawAll = true;

	return hr;
}