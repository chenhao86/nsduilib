#ifndef __WIN_SVC_H_76534A2CAA1D59EFE44080E15F81572C__
#define __WIN_SVC_H_76534A2CAA1D59EFE44080E15F81572C__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#define  SVC_START_ON_UNKNOWN         0
#define  SVC_START_ON_BOOT            1
#define  SVC_START_ON_SYSTEM          2
#define  SVC_START_ON_AUTO            3
#define  SVC_START_ON_DEMAND          4
#define  SVC_START_ON_DISABLED        5


#define  SVC_STATE_UNKNOWN            0
#define  SVC_STATE_STOPPED            1
#define  SVC_STATE_START_PENDING      2
#define  SVC_STATE_RUNNING            3
#define  SVC_STATE_STOP_PENDING       4
#define  SVC_STATE_PAUSED             5
#define  SVC_STATE_PAUSE_PENDING      6
#define  SVC_STATE_CONTINUE_PENDING   7


WINLIB_API int is_service_exist(const char* name);
WINLIB_API int is_service_running(const char* name);
WINLIB_API int is_service_stopped(const char* name);
WINLIB_API int get_service_start_mode(const char* name);
WINLIB_API int service_running_mode(const char* name);


WINLIB_API int stop_service(const char* name,int mills);
WINLIB_API int start_service(const char* name, int mills);
WINLIB_API int config_service_start_mode(const char* name, int startmode);

WINLIB_API int svc_init_mode(char* svcname,LPHANDLER_FUNCTION_EX pFunc, void* puserdata);
WINLIB_API int svc_report_mode(DWORD mode,DWORD time);
WINLIB_API void svc_close_mode();
WINLIB_API int svc_start(char* svcname, LPSERVICE_MAIN_FUNCTION pProc);

WINLIB_API int create_service(const char* name, const char* desc,const char* binpath, int startmode);
WINLIB_API int create_driver(const char* name, const char* desc,const char* binpath, int startmode);
WINLIB_API int delete_service(const char* name);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_SVC_H_76534A2CAA1D59EFE44080E15F81572C__ */
