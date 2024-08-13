#ifndef __WIN_REGEX_H_8C4DAE24E9E10F618C042648408F59EB__
#define __WIN_REGEX_H_8C4DAE24E9E10F618C042648408F59EB__


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#define  REGEX_NONE                     0
#define  REGEX_IGNORE_CASE              1

WINLIB_API int regex_compile(const char* restr, int flags, void**ppreg);
WINLIB_API int regex_exec(void* preg,const char* instr, int** ppstartpos, int **ppendpos, int * psize);
WINLIB_API int regex_split(void* preg, const char* instr, int** ppstartpos, int **ppendpos, int *psize);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_REGEX_H_8C4DAE24E9E10F618C042648408F59EB__ */
