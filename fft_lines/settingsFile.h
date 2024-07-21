#pragma once
#define MAX_BARCOUNT 1000
#define MIN_BARCOUNT 100
#define MAX_ZOOM 0.001f
#define MIN_ZOOM 0.00001f
#define NUMOF_GRADIENTS 6

#define DEFAULT_BARCOUNT 500
#define DEFAULT_ZOOM 0.0001f
#define DEFAULT_COLORSEL 0
#define DEFAULT_FFT 1
#define DEFAULT_BACKGROUND 1
#define DEFAULT_GRADIENT 1
#define DEFAULT_IGNORESERIAL 1
#define DEFAULT_CIRCLE 0
#define DEFAULT_WAVEFORM 0
#define DEFAULT_STEREO 0

#define DEFAULT_BOTTOMBARHEIGHT 30
#define DEFAULT_LEDBAR 3

typedef struct _BARINFO
{
	int width;
	int height;
	int x;
}BARINFO;

extern HRESULT initializeSettingsFile(HWND hwnd);
extern void uninitializeSettingsFile();

extern void UninitializeMemory();
extern HRESULT InitializeMemory();

extern HRESULT readSettings();
extern void writeSettings();


extern void ResizeBars(HWND hwnd, BARINFO* bars, int size, int channel, int dostereo);

extern BARINFO* barLeft;
extern BARINFO* barRight;
extern BARINFO* waveBar;

extern double* pGradients;

extern INT16* audioBufferLeft;
extern INT16* audioBufferRight;

extern float zoom;
extern LRESULT colorSel;

extern int barCount;

extern bool dofft;
extern bool background;
extern bool gradient;
extern bool ignoreSerial;
extern bool circle;
extern bool waveform;
extern bool stereo;

extern bool redrawAll;

extern const int bottomBarHeihgt;
extern const int led_bar;