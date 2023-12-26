#include <Windows.h>
#include <stdbool.h>

extern HWND SettingsDlg;
extern BOOL CALLBACK SettingsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern void setColor(float floatBetweenZeoAndOne, int* rgb);
extern void initializeSettingsFile(HWND hwnd);
extern void readSettings();
extern void writeSettings();

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

extern HBRUSH barBrush[255];
extern unsigned int rgb[3];


extern bool border;
extern bool background;
extern bool gradient;
extern bool ignoreSerial;

extern const int bottomBarHeihgt;
extern const int led_bar;