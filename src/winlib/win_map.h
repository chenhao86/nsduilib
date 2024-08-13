#ifndef __WIN_WINLIB_MAP_H_0A8F56E4926A88B6BBD59FDAF949B028__
#define __WIN_WINLIB_MAP_H_0A8F56E4926A88B6BBD59FDAF949B028__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <win_types.h>


#define   WINLIB_MAP_FILE_WRITE               0x4
#define   WINLIB_MAP_FILE_READ                0x2
#define   WINLIB_MAP_FILE_EXEC                0x1
#define   WINLIB_MAP_CREATE                   0x80
#define   WINLIB_MAP_FORALL                   0x100
#define   WINLIB_MAP_GLOBAL                   0x200

#define   WINLIB_MAP_FILE_FLAGS               (WINLIB_MAP_FILE_WRITE | WINLIB_MAP_FILE_READ | WINLIB_MAP_FILE_EXEC | WINLIB_MAP_CREATE | WINLIB_MAP_FORALL | WINLIB_MAP_GLOBAL)

WINLIB_API int map_buffer(char* name,int flag,uint64_t size,void** ppmap);
WINLIB_API int map_file(char* name, int flag, uint64_t* psize,void** ppmap);
WINLIB_API int write_buffer(void* pmap1, uint64_t offset, void* pbuf, int size);
WINLIB_API int read_buffer(void* pmap1, uint64_t offset, void* pbuf, int size);
WINLIB_API void unmap_buffer(void** ppmap);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_WINLIB_MAP_H_0A8F56E4926A88B6BBD59FDAF949B028__ */
