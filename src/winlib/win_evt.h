#ifndef __WIN_EVT_H_AC2143B43AEF32FD700FE78D3B8C75BB__
#define __WIN_EVT_H_AC2143B43AEF32FD700FE78D3B8C75BB__


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#define  BASE_EVENT_FATAL        0
#define  BASE_EVENT_ERROR        10
#define  BASE_EVENT_WARN         20
#define  BASE_EVENT_INFO         30
#define  BASE_EVENT_DEBUG        40
#define  BASE_EVENT_TRACE        50


WINLIB_API int init_event_log(int level,char* name);
WINLIB_API int log_event(int level,const char* file ,int lineno,char* fmt,...);
WINLIB_API void close_event_log(void);

WINLIB_API HANDLE get_or_create_event(char* name);
WINLIB_API HANDLE get_or_create_mutex(char* name);
WINLIB_API HANDLE open_event(char* name, int created);
WINLIB_API HANDLE open_mutex(char* name,int created);


#define DEBUG_LOG_EVENT(...)  log_event(BASE_EVENT_DEBUG,__FILE__,__LINE__,__VA_ARGS__)
#define ERROR_LOG_EVENT(...)  log_event(BASE_EVENT_ERROR,__FILE__,__LINE__,__VA_ARGS__)


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_EVT_H_AC2143B43AEF32FD700FE78D3B8C75BB__ */
