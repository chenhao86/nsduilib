#ifndef __WIN_VER_H_95648CD572068237E6C9AD8A23E54BE3__
#define __WIN_VER_H_95648CD572068237E6C9AD8A23E54BE3__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WINLIB_API int is_win7(void);
WINLIB_API int is_win10(void);
WINLIB_API int is_winserver_2019(void);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_VER_H_95648CD572068237E6C9AD8A23E54BE3__ */
