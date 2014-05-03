#pragma once
#ifndef _CONFIG_VC6_H_
#define _CONFIG_VC6_H_

#if _MSC_VER < 1300

#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#define __debugbreak() DebugBreak()

#endif // _MSC_VER < 1200

#endif // _CONFIG_VC6_H_