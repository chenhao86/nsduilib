#include <win_namedpipe.h>
#include <win_err.h>
#include <win_types.h>
#include <win_uniansi.h>


#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#define   NAMED_PIPE_MAGIC             0x33219

typedef struct __named_pipe {
    uint32_t m_magic;
    int m_servermode;
    int m_connpending;
    int m_rdpending;
    int m_rdleft;
    int m_wrpending;
    int m_wrleft;
    int m_reserv1;
    char* m_name;
    HANDLE m_hpipe;
    HANDLE m_connevt;
    HANDLE m_rdevt;
    HANDLE m_wrevt;
    OVERLAPPED m_connov;
    OVERLAPPED m_rdov;
    OVERLAPPED m_wrov;
} named_pipe_t, *pnamed_pipe_t;


void __free_namedpipe(pnamed_pipe_t *ppnp)
{
    BOOL bret;
    int ret;
    if (ppnp && *ppnp) {
        pnamed_pipe_t pnp = *ppnp;

        if (pnp->m_connpending) {
            ASSERT_IF(pnp->m_hpipe != NULL && pnp->m_hpipe != INVALID_HANDLE_VALUE);
            bret = CancelIoEx(pnp->m_hpipe, &(pnp->m_connov));
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("cancel [%s].connpending error[%d]", pnp->m_name , ret);
            }
            pnp->m_connpending = 0;
        }

        if (pnp->m_rdpending) {
            ASSERT_IF(pnp->m_hpipe != NULL && pnp->m_hpipe != INVALID_HANDLE_VALUE);
            bret = CancelIoEx(pnp->m_hpipe, &(pnp->m_rdov));
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("can not cancel pending [%s] read", pnp->m_name);
            }
            pnp->m_rdpending = 0;
            pnp->m_rdleft = 0;
        }

        if (pnp->m_wrpending) {
            ASSERT_IF(pnp->m_hpipe != NULL && pnp->m_hpipe != INVALID_HANDLE_VALUE);
            bret = CancelIoEx(pnp->m_hpipe, &(pnp->m_wrov));
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("can not cancel pending [%s] write", pnp->m_name);
            }
            pnp->m_wrpending = 0;
            pnp->m_wrleft = 0;
        }

        if (pnp->m_connevt != NULL) {
            bret = CloseHandle(pnp->m_connevt);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("close [%s].connevt[%p] error[%d]", pnp->m_name, pnp->m_connevt, ret);
            }
            pnp->m_connevt = NULL;
        }

        if (pnp->m_rdevt != NULL) {
            bret = CloseHandle(pnp->m_rdevt);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("close [%s].rdevt[%p] error[%d]", pnp->m_name, pnp->m_rdevt, ret);
            }
            pnp->m_rdevt = NULL;
        }

        if (pnp->m_wrevt != NULL) {
            bret = CloseHandle(pnp->m_wrevt);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("close [%s].wrevt[%p] error[%d]", pnp->m_name, pnp->m_wrevt, ret);
            }
            pnp->m_wrevt = NULL;
        }

        if (pnp->m_hpipe != NULL && pnp->m_hpipe != INVALID_HANDLE_VALUE) {
            if (pnp->m_servermode) {
                bret = DisconnectNamedPipe(pnp->m_hpipe);
                if (!bret) {
                    GETERRNO(ret);
                    ERROR_INFO("disconnect [%s] error[%d]", pnp->m_name, ret);
                }
                pnp->m_servermode = 0;
            }
            bret = CloseHandle(pnp->m_hpipe);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("close pipe [%s].[%p] error[%d]", pnp->m_name, pnp->m_hpipe, ret);
            }
            pnp->m_hpipe = NULL;
        }

        if (pnp->m_name) {
            free(pnp->m_name);
            pnp->m_name = NULL;
        }

        free(pnp);
        pnp = NULL;
        *ppnp = NULL;
    }
    return ;
}

pnamed_pipe_t __alloc_namedpipe(char* name, int servermode,int timeout)
{
    pnamed_pipe_t pnp = NULL;
    int ret;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    TCHAR* ptname = NULL;
    int tnamesize = 0;
    BOOL bret;

    pnp = (pnamed_pipe_t)malloc(sizeof(*pnp));
    if (pnp == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    memset(pnp, 0, sizeof(*pnp));
    pnp->m_magic = NAMED_PIPE_MAGIC;

    pnp->m_name = _strdup(name);
    if (pnp->m_name == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not strdup [%s] error[%d]", name, ret);
        goto fail;
    }

    ret = AnsiToTchar(name, &ptname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    pnp->m_rdevt = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (pnp->m_rdevt == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not create [%s].rdevt error[%d]", pnp->m_name, ret);
        goto fail;
    }
    pnp->m_rdov.hEvent = pnp->m_rdevt;

    pnp->m_wrevt = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (pnp->m_wrevt == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not create [%s].wrevt error[%d]", pnp->m_name, ret);
    }
    pnp->m_wrov.hEvent = pnp->m_wrevt;


    if (servermode) {
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE);
        sa.nLength = (DWORD) sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = (LPVOID) &sd;
        sa.bInheritHandle = TRUE;

        pnp->m_servermode = 1;
        pnp->m_hpipe = CreateNamedPipe(ptname, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                       1,
                                       8192, 8192, /*read write buffer 8192 size*/
                                       5000, /*timeout 5000 millisecond*/
                                       &sa);
        if (pnp->m_hpipe == INVALID_HANDLE_VALUE) {
            GETERRNO(ret);
            ERROR_INFO("can not create pipe [%s] error[%d]", pnp->m_name, ret);
            goto fail;
        }

        pnp->m_connevt = CreateEvent(NULL, TRUE, TRUE, NULL);
        if (pnp->m_connevt == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        pnp->m_connov.hEvent = pnp->m_connevt;

        bret = ConnectNamedPipe(pnp->m_hpipe, &(pnp->m_connov));
        if (!bret) {
            GETERRNO(ret);
            if (ret != -ERROR_IO_PENDING && ret != -ERROR_PIPE_CONNECTED) {
                ERROR_INFO("server pipe [%s] conn error[%d]", pnp->m_name, ret);
                goto fail;
            }

            if (ret == -ERROR_IO_PENDING) {
                pnp->m_connpending = 1;
            }
        }
    } else {
        if (timeout > 0) {
            bret = WaitNamedPipe(ptname,(DWORD)timeout);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("wait for [%s] error[%d]", name , ret);
                goto fail;
            }
        }
        pnp->m_hpipe = CreateFile(ptname,  GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (pnp->m_hpipe == INVALID_HANDLE_VALUE) {
            GETERRNO(ret);
            ERROR_INFO("can not connect [%s] error[%d]", pnp->m_name, ret);
            goto fail;
        }
    }

    AnsiToTchar(NULL, &ptname, &tnamesize);
    return pnp;
fail:
    __free_namedpipe(&pnp);
    AnsiToTchar(NULL, &ptname, &tnamesize);
    SETERRNO(ret);
    return NULL;
}

void* bind_namedpipe(char* name)
{
    pnamed_pipe_t pnp = NULL;
    int ret;
    pnp = __alloc_namedpipe(name, 1,0);
    if (pnp == NULL) {
        GETERRNO(ret);
        SETERRNO(ret);
        return NULL;
    }
    return (void*)pnp;
}

void* connect_namedpipe(char* name)
{
    pnamed_pipe_t pnp = NULL;
    int ret;
    pnp = __alloc_namedpipe(name, 0,0);
    if (pnp == NULL) {
        GETERRNO(ret);
        SETERRNO(ret);
        return NULL;
    }
    return (void*)pnp;
}

void* connect_namedpipe_timeout(char* name,int timeout)
{
    pnamed_pipe_t pnp = NULL;
    int ret;
    pnp = __alloc_namedpipe(name, 0,timeout);
    if (pnp == NULL) {
        GETERRNO(ret);
        SETERRNO(ret);
        return NULL;
    }
    return (void*)pnp;
}


void close_namedpipe(void** ppnp)
{
    pnamed_pipe_t pnp = NULL;
    if (ppnp && *ppnp) {
        pnp = (pnamed_pipe_t) (*ppnp);
        ASSERT_IF (pnp->m_magic == NAMED_PIPE_MAGIC);
        __free_namedpipe(&pnp);
        *ppnp = NULL;
    }
    return ;
}

HANDLE get_namedpipe_rdevt(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t) pnp1;
    int ret;
    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return NULL;
    }
    SETERRNO(0);
    return pnp->m_rdevt;
}

HANDLE get_namedpipe_wrevt(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t) pnp1;
    int ret;
    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return NULL;
    }
    SETERRNO(0);
    return pnp->m_wrevt;
}

HANDLE get_namedpipe_connevt(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t) pnp1;
    int ret;
    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return NULL;
    }
    SETERRNO(0);
    return pnp->m_connevt;
}


int get_namedpipe_rdstate(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t) pnp1;
    int ret;
    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    SETERRNO(0);
    return pnp->m_rdpending;
}

int get_namedpipe_wrstate(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t) pnp1;
    int ret;
    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    SETERRNO(0);
    return pnp->m_wrpending;
}

int get_namedpipe_connstate(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t) pnp1;
    int ret;
    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    SETERRNO(0);
    return pnp->m_connpending;
}


int read_namedpipe(void* pnp1, char* buffer, int bufsize)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    int ret;
    int retlen = 0;
    BOOL bret;
    char* ptr = NULL;
    int leftlen = 0;
    DWORD cbread;

    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pnp->m_rdpending ||
            pnp->m_connpending) {
        ret = -ERROR_IO_PENDING;
        goto fail;
    }


    leftlen = bufsize;
    ptr = buffer;
    while (leftlen > 0)  {
        bret = ReadFile(pnp->m_hpipe, ptr, (DWORD)leftlen, &cbread, &(pnp->m_rdov));
        if (!bret) {
            GETERRNO(ret);
            if (ret == -ERROR_IO_PENDING) {
                pnp->m_rdpending = 1;
                pnp->m_rdleft = leftlen;
                break;
            } else if (ret == -ERROR_MORE_DATA) {
                retlen += leftlen;
                leftlen = 0;
                break;
            }
            ERROR_INFO("read [%s] buffer error[%d]", pnp->m_name, ret);
            goto fail;
        }
        if (cbread == 0) {
            ret = -ERROR_BROKEN_PIPE;
            ERROR_INFO("[%s] read 0", pnp->m_name);
            goto fail;
        }

        leftlen -= cbread;
        retlen += cbread;
        ptr += cbread;
    }

    return retlen;
fail:
    SETERRNO(ret);
    return ret;
}

int write_namedpipe(void* pnp1, char* buffer, int bufsize)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    int ret;
    int retlen = 0;
    BOOL bret;
    char* ptr = NULL;
    int leftlen = 0;
    DWORD cbwrite;

    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pnp->m_connpending ||
            pnp->m_wrpending) {
        ret = -ERROR_IO_PENDING;
        goto fail;
    }


    leftlen = bufsize;
    ptr = buffer;
    while (leftlen > 0)  {
        bret = WriteFile(pnp->m_hpipe, ptr, (DWORD)leftlen, &cbwrite, &(pnp->m_wrov));
        if (!bret) {
            GETERRNO(ret);
            if (ret == -ERROR_IO_PENDING) {
                pnp->m_wrpending = 1;
                pnp->m_wrleft = leftlen;
                break;
            }
            ERROR_INFO("read [%s] buffer error[%d]", pnp->m_name, ret);
            goto fail;
        }

        leftlen -= cbwrite;
        retlen += cbwrite;
        ptr += cbwrite;
    }

    return retlen;
fail:
    SETERRNO(ret);
    return ret;
}

int complete_namedpipe_rdpending(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    int ret;
    int completed = 0;
    DWORD cbread = 0;
    BOOL bret;

    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pnp->m_rdpending == 0) {
        ret = -ERROR_INVALID_SERVER_STATE;
        goto fail;
    }

    cbread = 0;
    bret = GetOverlappedResult(pnp->m_hpipe, &(pnp->m_rdov), &cbread, FALSE);
    if (!bret) {
        GETERRNO(ret);
        if (ret != -ERROR_IO_PENDING && ret != -ERROR_MORE_DATA) {
            ERROR_INFO("get [%s]read ov error[%d]", pnp->m_name, ret);
            goto fail;
        }
        pnp->m_rdleft -= cbread;
    } else {
        pnp->m_rdleft -= cbread;
    }

    /*we have all completed*/
    if (pnp->m_rdleft == 0) {
        pnp->m_rdpending = 0;
        completed = 1;
    }

    return completed;
fail:
    SETERRNO(ret);
    return ret;
}

int complete_namedpipe_wrpending(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    int ret;
    int completed = 0;
    DWORD cbwrite = 0;
    BOOL bret;

    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pnp->m_wrpending == 0) {
        ret = -ERROR_INVALID_SERVER_STATE;
        goto fail;
    }

    bret = GetOverlappedResult(pnp->m_hpipe, &(pnp->m_wrov), &cbwrite, FALSE);
    if (!bret) {
        GETERRNO(ret);
        if (ret != -ERROR_IO_PENDING) {
            ERROR_INFO("get [%s]write ov error[%d]", pnp->m_name, ret);
            goto fail;
        }
    } else {
        pnp->m_wrleft -= cbwrite;
    }

    /*we have all completed*/
    if (pnp->m_wrleft == 0) {
        pnp->m_wrpending = 0;
        completed = 1;
    }
    
    return completed;
fail:
    SETERRNO(ret);
    return ret;
}

int complete_namedpipe_connpending(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    int ret;
    int completed = 0;
    DWORD cbret = 0;
    BOOL bret;

    if (pnp == NULL || pnp->m_magic != NAMED_PIPE_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (pnp->m_connpending == 0) {
        ret = -ERROR_INVALID_SERVER_STATE;
        goto fail;
    }

    bret = GetOverlappedResult(pnp->m_hpipe, &(pnp->m_connov),&cbret,FALSE);
    if (!bret) {
    	GETERRNO(ret);
    	if (ret != !-ERROR_IO_PENDING) {
    		ERROR_INFO("get [%s] bind ov error[%d]", pnp->m_name, ret);
    		goto fail;
    	}
    } else {
    	completed = 1;
        pnp->m_connpending = 0;
    }

    return completed;
fail:
    SETERRNO(ret);
    return ret;
}

int cancel_namedpipe_connect(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    BOOL bret;
    int ret;
    if (pnp == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (pnp->m_connpending == 0) {
        return 0;
    }
    bret = CancelIoEx(pnp->m_hpipe, &(pnp->m_connov));
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("cancel [%s] connect pending error [%d]", pnp->m_name, ret);
        SETERRNO(ret);
        return ret;
    }
    pnp->m_connpending = 0;
    return 1;
}

int cancel_namedpipe_read(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    BOOL bret;
    int ret;
    if (pnp == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (pnp->m_rdpending == 0) {
        return 0;
    }
    bret = CancelIoEx(pnp->m_hpipe, &(pnp->m_rdov));
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("cancel [%s] read pending error [%d]", pnp->m_name, ret);
        SETERRNO(ret);
        return ret;
    }
    pnp->m_rdpending = 0;
    return 1;
}

int cancel_namedpipe_write(void* pnp1)
{
    pnamed_pipe_t pnp = (pnamed_pipe_t)pnp1;
    BOOL bret;
    int ret;
    if (pnp == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (pnp->m_wrpending == 0) {
        return 0;
    }
    bret = CancelIoEx(pnp->m_hpipe, &(pnp->m_wrov));
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("cancel [%s] write pending error [%d]", pnp->m_name, ret);
        SETERRNO(ret);
        return ret;
    }
    pnp->m_wrpending = 0;
    return 1;
}