#ifndef __WIN_WINDOW_H_C408B03B8AE4FAC8777BFD55BB31AA2E__
#define __WIN_WINDOW_H_C408B03B8AE4FAC8777BFD55BB31AA2E__


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*win_enum_func_t)(HWND hwnd,void* param);

WINLIB_API int get_win_handle(win_enum_func_t pcallback,void* param);
WINLIB_API int get_win_handle_by_classname(const char* typeclass,int pid,HWND *ppwnd[],int *pwinsize);
WINLIB_API int get_window_from_pid(int pid, HWND** pphdl,int *psize);
WINLIB_API int get_window_text(HWND hd, char** pptext,int *psize);

#ifdef __cplusplus
};
#endif


#endif /* __WIN_WINDOW_H_C408B03B8AE4FAC8777BFD55BB31AA2E__ */
