#include <Windows.h>
#include <stdbool.h>

extern bool doSerial;

extern int InitializeSerial(HWND hwnd);

extern int CloseSerial(HWND hwnd);

extern int WriteSerial(char byte_to_send[1], HWND hwnd);