#ifndef __WIN_USER_H_2E2D3253C4E4E70BB559BE70960D094D__
#define __WIN_USER_H_2E2D3253C4E4E70BB559BE70960D094D__


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

typedef struct __user_info {
	char m_name[256];
	char m_sid[256];
} user_info_t, *puser_info_t;




#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WINLIB_API int user_change_password(char* user, char* oldpassword,char* newpassword);
WINLIB_API int get_user_info(int freed,HANDLE exithd,puser_info_t* ppuser,int* psize);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_USER_H_2E2D3253C4E4E70BB559BE70960D094D__ */
