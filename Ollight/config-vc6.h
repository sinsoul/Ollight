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


#include <crtdbg.h>
#include <errno.h>

template <typename charT>
inline
int _tcsncpy_s(charT* dst, size_t size, const charT* src, size_t count)
{
    charT *p;
    size_t available;

    if (count == 0 && dst == NULL && size == 0)
    {
        /* this case is allowed; nothing to do */
        return 0;
    }

    /* validation section */
	_ASSERTE(dst != NULL && size > 0);
	if(dst == NULL || size <= 0)
	{
		errno = EINVAL;
		return EINVAL;
	}

    if (count == 0)
    {
        /* notice that the source string pointer can be NULL in this case */
        *dst = 0;
        return 0;
    }
	if(src == NULL)
	{
		*dst = 0;
		_ASSERTE(src != NULL);
		if(src == NULL)
		{
			errno = EINVAL;
			return EINVAL;
		}
	}

    p = dst;
    available = size;
    if (count == (size_t)-1)
    {
        while ((*p++ = *src++) != 0 && --available > 0)
        {
        }
    }
    else
    {
        while ((*p++ = *src++) != 0 && --available > 0 && --count > 0)
        {
        }
        if (count == 0)
        {
            *p = 0;
        }
    }

    if (available == 0)
    {
        if (count == (size_t)-1)
        {
            dst[size - 1] = 0;
            return 80;
        }
        *dst = 0;
		_ASSERTE(!L"Buffer is too small");
		errno = ERANGE;
		return ERANGE;

    }
    return 0;
}

#endif // _MSC_VER < 1200

#endif // _CONFIG_VC6_H_
