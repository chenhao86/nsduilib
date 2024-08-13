#ifndef __WIN_MEMOP_H_C35697F8AEE1B887008F91E93AA41C68__
#define __WIN_MEMOP_H_C35697F8AEE1B887008F91E93AA41C68__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


#define  MEM_EXECUTE           1
#define  MEM_READ              2
#define  MEM_WRITE             4


#ifdef _M_X64
#define  MEM_PAGE_SIZE         (1ULL << 12)
#define  MEM_ALIGN_MASK        (MEM_PAGE_SIZE - 1)
#define  MEM_ALIGN_BITS        (~(MEM_ALIGN_MASK))
#elif defined(_M_IX86)
#define  MEM_PAGE_SIZE         (1UL << 12)
#define  MEM_ALIGN_MASK        (MEM_PAGE_SIZE - 1)
#define  MEM_ALIGN_BITS        (~(MEM_ALIGN_MASK))
#else
#error "not supported "
#endif


WINLIB_API int memory_valid(void* ptr,int memsize);
WINLIB_API int memory_set_mode(void* ptr,int memsize,int mode,int *porigmode);
WINLIB_API int memory_reset_mode(void* ptr,int memsize,int origmode);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_MEMOP_H_C35697F8AEE1B887008F91E93AA41C68__ */
