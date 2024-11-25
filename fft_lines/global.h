#include <windows.h>
extern HWND globalhwnd;
#define N 2048
#define WM_ERROR 0x8001

extern LARGE_INTEGER FrameStartingTime, FrameEndingTime, ElapsedFrameTime;