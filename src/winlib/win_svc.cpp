#include <win_svc.h>
#include <win_err.h>
#include <win_uniansi.h>
#include <win_types.h>
#include <win_time.h>

#pragma comment(lib,"Advapi32.lib")

#if _MSC_VER >= 1910
#pragma warning(push)
#pragma warning(disable:5045)
#endif

void __close_scm(SC_HANDLE* pschd)
{
    BOOL bret;
    int ret;
    if (pschd != NULL && *pschd != NULL) {
        bret = CloseServiceHandle(*pschd);
        if (!bret) {
            GETERRNO(ret);
            ERROR_INFO("close %p error[%d]", *pschd, ret);
        }
        *pschd = NULL;
    }
}

void __close_svc(SC_HANDLE* pshsv)
{
    __close_scm(pshsv);
}

SC_HANDLE __open_scm(const char* name, DWORD accmode)
{
    SC_HANDLE schd = NULL;
    int ret;
    TCHAR* ptname = NULL;
    int tnamesize = 0;

    if (name != NULL) {
        ret = AnsiToTchar(name, &ptname, &tnamesize);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
    }

    schd = OpenSCManager(ptname, NULL, accmode);
    if (schd == NULL) {
        GETERRNO(ret);
        ERROR_INFO("open %s scm mode [0x%x|%d] err[%d]", name != NULL ? name : "local", accmode, accmode, ret);
        goto fail;
    }

    AnsiToTchar(NULL, &ptname, &tnamesize);

    return schd;
fail:
    __close_scm(&schd);
    AnsiToTchar(NULL, &ptname, &tnamesize);
    SETERRNO(ret);
    return NULL;
}

SC_HANDLE __open_svc(SC_HANDLE schd, const char* name, DWORD accmode)
{
    SC_HANDLE shsv = NULL;
    int ret;
    TCHAR* ptname = NULL;
    int tnamesize = 0;
    if (schd == NULL || name == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    ret = AnsiToTchar(name, &ptname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    shsv = OpenService(schd, ptname, accmode);
    if (shsv == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not open [%s] error[%d]", name, ret);
        goto fail;
    }

    AnsiToTchar(NULL, &ptname, &tnamesize);
    return shsv;
fail:
    __close_svc(&shsv);
    AnsiToTchar(NULL, &ptname, &tnamesize);
    SETERRNO(ret);
    return NULL;
}

int is_service_exist(const char* name)
{
    SC_HANDLE schd = NULL;
    SC_HANDLE shsv = NULL;
    int exist = 0;
    int ret;

    schd = __open_scm(NULL, GENERIC_READ);
    if (schd == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    shsv = __open_svc(schd, name, GENERIC_READ);
    if (shsv == NULL) {
        GETERRNO(ret);
        if (ret != -ERROR_SERVICE_DOES_NOT_EXIST) {
            goto fail;
        }
        exist = 0;
    } else {
        exist = 1;
    }

    __close_svc(&shsv);
    __close_scm(&schd);

    return exist;
fail:
    __close_svc(&shsv);
    __close_scm(&schd);
    SETERRNO(ret);
    return ret;
}

int __open_handle(const char* name, SC_HANDLE *pschd , SC_HANDLE *pshsv, DWORD scmmode, DWORD svcmode)
{
    int ret;
    if (name == NULL) {
        __close_svc(pshsv);
        __close_scm(pschd);
        return 0;
    }

    if (pschd == NULL || pshsv == NULL ||
            *pschd != NULL || *pshsv != NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    *pschd = __open_scm(NULL, scmmode);
    if (*pschd == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    *pshsv = __open_svc(*pschd, name, svcmode);
    if (*pshsv == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    return 0;
fail:
    __close_svc(pshsv);
    __close_scm(pschd);
    SETERRNO(ret);
    return ret;
}

int __inner_get_state(SC_HANDLE shsv, SERVICE_STATUS_PROCESS* pssp)
{
    BOOL bret;
    DWORD needed = 0;
    int ret;

    bret = QueryServiceStatusEx(shsv, SC_STATUS_PROCESS_INFO, (LPBYTE)pssp, sizeof(*pssp), &needed);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("query error[%d]", ret);
        SETERRNO(ret);
        return ret;
    }
    return 0;
}

int __get_service_current_state(const char* name)
{
    SC_HANDLE schd = NULL;
    SC_HANDLE shsv = NULL;
    int state;
    int ret;
    SERVICE_STATUS_PROCESS ssp;

    ret = __open_handle(name, &schd, &shsv, GENERIC_READ, GENERIC_READ);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    memset(&ssp, 0 , sizeof(ssp));
    ret = __inner_get_state(shsv, &ssp);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    state = (int) ssp.dwCurrentState;

    __open_handle(NULL, &schd, &shsv, GENERIC_READ, GENERIC_READ);
    return state;
fail:
    __open_handle(NULL, &schd, &shsv, GENERIC_READ, GENERIC_READ);
    SETERRNO(ret);
    return ret;
}

int is_service_running(const char* name)
{
    int state = 0;
    int ret;

    state = __get_service_current_state(name);
    if (state < 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }

    if (state == SERVICE_RUNNING) {
        return 1;
    }
    return 0;
}

int is_service_stopped(const char* name)
{
    int state = 0;
    int ret;

    state = __get_service_current_state(name);
    if (state < 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }

    if (state == SERVICE_STOPPED) {
        return 1;
    }
    return 0;
}

int service_running_mode(const char* name)
{
    int state ;
    int ret = SVC_STATE_UNKNOWN;

    state = __get_service_current_state(name);
    if (state < 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }

    switch (state) {
    case SERVICE_STOPPED:
        ret = SVC_STATE_STOPPED;
        break;
    case SERVICE_START_PENDING:
        ret = SVC_STATE_START_PENDING;
        break;
    case SERVICE_STOP_PENDING:
        ret = SVC_STATE_STOP_PENDING;
        break;
    case SERVICE_RUNNING:
        ret = SVC_STATE_RUNNING;
        break;
    case SERVICE_PAUSED:
        ret = SVC_STATE_PAUSED;
        break;
    case SERVICE_PAUSE_PENDING:
        ret = SVC_STATE_PAUSE_PENDING;
        break;
    case SERVICE_CONTINUE_PENDING:
        ret = SVC_STATE_CONTINUE_PENDING;
        break;
    default:
        ret = SVC_STATE_UNKNOWN;
        break;
    }
    return ret;
}

int __inner_get_config(SC_HANDLE shsv , QUERY_SERVICE_CONFIGW** ppconfigw, int *psize)
{
    int ret;
    BOOL bret;
    DWORD needed = 0;
    QUERY_SERVICE_CONFIGW* pretconfig = NULL;
    int retsize = 0;
    if (shsv == NULL) {
        if (ppconfigw && *ppconfigw != NULL) {
            free(*ppconfigw);
            *ppconfigw = NULL;
        }
        if (psize) {
            *psize = 0;
        }
        return 0;
    }

    if (ppconfigw == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretconfig = *ppconfigw;
    retsize = *psize;

try_again:
    bret = QueryServiceConfigW(shsv, pretconfig, (DWORD)retsize, &needed);
    if (!bret) {
        GETERRNO(ret);
        if (ret != -ERROR_INSUFFICIENT_BUFFER) {
            ERROR_INFO("query config error[%d]", ret);
            goto fail;
        }

        if (pretconfig != *ppconfigw && pretconfig != NULL)  {
            free(pretconfig);
        }
        pretconfig = NULL;
        if (retsize < (int)needed) {
            retsize = (int)needed ;
        } else {
            retsize <<= 1;
        }
        pretconfig = (QUERY_SERVICE_CONFIGW*) malloc((size_t)retsize);
        if (pretconfig == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc %d error[%d]", retsize, ret);
            goto fail;
        }
        goto try_again;
    }

    if (*ppconfigw != NULL && pretconfig != *ppconfigw) {
        free(*ppconfigw);
    }

    *ppconfigw = pretconfig;
    *psize = retsize;
    return 0;
fail:
    if (pretconfig != NULL && pretconfig != *ppconfigw) {
        free(pretconfig);
    }
    pretconfig = NULL;
    retsize = 0;
    SETERRNO(ret);
    return ret;
}

int __inner_set_config_start_mode(SC_HANDLE shsv , DWORD mode)
{
    BOOL bret;
    int ret;

    bret = ChangeServiceConfigW(shsv, SERVICE_NO_CHANGE,
                                mode, SERVICE_NO_CHANGE,
                                NULL, NULL,
                                NULL, NULL,
                                NULL, NULL, NULL);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not change config error[%d]", ret);
        goto fail;
    }
    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int __get_service_start_mode(const char* name)
{
    SC_HANDLE schd = NULL;
    SC_HANDLE shsv = NULL;
    int ret;
    int state;
    QUERY_SERVICE_CONFIGW* pconfigw = NULL;
    int size = 0;

    ret = __open_handle(name, &schd, &shsv, GENERIC_READ, GENERIC_READ);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __inner_get_config(shsv, &pconfigw, &size);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    state =(int) pconfigw->dwStartType;

    __inner_get_config(NULL, &pconfigw, &size);
    __open_handle(NULL, &schd, &shsv, GENERIC_READ, GENERIC_READ);
    return state;
fail:
    __inner_get_config(NULL, &pconfigw, &size);
    __open_handle(NULL, &schd, &shsv, GENERIC_READ, GENERIC_READ);
    SETERRNO(ret);
    return ret;

}

int get_service_start_mode(const char* name)
{
    int state = 0;
    int ret;

    state = __get_service_start_mode(name);
    if (state < 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }
    switch (state) {
    case SERVICE_BOOT_START:
        ret = SVC_START_ON_BOOT;
        break;
    case SERVICE_SYSTEM_START:
        ret = SVC_START_ON_SYSTEM;
        break;
    case SERVICE_AUTO_START:
        ret = SVC_START_ON_AUTO;
        break;
    case SERVICE_DEMAND_START:
        ret = SVC_START_ON_DEMAND;
        break;
    case SERVICE_DISABLED:
        ret = SVC_START_ON_DISABLED;
        break;
    default:
        ret = SVC_START_ON_UNKNOWN;
        break;
    }

    return ret;
}

int config_service_start_mode(const char* name, int startmode)
{
    int mode;
    int ret;
    SC_HANDLE shsv = NULL;
    SC_HANDLE schd = NULL;
    QUERY_SERVICE_CONFIGW* pconfigw = NULL;
    int configsize = 0;

    switch (startmode) {
    case SVC_START_ON_BOOT:
        mode = SERVICE_BOOT_START;
        break;
    case SVC_START_ON_SYSTEM:
        mode = SERVICE_SYSTEM_START;
        break;
    case SVC_START_ON_AUTO:
        mode = SERVICE_AUTO_START;
        break;
    case SVC_START_ON_DEMAND:
        mode = SERVICE_DEMAND_START;
        break;
    case SVC_START_ON_DISABLED:
        mode = SERVICE_DISABLED;
        break;
    default:
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ret = __open_handle(name, &schd, &shsv, SC_MANAGER_ALL_ACCESS, SERVICE_ALL_ACCESS);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __inner_get_config(shsv, &pconfigw, &configsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (pconfigw->dwStartType != (DWORD) mode) {
        pconfigw->dwStartType = (DWORD)mode;
        ret =  __inner_set_config_start_mode(shsv, (DWORD)mode);
        if (ret < 0) {
            GETERRNO(ret);
            ERROR_INFO("[%s] set [%d] error[%d]", name, startmode, ret);
            goto fail;
        }
    }

    __inner_get_config(NULL, &pconfigw, &configsize);
    __open_handle(NULL, &schd, &shsv, SERVICE_ALL_ACCESS, SERVICE_ALL_ACCESS);
    return 0;
fail:
    __inner_get_config(NULL, &pconfigw, &configsize);
    __open_handle(NULL, &schd, &shsv, SERVICE_ALL_ACCESS, SERVICE_ALL_ACCESS);
    SETERRNO(ret);
    return ret;
}


typedef struct __svc_depends {
    char* m_svcname;
    ENUM_SERVICE_STATUSA* m_depends;
    struct __svc_depends** m_subdepends;
    int m_depcnt;
    int m_reserv1;
} svc_depends_t,*psvc_depends_t;


void __free_svc_depends(psvc_depends_t* ppdep)
{
    psvc_depends_t pdep;
    int i;
    if (ppdep && *ppdep) {
        pdep = *ppdep;
        if (pdep->m_subdepends != NULL) {
            for (i=0;i< pdep->m_depcnt;i++) {
                __free_svc_depends(&(pdep->m_subdepends[i]));
            }
            free(pdep->m_subdepends);
            pdep->m_subdepends = NULL;
        }


        if (pdep->m_depends) {
            free(pdep->m_depends);
        }
        pdep->m_depends = NULL;
        pdep->m_depcnt = 0;
        if (pdep->m_svcname) {
            free(pdep->m_svcname);
        }
        pdep->m_svcname = NULL;
        free(pdep);
        *ppdep = NULL;
    }
    return ;
}

psvc_depends_t __alloc_svc_depends(const char* name)
{
    psvc_depends_t pdep = NULL;
    int ret;

    pdep = (psvc_depends_t)malloc(sizeof(*pdep));
    if (pdep == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not alloc [%d] error[%d]", sizeof(*pdep), ret);
        goto fail;
    }

    memset(pdep, 0, sizeof(*pdep));
    pdep->m_svcname = _strdup(name);
    if (pdep->m_svcname == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    return pdep;
fail:
    __free_svc_depends(&pdep);
    SETERRNO(ret);
    return NULL;
}

psvc_depends_t __get_svc_depends(SC_HANDLE sch,const char* name)
{
    psvc_depends_t pdep = NULL;
    ENUM_SERVICE_STATUSA* pstatus=NULL;
    int stssize=0;
    int i;
    BOOL bret;
    DWORD bufsize=0;
    DWORD bufret;
    DWORD svccnt=0;
    SC_HANDLE hservice = NULL;
    int ret;

    pdep = __alloc_svc_depends(name);
    if (pdep == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    hservice = __open_svc(sch,name,SERVICE_ALL_ACCESS);
    if (hservice == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    stssize = 4;
try_again:
    if (pstatus) {
        free(pstatus);
    }
    pstatus = NULL;
    bufsize = sizeof(*pstatus) * stssize;
    pstatus = (ENUM_SERVICE_STATUSA*)malloc(bufsize);
    if (pstatus == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    bret = EnumDependentServicesA(hservice,SERVICE_STATE_ALL,pstatus,bufsize,&bufret,&svccnt);
    if (!bret) {
        GETERRNO(ret);
        if (ret == -ERROR_MORE_DATA) {
            stssize <<= 1;
            goto try_again;
        }
        ERROR_INFO("can not get [%s] depends error[%d]", name,ret);
        goto fail;
    }

    pdep->m_depends = pstatus;
    pdep->m_depcnt = (int) svccnt;
    pstatus = NULL;

    if (hservice != NULL) {
        CloseHandle(hservice);
    }
    hservice = NULL;

    if (pdep->m_depcnt > 0) {
        pdep->m_subdepends = (struct __svc_depends**)malloc(sizeof(*(pdep->m_subdepends)) * pdep->m_depcnt);
        if (pdep->m_subdepends == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc [%d] error[%d]", sizeof(*(pdep->m_subdepends)) * pdep->m_depcnt);
            goto fail;
        }
        memset(pdep->m_subdepends, 0, sizeof(pdep->m_subdepends[0]) * pdep->m_depcnt);
        for (i=0;i<pdep->m_depcnt;i++) {
            pdep->m_subdepends[i] = __get_svc_depends(sch,pstatus[i].lpServiceName);
            if (pdep->m_subdepends[i] == NULL)  {
                GETERRNO(ret);
                goto fail;
            }
        }
    }

    return pdep;
fail:
    if (pstatus) {
        free(pstatus);
    }
    pstatus = NULL;
    svccnt = 0;
    /*because we freed pstatus on the NULL*/
    __free_svc_depends(&pdep);
    if (hservice != NULL) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    SETERRNO(ret);
    return NULL;
}

int __stop_service_single(SC_HANDLE sch, char* name, int mills)
{
    SC_HANDLE hservice = NULL;
    SERVICE_STATUS_PROCESS status;
    SERVICE_STATUS sts;
    DWORD byteret;
    int ret;
    uint64_t sticks,cticks;
    BOOL bret;
    int retcnt = 0;

    sticks = get_current_ticks();

    hservice = __open_svc(sch, name, SERVICE_ALL_ACCESS);
    if (hservice == NULL) {
        GETERRNO(ret);
        if (ret != -ERROR_SERVICE_DOES_NOT_EXIST) {
            SETERRNO(ret);
            return ret;            
        }
        return 0;
    }

    memset(&status, 0, sizeof(status));
    do{
        cticks = get_current_ticks();
        ret = need_wait_times(sticks,cticks, mills);
        if (ret < 0) {
            ret = -WAIT_TIMEOUT;
            ERROR_INFO("wait [%s] stop timed out", name);
            goto fail;
        }

        bret = QueryServiceStatusEx(hservice,SC_STATUS_PROCESS_INFO,(LPBYTE)&status, sizeof(status),&byteret);
        if (!bret) {
            GETERRNO(ret);
            ERROR_INFO("can not query [%s] status [%d]", name, ret);
            goto fail;
        }
        if (status.dwCurrentState == SERVICE_STOP_PENDING) {
            bret = SwitchToThread();
            if (!bret) {
                SleepEx(1,TRUE);
            }
        } else if (status.dwCurrentState != SERVICE_STOPPED) {
            /*now to stopped*/
            bret = ControlService(hservice,SERVICE_CONTROL_STOP,&sts);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("control [%s] stop error[%d]", name, ret);
                goto fail;
            }
            retcnt = 1;
        }
    }while(status.dwCurrentState != SERVICE_STOPPED );


    if (hservice != NULL) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    return retcnt;
fail:
    if (hservice != NULL) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    SETERRNO(ret);
    return ret;
}



int __stop_service_dep(SC_HANDLE sch,psvc_depends_t pdep, int mills)
{
    int i;
    int ret;
    int ncnt = 0;
    if (pdep->m_depcnt > 0) {
        for (i=0;i<pdep->m_depcnt;i++) {
            ret = __stop_service_dep(sch, pdep->m_subdepends[i],mills);
            if (ret < 0) {
                GETERRNO(ret);
                SETERRNO(ret);
                return ret;
            }
            ncnt += ret;
        }
    }

    ret = __stop_service_single(sch, pdep->m_svcname,mills);
    if (ret < 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }
    ncnt += ret;
    return ncnt;
}


int stop_service(const char* name,  int mills)
{
    SC_HANDLE sch = NULL;
    int ret;
    psvc_depends_t pdep=NULL;

    sch = __open_scm(NULL, SC_MANAGER_ALL_ACCESS);
    if (sch == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    pdep = __get_svc_depends(sch,name);
    if (pdep == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __stop_service_dep(sch,pdep,mills);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    __close_scm(&sch);
    return 0;
fail:
    __close_scm(&sch);
    SETERRNO(ret);
    return ret;
}

int __start_service_single(SC_HANDLE sch, ENUM_SERVICE_STATUSA* pstatus,int mills)
{
    int nret=0;
    SC_HANDLE hservice = NULL;
    int ret;
    SERVICE_STATUS_PROCESS procsts;
    uint64_t sticks,cticks;
    BOOL bret;
    DWORD bufret;
    if (pstatus->ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
        return nret;
    }

    sticks = get_current_ticks();
    hservice = __open_svc(sch,pstatus->lpServiceName,SERVICE_ALL_ACCESS);
    if (hservice == NULL) {
        GETERRNO(ret);
        if (ret != -ERROR_SERVICE_DOES_NOT_EXIST) {
            SETERRNO(ret);
            return ret;
        }
        return 0;
    }

    memset(&procsts,0, sizeof(procsts));
    do{
        cticks = get_current_ticks();
        ret = need_wait_times(sticks,cticks, mills);
        if (ret < 0) {
            ret = -WAIT_TIMEOUT;
            ERROR_INFO("serice [%s] wait running timeout", pstatus->lpServiceName);
            goto fail;
        }
        bret = QueryServiceStatusEx(hservice,SC_STATUS_PROCESS_INFO,(LPBYTE)&procsts, sizeof(procsts),&bufret);
        if (!bret) {
            GETERRNO(ret);
            ERROR_INFO("query [%s] status error[%d]", pstatus->lpServiceName, ret);
            goto fail;
        }

        if (procsts.dwCurrentState == SERVICE_START_PENDING) {
            bret = SwitchToThread();
            if (!bret) {
                SleepEx(1,TRUE);
            }
        } else if (procsts.dwCurrentState != SERVICE_RUNNING) {
            bret = StartService(hservice,0,NULL);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("start service [%s] error[%d]", pstatus->lpServiceName, ret);
                goto fail;
            }
            nret ++;
        }
    } while(procsts.dwCurrentState != SERVICE_RUNNING);


    if (hservice) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    return nret;
fail:
    if (hservice) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    SETERRNO(ret);
    return ret;
}

#pragma warning(push)
#pragma warning(disable:4717)

int __start_service_dep(SC_HANDLE sch,psvc_depends_t pdep,int mills)
{
    int i;
    int retcnt = 0;
    int ret;
    if (pdep->m_subdepends) {
        for (i=0;i<pdep->m_depcnt;i++) {
            ret = __start_service_dep(sch, pdep->m_subdepends[i], mills);
            if (ret < 0) {
                GETERRNO(ret);
                SETERRNO(ret);
                return ret;
            }
            retcnt += ret;
        }
    }

    ret = __start_service_dep(sch,pdep,mills);
    if (ret < 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }
    retcnt += ret;
    return retcnt;
}

#pragma warning(pop)


int start_service(const char* name, int mills)
{
    SC_HANDLE schd = NULL, shsv = NULL;
    int ret;
    SERVICE_STATUS_PROCESS ssp;
    uint64_t sticks, cticks;
    BOOL bret;

    ret = __open_handle(name, &schd, &shsv, SC_MANAGER_ALL_ACCESS, SERVICE_ALL_ACCESS);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    sticks = get_current_ticks();
    /*now we should get the state*/
    ret =  __inner_get_state(shsv, &ssp);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (ssp.dwCurrentState == SERVICE_RUNNING) {
        goto succ;
    } else if (ssp.dwCurrentState == SERVICE_START_PENDING) {
        while (1) {
            ret =  __inner_get_state(shsv, &ssp);
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
            if (ssp.dwCurrentState == SERVICE_RUNNING) {
                goto succ;
            } else if (ssp.dwCurrentState != SERVICE_START_PENDING) {
                ret = -ERROR_INTERNAL_ERROR;
                ERROR_INFO("[%s] state [%d]", name, ssp.dwCurrentState);
                goto fail;
            }

            /*wait for a 100*/
            ret = 100;
            cticks = get_current_ticks();
            ret = need_wait_times(sticks,cticks, mills);
            if (ret < 0) {
                ret = -WAIT_TIMEOUT;
                ERROR_INFO("wait [%s] start pending timed out", name);
                goto fail;
            }
            if (ret > 100) {
                ret = 100;
            }
            SleepEx((DWORD)ret, TRUE);
        }
    }

    bret = StartService(shsv, 0, NULL);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("start [%s] error[%d]", name, ret);
        goto fail;
    }

    ret =  __inner_get_state(shsv, &ssp);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (ssp.dwCurrentState == SERVICE_RUNNING) {
        goto succ;
    } else if (ssp.dwCurrentState == SERVICE_START_PENDING) {
        while (1) {
            ret =  __inner_get_state(shsv, &ssp);
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
            if (ssp.dwCurrentState == SERVICE_RUNNING) {
                goto succ;
            } else if (ssp.dwCurrentState != SERVICE_START_PENDING) {
                ret = -ERROR_INTERNAL_ERROR;
                ERROR_INFO("[%s] state [%d]", name, ssp.dwCurrentState);
                goto fail;
            }

            /*wait for a 100*/
            ret = 100;
            cticks = get_current_ticks();
            ret = need_wait_times(sticks,cticks, mills);
            if (ret < 0) {
                ret = -WAIT_TIMEOUT;
                ERROR_INFO("wait [%s] start pending timed out", name);
                goto fail;
            }
            if (ret > 100) {
                ret = 100;
            }
            SleepEx((DWORD)ret, TRUE);
        }
    } else {
        ret = -ERROR_INTERNAL_ERROR;
        ERROR_INFO("start [%s] not valid state [%d]", name, ssp.dwCurrentState);
        goto fail;
    }

succ:
    __open_handle(NULL, &schd, &shsv, SERVICE_ALL_ACCESS, SERVICE_ALL_ACCESS);
    return 0;
fail:
    __open_handle(NULL, &schd, &shsv, SERVICE_ALL_ACCESS, SERVICE_ALL_ACCESS);
    SETERRNO(ret);
    return ret;
}




static SERVICE_STATUS          glbl_svc_status;
static SERVICE_STATUS_HANDLE   glbl_svc_status_hd = NULL;


int  svc_init_mode(char* svcname, LPHANDLER_FUNCTION_EX pFunc, void* puserdata)
{
    int ret;
    TCHAR* ptsvcname = NULL;
    int tsvcsize = 0;

    if (glbl_svc_status_hd != NULL ) {
        return 0;
    }
    ret =  AnsiToTchar(svcname, &ptsvcname, &tsvcsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    glbl_svc_status_hd = RegisterServiceCtrlHandlerEx(ptsvcname, pFunc, puserdata);
    if (glbl_svc_status_hd == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not register svc[%s] error %d\n", svcname, ret);
        goto fail;
    }

    memset(&glbl_svc_status, 0, sizeof(glbl_svc_status));
    glbl_svc_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    glbl_svc_status.dwServiceSpecificExitCode = 0;

    ret = svc_report_mode(SERVICE_START_PENDING, 3000);
    if (ret < 0) {
        goto fail;
    }

    AnsiToTchar(NULL, &ptsvcname, &tsvcsize);

    return 0;
fail:
    if (glbl_svc_status_hd) {
        CloseHandle(glbl_svc_status_hd);
    }
    glbl_svc_status_hd = NULL;
    AnsiToTchar(NULL, &ptsvcname, &tsvcsize);
    SETERRNO(ret);
    return ret;
}

int svc_report_mode(DWORD mode, DWORD time)
{
    static DWORD st_chkpnt = 1;
    BOOL bret;
    int ret;
    glbl_svc_status.dwServiceType = SERVICE_WIN32;
    glbl_svc_status.dwCurrentState = mode;
    glbl_svc_status.dwWin32ExitCode = NO_ERROR;
    glbl_svc_status.dwWaitHint = time;

    if (mode == SERVICE_START_PENDING) {
        glbl_svc_status.dwControlsAccepted = 0;
    } else {
        glbl_svc_status.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                             | SERVICE_ACCEPT_PAUSE_CONTINUE
                                             | SERVICE_ACCEPT_SHUTDOWN
                                             | SERVICE_ACCEPT_PARAMCHANGE
                                             | SERVICE_ACCEPT_NETBINDCHANGE
                                             | SERVICE_ACCEPT_HARDWAREPROFILECHANGE
                                             | SERVICE_ACCEPT_POWEREVENT
                                             | SERVICE_ACCEPT_SESSIONCHANGE
                                             | SERVICE_ACCEPT_PRESHUTDOWN
                                             | SERVICE_ACCEPT_TIMECHANGE
                                             | SERVICE_ACCEPT_TRIGGEREVENT;
    }

    if ((mode == SERVICE_RUNNING) ||
            (mode == SERVICE_STOPPED)) {
        glbl_svc_status.dwCheckPoint = 0;
    } else {
        //glbl_svc_status.dwCheckPoint = st_chkpnt;
        //st_chkpnt ++;
        glbl_svc_status.dwCheckPoint = 0;
    }

    DEBUG_INFO("hd %p mode %d accepted control 0x%lx", glbl_svc_status_hd, mode, glbl_svc_status.dwControlsAccepted);

    bret = SetServiceStatus(glbl_svc_status_hd, &glbl_svc_status);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("report service mode 0x%x error %d\n", mode, ret);
        return ret;
    }
    return 0;
}


void svc_close_mode()
{
    int ret;
    if (glbl_svc_status_hd == NULL) {
        return ;
    }

    ret = svc_report_mode(SERVICE_STOPPED, 1000);
    if (ret < 0) {
        ERROR_INFO("close mode report error %d\n", ret);
    }
    CloseHandle(glbl_svc_status_hd);
    glbl_svc_status_hd = NULL;
    return ;
}

int svc_start(char* svcname, LPSERVICE_MAIN_FUNCTION pProc)
{
    BOOL bret;
    int ret;
    TCHAR* ptsvcname = NULL;
    int tsvcsize = 0;
    SERVICE_TABLE_ENTRY DispatchTable[] = {
        { NULL, NULL },
        { NULL, NULL }
    };


    ret = AnsiToTchar(svcname, &ptsvcname, &tsvcsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    DispatchTable[0].lpServiceName = ptsvcname;
    DispatchTable[0].lpServiceProc = pProc;

    bret = StartServiceCtrlDispatcher( DispatchTable );

    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not start service svc error %d\n", ret);
        goto fail;
    }

    DispatchTable[0].lpServiceName = NULL;
    DispatchTable[0].lpServiceProc = NULL;
    AnsiToTchar(NULL, &ptsvcname, &tsvcsize);
    return 0;
fail:
    DispatchTable[0].lpServiceName = NULL;
    DispatchTable[0].lpServiceProc = NULL;
    AnsiToTchar(NULL, &ptsvcname, &tsvcsize);
    SETERRNO(ret);
    return ret;

}

int __create_svc_inner(const char* name, const char* desc, const char* binpath, int startmode, DWORD svctype)
{
    TCHAR* ptname = NULL;
    int tnamesize = 0;
    TCHAR* ptdesc = NULL;
    int tdescsize = 0;
    TCHAR* ptbin = NULL;
    int tbinsize = 0;
    DWORD startdword = 0;
    SC_HANDLE sch = NULL;
    SC_HANDLE hservice = NULL;
    int ret;    

    if (name == NULL || binpath == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    switch (startmode) {
    case SVC_START_ON_BOOT:
        startdword = SERVICE_BOOT_START;
        break;
    case SVC_START_ON_SYSTEM:
        startdword = SERVICE_SYSTEM_START;
        break;
    case SVC_START_ON_AUTO:
        startdword = SERVICE_AUTO_START;
        break;
    case SVC_START_ON_DEMAND:
        startdword = SERVICE_DEMAND_START;
        break;
    case SVC_START_ON_DISABLED:
        startdword = SERVICE_DISABLED;
        break;
    default:
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }


    sch = __open_scm(NULL, SC_MANAGER_ALL_ACCESS);
    if (sch == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToTchar(name, &ptname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (desc != NULL) {
        ret = AnsiToTchar(desc, &ptdesc, &tdescsize);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
    }

    ret = AnsiToTchar(binpath, &ptbin, &tbinsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    hservice = CreateService(sch,
                             ptname,
                             ptdesc, SERVICE_ALL_ACCESS,
                             svctype,
                             startdword,
                             SERVICE_ERROR_NORMAL,
                             ptbin,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    if (hservice == NULL) {
        GETERRNO(ret);
        if (ret != -ERROR_SERVICE_EXISTS) {
            ERROR_INFO("create service %s [%s] error[%d]", name, binpath, ret);
            goto fail;
        }
    }

    if (hservice != NULL) {
        CloseHandle(hservice);
    }
    hservice = NULL;


    AnsiToTchar(NULL, &ptname, &tnamesize);
    AnsiToTchar(NULL, &ptdesc, &tdescsize);
    AnsiToTchar(NULL, &ptbin, &tbinsize);
    CloseHandle(sch);
    sch = NULL;

    return 0;
fail:
    /*no hservice close*/
    //if (hservice) {
    //    CloseHandle(hservice);
    //}
    hservice = NULL;

    AnsiToTchar(NULL, &ptname, &tnamesize);
    AnsiToTchar(NULL, &ptdesc, &tdescsize);
    AnsiToTchar(NULL, &ptbin, &tbinsize);

    if (sch != NULL) {
        CloseHandle(sch);
    }
    sch = NULL;
    SETERRNO(ret);
    return ret;    
}

int create_service(const char* name, const char* desc, const char* binpath, int startmode)
{
    return __create_svc_inner(name,desc,binpath,startmode,SERVICE_WIN32_OWN_PROCESS);
}

int create_driver(const char* name, const char* desc,const char* binpath, int startmode)
{
    return __create_svc_inner(name,desc,binpath,startmode,SERVICE_KERNEL_DRIVER);
}

int delete_service(const char* name)
{
    SC_HANDLE sch=NULL;
    SC_HANDLE hservice=NULL;
    int ret;
    BOOL bret;

    if (name == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    sch = __open_scm(NULL,SC_MANAGER_ALL_ACCESS);
    if (sch == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    hservice = __open_svc(sch,name,SERVICE_ALL_ACCESS);
    if (hservice == NULL) {
        GETERRNO(ret);
        if (ret != -ERROR_SERVICE_DOES_NOT_EXIST) {
            GETERRNO(ret);
            goto fail;
        }
        goto succ;
    }

    bret = DeleteService(hservice);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not delete service %s error[%d]", name, ret);
        goto fail;
    }

succ:
    if (hservice) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    if (sch) {
        CloseHandle(sch);
    }
    sch = NULL;

    return 0;
fail:
    if (hservice) {
        CloseHandle(hservice);
    }
    hservice = NULL;
    if (sch) {
        CloseHandle(sch);
    }
    sch = NULL;

    SETERRNO(ret);
    return ret;
}


#if _MSC_VER >= 1910
#pragma warning(pop)
#endif