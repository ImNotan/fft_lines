#include <Windows.h>
#include <stdbool.h>

#define MAX_BARCOUNT 1000
#define MIN_BARCOUNT 100
#define MAX_ZOOM 0.0001f
#define MIN_ZOOM 0.000001f
#define NUMOF_GRADIENTS 6

#define DEFAULT_BARCOUNT 500
#define DEFAULT_ZOOM 0.0001f
#define DEFAULT_COLORSEL 0
#define DEFAULT_BORDER 0
#define DEFAULT_BACKGROUND 1
#define DEFAULT_GRADIENT 1
#define DEFAULT_IGNORESERIAL 1
#define DEFAULT_CIRCLE 0
#define DEFAULT_WAVEFORM 0

#define DEFAULT_BOTTOMBARHEIGHT 30
#define DEFAULT_LEDBAR 4

extern HWND SettingsDlg;
extern LRESULT CALLBACK SettingsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern void initializeSettingsFile(HWND hwnd);
extern void setColor();
extern void readSettings();
extern void writeSettings();
extern void ResizeBars(HWND hwnd);

typedef struct _BARINFO
{
	int width;
	int height;
	int x;
}BARINFO;

extern float zoom;
extern LRESULT colorSel;

extern int barCount;
extern BARINFO* bar;

extern bool border;
extern bool background;
extern bool gradient;
extern bool ignoreSerial;
extern bool circle;
extern bool waveform;

extern const int bottomBarHeihgt;
extern const int led_bar;

extern double* pGradients;