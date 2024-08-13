#ifndef __WIN_INNER_H__
#define __WIN_INNER_H__

#ifndef __WINLIB_INNER_INCLUDE__
#error "must included in the winlib header files"
#endif

#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4820)
#pragma warning(disable:4530)
#pragma warning(disable:4577)

#include <Windows.h>

#pragma warning(pop)

#if defined(WINLIB_DLL_IMPORT)
#define WINLIB_API  __declspec(dllimport)
#elif defined(WINLIB_DLL_EXPORT)
#define WINLIB_API __declspec(dllexport) 
#else
#define WINLIB_API
#endif


#endif /*__WIN_INNER_H__*/