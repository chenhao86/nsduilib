#ifndef __WIN_THREAD_H_C4B31E86DE418831D1B89202221055D3__
#define __WIN_THREAD_H_C4B31E86DE418831D1B89202221055D3__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


/*event to notify exit*/
typedef int (*thread_func_t)(void* param,HANDLE extevt);

WINLIB_API int create_thread(thread_func_t pfunc,void* param, int started,void** ppthr);
WINLIB_API int resume_thread(void* pthr);
WINLIB_API int stop_thread(void* pthr,int* pexitcode);
WINLIB_API int is_exited_thread(void* pthr,int *pexitcode);
WINLIB_API void free_thread(void** ppthr);



#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_THREAD_H_C4B31E86DE418831D1B89202221055D3__ */
