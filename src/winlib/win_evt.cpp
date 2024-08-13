#include <win_evt.h>
#include <win_err.h>
#include <win_uniansi.h>
#include <win_strop.h>

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

static HANDLE st_hevent = NULL;
static int st_evtlevel = BASE_EVENT_ERROR;

//
// MessageId: DATABASE_CATEGORY
//
// MessageText:
//
// Database Events
//
#define DATABASE_CATEGORY                ((WORD)0x00000002L)
// The following are message definitions.
//
// MessageId: MSG_INVALID_COMMAND
//
// MessageText:
//
// The command is not valid.
//
#define MSG_EVENT_LOG              ((DWORD)0xC0020100L)


int init_event_log(int level, char* name)
{
    int ret;
    TCHAR* ptname = NULL;
    int tnamesize = 0;
    if (name == NULL || (level < BASE_EVENT_FATAL || level > BASE_EVENT_TRACE)) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (st_hevent != NULL) {
        CloseHandle(st_hevent);
        st_hevent = NULL;
    }

    ret = AnsiToTchar(name, &ptname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    st_hevent = RegisterEventSource(NULL, ptname);
    if (st_hevent == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not register source [%s] error[%d]", name , ret);
        goto fail;
    }


    st_evtlevel = level;
    AnsiToTchar(NULL, &ptname, &tnamesize);
    return 0;

fail:
    AnsiToTchar(NULL, &ptname, &tnamesize);
    if (st_hevent != NULL) {
        CloseHandle(st_hevent);
        st_hevent = NULL;
    }
    st_evtlevel = BASE_EVENT_ERROR;
    SETERRNO(ret);
    return ret;
}

void close_event_log(void)
{
    if (st_hevent != NULL) {
        CloseHandle(st_hevent);
        st_hevent = NULL;
    }
    st_evtlevel = BASE_EVENT_ERROR;
    return ;
}

int log_event(int level, const char* file , int lineno, char* fmt, ...)
{
    int ret;
    char* evtbuf = NULL;
    int evtsize = 0;
    int evtlen = 0;
    TCHAR* ptevt = NULL;
    int tevtsize = 0;
    BOOL bret;
    TCHAR* pinsertstr[2] = {NULL, NULL};
    va_list ap;

    ret = snprintf_safe(&evtbuf, &evtsize, "[%ld:0x%lx][%s:%d] ",GetCurrentProcessId(),GetCurrentProcessId(), file, lineno);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    va_start(ap, fmt);
    ret = append_vsnprintf_safe(&evtbuf, &evtsize, fmt, ap);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    evtlen = ret;

    ret = AnsiToTchar(evtbuf, &ptevt, &tevtsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }


    if (st_hevent == NULL ||
            level <= st_evtlevel) {
        pinsertstr[0] = ptevt;
#if defined(UNICODE) || defined(_UNICODE)
        bret = ReportEventW(st_hevent, EVENTLOG_ERROR_TYPE, DATABASE_CATEGORY, MSG_EVENT_LOG, NULL, 1, 0, (LPCWSTR *)pinsertstr, NULL);
#else
        bret = ReportEventA(st_hevent, EVENTLOG_ERROR_TYPE, DATABASE_CATEGORY, MSG_EVENT_LOG, NULL, 1, 0, (LPCSTR *)pinsertstr, NULL);
#endif
        if (!bret) {
            GETERRNO(ret);
            ERROR_INFO("can not report event [%s] error[%d]", evtbuf, ret);
            goto fail;
        }
        DEBUG_INFO("insert event [%s]", evtbuf);
    } else {
    	DEBUG_INFO("not level for [%s]", evtbuf);
    }



    AnsiToTchar(NULL, &ptevt, &tevtsize);
    snprintf_safe(&evtbuf, &evtsize, NULL);
    return evtlen;
fail:
    AnsiToTchar(NULL, &ptevt, &tevtsize);
    snprintf_safe(&evtbuf, &evtsize, NULL);
    SETERRNO(ret);
    return ret;
}

HANDLE open_event(char* name, int created)
{
    TCHAR* ptname= NULL;
    int tnamesize=0;
    HANDLE evt=NULL;
    int ret;

    ret = AnsiToTchar(name,&ptname,&tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    if (created) {
        SETERRNO(0);
        evt = CreateEvent(NULL,TRUE,FALSE,ptname);
        if (evt != NULL) {
            GETERRNO(ret);
            if (ret == -ERROR_ALREADY_EXISTS) {
                goto fail;
            }
        }
    } else {
        evt = OpenEvent(EVENT_ALL_ACCESS,FALSE,ptname);
    }
    if (evt == NULL || evt == INVALID_HANDLE_VALUE) {
        GETERRNO(ret);
        evt = NULL;
        goto fail;
    }

    //DEBUG_INFO("%s [%s] [%p]", created ? "CreateEvent" : "OpenEvent", name, evt);
    AnsiToTchar(NULL,&ptname,&tnamesize);
    return evt;
fail:
    if (evt != NULL) {
        CloseHandle(evt);
    }
    evt = NULL;
    AnsiToTchar(NULL,&ptname,&tnamesize);
    SETERRNO(ret);
    return NULL;
}


HANDLE get_or_create_event(char* name)
{
    int ret;
    HANDLE evt = NULL;

    evt = open_event(name,0);
    if (evt == NULL) {
        evt = open_event(name,1);
    }
    if (evt == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    return evt;
fail:
    SETERRNO(ret);
    return NULL;
}

HANDLE open_mutex(char* name,int created)
{
    TCHAR* ptname= NULL;
    int tnamesize=0;
    HANDLE mux=NULL;
    int ret;

    ret = AnsiToTchar(name,&ptname,&tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    if (created) {
        SETERRNO(0);
        mux = CreateMutex(NULL,FALSE,ptname);
        if (mux != NULL) {
            GETERRNO(ret);
            if (ret == -ERROR_ALREADY_EXISTS) {
                goto fail;
            }
        }
    } else {
        mux = OpenMutex(SYNCHRONIZE,FALSE,ptname);
    }
    if (mux == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    //DEBUG_INFO("%s [%s] [%p]", created ? "CreateMutex" : "OpenMutex" , name, mux);

    AnsiToTchar(NULL,&ptname,&tnamesize);
    return mux;
fail:
    if (mux != NULL) {
        CloseHandle(mux);
    }
    mux = NULL;
    AnsiToTchar(NULL,&ptname,&tnamesize);
    SETERRNO(ret);
    return NULL;    
}

HANDLE get_or_create_mutex(char* name)
{
    HANDLE mux=NULL;
    int ret;

    mux = open_mutex(name,1);
    if (mux == NULL) {
        mux = open_mutex(name,0);
    }
    if (mux == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    return mux;
fail:
    SETERRNO(ret);
    return NULL;
}
