#pragma once
#ifndef _CONFIG_VC6_H_
#define _CONFIG_VC6_H_

#if _MSC_VER < 1300

#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))

#ifdef _UNICODE
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"
#define strcpy_s StringCchCopyA
#define strcat_s StringCchCatA
#define sprintf_s StringCchPrintfA
#else
#endif // _UNICODE

#define wcscat_s StringCchCatW
#define swprintf_s StringCchPrintfW
#define wcscpy_s StringCchCopyW

#define __debugbreak() DebugBreak()

#endif // _MSC_VER < 1200

#endif // _CONFIG_VC6_H_