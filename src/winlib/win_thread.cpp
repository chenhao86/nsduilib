#include <win_thread.h>
#include <win_err.h>


#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

typedef struct __win_thread {
    HANDLE m_exitevt;
    HANDLE m_thrhd;
    thread_func_t m_lpfunc;
    void* m_param;
    int m_exitcode;
    int m_exited;
} win_thread_t, *pwin_thread_t;

void __notify_thread_exit(pwin_thread_t pthr)
{
    BOOL bret;
    int ret;
    int cnt = 0;
    if (pthr->m_exited == 0) {
        ASSERT_IF(pthr->m_exitevt != NULL);
        while (pthr->m_exited == 0) {
            bret = SetEvent(pthr->m_exitevt);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("set thread %p event error[%d]", pthr->m_thrhd, ret);
            }
            bret = SwitchToThread();
            if (!bret) {
                SleepEx(1, TRUE);
            }
            cnt ++;
            if (cnt > 10) {
                ERROR_INFO("[%d]thread %p still alive", cnt, pthr->m_thrhd);
            }
        }
    }
    if (pthr->m_thrhd) {
        CloseHandle(pthr->m_thrhd);
        pthr->m_thrhd = NULL;
    }
    if (pthr->m_exitevt) {
        CloseHandle(pthr->m_exitevt);
        pthr->m_exitevt = NULL;
    }
    return;
}


void __free_win_thread(pwin_thread_t* ppthr)
{
    pwin_thread_t pthr;
    if (ppthr && *ppthr) {
        pthr = *ppthr;
        __notify_thread_exit(pthr);
        ASSERT_IF(pthr->m_exitevt == NULL);
        ASSERT_IF(pthr->m_thrhd == NULL);
        free(pthr);
        *ppthr = NULL;
    }
    return ;
}

DWORD WINAPI __win_thread_inner(void* args)
{
    pwin_thread_t pthr = (pwin_thread_t) args;
    pthr->m_exitcode = pthr->m_lpfunc(pthr->m_param, pthr->m_exitevt);
    pthr->m_exited = 1;
    return 0;
}

pwin_thread_t __alloc_win_thread(thread_func_t pfunc, void* param, int started)
{
    pwin_thread_t pthr = NULL;
    int ret;
    DWORD thrid = 0;

    pthr = (pwin_thread_t)malloc(sizeof(*pthr));
    if (pthr == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    memset(pthr, 0, sizeof(*pthr));
    /*we make sure the exited not start*/
    pthr->m_exited = 1;
    pthr->m_exitcode = 0;

    pthr->m_exitevt = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pthr->m_exitevt == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not create exit notify event error[%d]", ret);
        goto fail;
    }

    pthr->m_param = param;
    pthr->m_lpfunc = pfunc;

    pthr->m_exited = 0;
    if (started) {
        pthr->m_thrhd = CreateThread(NULL, 0, __win_thread_inner, pthr, 0, &thrid);
    } else {
        pthr->m_thrhd = CreateThread(NULL, 0, __win_thread_inner, pthr, CREATE_SUSPENDED, &thrid);
    }

    if (pthr->m_thrhd == NULL ||
            pthr->m_thrhd == INVALID_HANDLE_VALUE) {
        GETERRNO(ret);
        pthr->m_exited = 1;
        pthr->m_exitcode = ret;
        goto fail;
    }

    if (started == 0) {
        /*because not started , so we not make exited*/
        pthr->m_exited = 1;
    }

    return pthr;
fail:
    __free_win_thread(&pthr);
    SETERRNO(ret);
    return NULL;
}

int create_thread(thread_func_t pfunc, void* param, int started, void** ppthr)
{
    pwin_thread_t pthr = NULL;
    int ret;
    if (pfunc == NULL) {
        if (ppthr && *ppthr) {
            pthr = (pwin_thread_t) * ppthr;
            __free_win_thread(&pthr);
            *ppthr = NULL;
        }
        return 0;
    }

    if (ppthr == NULL || *ppthr != NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    pthr = __alloc_win_thread(pfunc, param, started);
    if (pthr == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    *ppthr = pthr;
    return 1;
fail:
    __free_win_thread(&pthr);
    SETERRNO(ret);
    return ret;
}

int resume_thread(void* pthr1)
{
    pwin_thread_t pthr = (pwin_thread_t) pthr1;
    int ret;
    DWORD dret;
    int resumed = 0;
    if (pthr == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pthr->m_exited == 1) {
        if (pthr->m_thrhd != NULL) {
            /*we make sure this is resumed*/
            pthr->m_exited = 0;
            dret = ResumeThread(pthr->m_thrhd);
            if (dret == (DWORD) - 1)	 {
                GETERRNO(ret);
                /*that is already not resumed*/
                pthr->m_exited = 1;
                ERROR_INFO("resume thread %p error[%d]", pthr->m_thrhd, ret);
                goto fail;
            }
            resumed = 1;
        }
    }
    return resumed;
fail:
    SETERRNO(ret);
    return ret;
}

int stop_thread(void* pthr1, int* pexitcode)
{
    pwin_thread_t pthr = (pwin_thread_t) pthr1;
    int ret;
    if (pthr == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }
    __notify_thread_exit(pthr);
    if (pexitcode) {
        *pexitcode = pthr->m_exitcode;
    }
    return pthr->m_exited;
fail:
    SETERRNO(ret);
    return ret;
}

int is_exited_thread(void* pthr1, int *pexitcode)
{
    pwin_thread_t pthr = (pwin_thread_t) pthr1;
    int ret;
    int stopped = 0;
    if (pthr == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pthr->m_exited) {
        stopped = 1;
        if (pexitcode) {
            *pexitcode = pthr->m_exitcode;
        }
    }
    return stopped;
fail:
    SETERRNO(ret);
    return ret;
}

void free_thread(void** ppthr1)
{
	pwin_thread_t pthr;
	if (ppthr1 && *ppthr1) {
		pthr = (pwin_thread_t) *ppthr1;
		__free_win_thread(&pthr);
		*ppthr1 = NULL;
	}
	return ;
}