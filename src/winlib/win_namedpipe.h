#ifndef __WIN_NAMEDPIPE_H_2903D1FEC3DF4936CEE08FAEEB065689__
#define __WIN_NAMEDPIPE_H_2903D1FEC3DF4936CEE08FAEEB065689__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


WINLIB_API void* bind_namedpipe(char* name);
WINLIB_API void* connect_namedpipe(char* name);
WINLIB_API void* connect_namedpipe_timeout(char* name,int timeout);
WINLIB_API void close_namedpipe(void** ppnp);
WINLIB_API HANDLE get_namedpipe_rdevt(void* pnp);
WINLIB_API HANDLE get_namedpipe_wrevt(void* pnp);
WINLIB_API HANDLE get_namedpipe_connevt(void* pnp);
WINLIB_API int read_namedpipe(void* pnp,char* buffer,int bufsize);
WINLIB_API int write_namedpipe(void* pnp,char* buffer,int bufsize);
WINLIB_API int complete_namedpipe_rdpending(void* pnp);
WINLIB_API int complete_namedpipe_wrpending(void* pnp);
WINLIB_API int complete_namedpipe_connpending(void* pnp);
WINLIB_API int get_namedpipe_rdstate(void* pnp);
WINLIB_API int get_namedpipe_wrstate(void* pnp);
WINLIB_API int get_namedpipe_connstate(void* pnp);
WINLIB_API int cancel_namedpipe_connect(void* pnp);
WINLIB_API int cancel_namedpipe_read(void* pnp);
WINLIB_API int cancel_namedpipe_write(void* pnp);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_NAMEDPIPE_H_2903D1FEC3DF4936CEE08FAEEB065689__ */
