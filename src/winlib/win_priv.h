#ifndef __WIN_PRIV_H_42F3970B7071BE6584FDEB901119D183__
#define __WIN_PRIV_H_42F3970B7071BE6584FDEB901119D183__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


WINLIB_API int enable_security_priv(void);
WINLIB_API int disable_security_priv(void);
WINLIB_API int is_security_priv(void);

WINLIB_API int enable_takeown_priv(void);
WINLIB_API int disable_takeown_priv(void);
WINLIB_API int is_takeown_priv(void);


WINLIB_API int enable_restore_priv(void);
WINLIB_API int disable_restore_priv(void);
WINLIB_API int is_restore_priv(void);

WINLIB_API int enable_backup_priv(void);
WINLIB_API int disable_backup_priv(void);
WINLIB_API int is_backup_priv(void);

WINLIB_API int enable_impersonate_priv(void);
WINLIB_API int disable_impersonate_priv(void);
WINLIB_API int is_impersonate_priv(void);

WINLIB_API int enable_audit_priv(void);
WINLIB_API int disable_audit_priv(void);
WINLIB_API int is_audit_priv(void);

WINLIB_API int enable_debug_priv(void);
WINLIB_API int disable_debug_priv(void);
WINLIB_API int is_debug_priv(void);

WINLIB_API int enable_tcb_priv(void);
WINLIB_API int disable_tcb_priv(void);
WINLIB_API int is_tcb_priv(void);

WINLIB_API int enable_token_debug_priv(HANDLE htoken);
WINLIB_API int disable_token_debug_priv(HANDLE htoken);

WINLIB_API int enable_token_tcb_priv(HANDLE htoken);
WINLIB_API int disable_token_tcb_priv(HANDLE htoken);

WINLIB_API int enable_shutdown_priv(void);
WINLIB_API int disable_shutdown_priv(void);
WINLIB_API int is_shutdown_priv(void);

WINLIB_API int enable_create_global_priv(void);
WINLIB_API int disable_create_global_priv(void);
WINLIB_API int is_create_global_priv(void);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_PRIV_H_42F3970B7071BE6584FDEB901119D183__ */
