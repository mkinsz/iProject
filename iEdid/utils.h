#ifndef UTILS
#define UTILS

#include <windows.h>
#include <tchar.h>
#include "atlbase.h"

static inline void OutputLog(const char* format, ...)
{
	char buf[1024] = { 0 };
	va_list args;
	va_start(args, format);
	_vsnprintf_s(buf, sizeof(buf) - 1, format, args);
	//_vstprintf(buf, format, args);
	va_end(args);
	OutputDebugString(CA2W(buf));
}

#endif // UTILS