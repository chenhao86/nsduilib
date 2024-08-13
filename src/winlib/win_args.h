#ifndef __WIN_ARGS_H__
#define __WIN_ARGS_H__


#include <tchar.h>
#include <win_types.h>

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

WINLIB_API void free_args(char*** pppargs);

#ifdef _UNICODE
#define  copy_args      copy_args_w
#else
#define  copy_args      copy_args_a
#endif

WINLIB_API char** copy_args(int argc,TCHAR *argv[]);
WINLIB_API char** copy_argv(char** argv);
WINLIB_API void free_argv(char*** pppargv);
WINLIB_API int  parse_number(char* str,uint64_t *pnum,char** ppend);
WINLIB_API int  parse_int(char* str,int64_t* pnum,char**ppend);
WINLIB_API int  parse_long_double(char* str, long double *pdbl, char** ppend);
WINLIB_API int  split_argv(char* pcmd,char***pppargv, int* pargc);

#ifdef __cplusplus
};
#endif

#endif /*__WIN_ARGS_H__*/