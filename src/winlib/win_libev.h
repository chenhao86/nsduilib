#ifndef __WIN_LIBEV_H_EEE27640B98D24EC97C7B91D7101EBFA__
#define __WIN_LIBEV_H_EEE27640B98D24EC97C7B91D7101EBFA__


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


#include <win_types.h>

#define INFINIT_TIME         ((uint32_t)0xffffffff)

typedef enum {
	normal_event = 0,
	timer_event,
	timeout_event,
	abandon_event,
	failed_event ,
} libev_enum_event_t;


typedef int (*libev_evt_callback_t)(HANDLE hd,libev_enum_event_t event,void* pevmain,void* args);
typedef int (*libev_timer_callback_t)(uint64_t guid,libev_enum_event_t event,void* pevmain,void* args);



/*****************************************
*  return value negative is the 
*****************************************/
WINLIB_API int libev_insert_timer(void* pevmain,uint64_t* pguid,libev_timer_callback_t pfunc,void* args,uint32_t timemills,int conti);
WINLIB_API int libev_insert_handle(void* pevmain,HANDLE hd, libev_evt_callback_t pfunc,void* args);
WINLIB_API int libev_remove_timer(void* pevmain,uint64_t guid);
WINLIB_API int libev_remove_handle(void* pevmain,HANDLE hd);

WINLIB_API int libev_winev_loop(void* pevmain);
WINLIB_API void* libev_init_winev();
WINLIB_API void libev_break_winev_loop(void* pevmain);

WINLIB_API void libev_free_winev(void** ppevmain);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_LIBEV_H_EEE27640B98D24EC97C7B91D7101EBFA__ */
