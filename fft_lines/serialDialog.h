#include <Windows.h>
#include <stdbool.h>

extern LRESULT CALLBACK SerialDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

extern bool doSerial;

extern HWND hwndSerialDialog;

extern int InitializeSerial(HWND hwnd);

extern int CloseSerial(HWND hwnd);

extern int WriteSerial(char byte_to_send[1], HWND hwnd);