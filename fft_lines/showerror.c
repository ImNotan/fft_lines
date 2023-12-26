#include <Windows.h>
#include <stdbool.h>

#include "showerror.h"

BOOL ShowError(HRESULT hres, HWND hwnd)
{
	if (hres == S_OK)
		return 0;
	else
	{
		wchar_t buffer[9];
		wsprintf(&buffer, L"%x", hres);
		MessageBoxW(hwnd, buffer, L"ERROR", MB_OK | MB_ICONERROR | MB_APPLMODAL);
		return 1;
	}
}