#ifndef __WIN_ENVOP_H_65CC41A9CD077A8759BB97D7F81A5D94__
#define __WIN_ENVOP_H_65CC41A9CD077A8759BB97D7F81A5D94__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <win_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  COMPUTER_NAME_NONE         0
#define  COMPUTER_NAME_DNS          1
#define  COMPUTER_NAME_NETBIOS      2
#define  COMPUTER_NAME_PHYS         4

#define   WIN32_ARCH    1
#define   WIN64_ARCH    2


#define  EN_US_CODEPAGE             437

#define  MAX_INFO_CLASS             0xffffffff

WINLIB_API int get_env_variable(char* envvar,char** ppenvval,int* pvalsize);
WINLIB_API int get_computer_name(int type, char** ppname,int *pnamesize);
WINLIB_API int set_computer_name(int type,char* pname);
WINLIB_API int get_codepage(void);
WINLIB_API int set_codepage(int cp);
WINLIB_API int get_current_user(int freed,char** ppuser,int *psize);
WINLIB_API int get_executable_wholepath(int freed,char** ppath, int *psize);
WINLIB_API int get_executable_dirname(int freed,char** ppath, int *psize);
WINLIB_API int get_desktop_session(void);
WINLIB_API int user_password_ok(const char* user, const char* password);

WINLIB_API int win_arch_type();

WINLIB_API int init_nt_envop_funcs(void);
WINLIB_API void fini_nt_envop_funcs(void);

#ifdef __cplusplus
};
#endif


#endif /* __WIN_ENVOP_H_65CC41A9CD077A8759BB97D7F81A5D94__ */
