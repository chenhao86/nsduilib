#ifndef __WIN_TIME_H__
#define __WIN_TIME_H__

#include <win_types.h>

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

WINLIB_API uint64_t get_current_ticks(void);
WINLIB_API uint64_t get_ms_from_epoch(void);
WINLIB_API int need_wait_times(uint64_t sticks, uint64_t eticks, int timeout);
WINLIB_API int sleep_mill(int mills);
WINLIB_API int get_last_bootuptime(HANDLE hevt,uint64_t *pboottime);
WINLIB_API int tm_to_str(struct tm* ptm, char** ppstr, int *psize);
WINLIB_API int tm_from_str(char* str, struct tm* ptm);
WINLIB_API int tm_to_time(struct tm* ptm, time_t* ptime);
WINLIB_API int time_to_tm(time_t* ptime,struct tm *ptm);

#ifdef __cplusplus
};
#endif


#endif /*__WIN_TIME_H__*/