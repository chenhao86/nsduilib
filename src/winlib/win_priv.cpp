#include <win_priv.h>
#include <win_err.h>
#include <win_types.h>
#include <win_uniansi.h>

#include <Windows.h>

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

int __handle_priv_token(HANDLE htoken, const char* privstr, int enabled)
{
    TCHAR* ptpriv = NULL;
    int tprivsize = 0;
    int ret;
    BOOL bret;
    TOKEN_PRIVILEGES tp;
    LUID luid;


    ret = AnsiToTchar(privstr, &ptpriv, &tprivsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    bret = LookupPrivilegeValue(NULL, ptpriv, &luid);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("lookup [%s] error[%d]", privstr, ret);
        goto fail;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (enabled) {
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    } else {
        tp.Privileges[0].Attributes = 0;
    }

    bret = AdjustTokenPrivileges(htoken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("adjust %s [%s] error[%d]", privstr, enabled ? "enable" : "disable", ret);
        goto fail;
    }
    DEBUG_INFO("%s [%s] succ",  enabled ? "enable" : "disable", privstr);


    return 0;
fail:
    AnsiToTchar(NULL, &ptpriv, &tprivsize);
    SETERRNO(ret);
    return ret;
}



int __handle_priv(const char* privstr, int enabled)
{
    TCHAR* ptpriv = NULL;
    int tprivsize = 0;
    int ret;
    BOOL bret;
    HANDLE htoken = NULL;

    bret = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &htoken);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("open process token error[%d]", ret);
        goto fail;
    }

    ret = __handle_priv_token(htoken, privstr, enabled);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (htoken != NULL) {
        CloseHandle(htoken);
    }
    htoken = NULL;

    return 0;
fail:
    if (htoken != NULL) {
        CloseHandle(htoken);
    }
    htoken = NULL;
    AnsiToTchar(NULL, &ptpriv, &tprivsize);
    SETERRNO(ret);
    return ret;
}

int __get_priv_value_inner(HANDLE htoken,const char* privstr)
{
    TCHAR* ptpriv = NULL;
    int tprivsize = 0;
    int ret;
    BOOL bret;
    LUID luid;
    int value = 0;
    PVOID ptokenbuf=NULL;
    DWORD tokenbufsize=32;
    DWORD dret;
    TOKEN_PRIVILEGES* ptp=NULL;
    DWORD i;


    ret = AnsiToTchar(privstr, &ptpriv, &tprivsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    bret = LookupPrivilegeValue(NULL, ptpriv, &luid);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("lookup [%s] error[%d]", privstr, ret);
        goto fail;
    }

try_again:
    if (ptokenbuf) {
        HeapFree(GetProcessHeap(), 0, ptokenbuf);
    }
    ptokenbuf = NULL;
    ptokenbuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tokenbufsize);
    if (ptokenbuf == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    bret = GetTokenInformation(htoken,TokenPrivileges,ptokenbuf,tokenbufsize,&dret);
    if (!bret) {
        GETERRNO(ret);
        if (ret == -ERROR_INSUFFICIENT_BUFFER) {
            tokenbufsize <<= 1;
            goto try_again;
        }
        ERROR_INFO("get token buffer error[%d]", ret);
        goto fail;
    }

    ptp = (TOKEN_PRIVILEGES*) ptokenbuf;
    for (i=0;i<ptp->PrivilegeCount ;i ++) {
        if (ptp->Privileges[i].Luid.LowPart == luid.LowPart && 
            ptp->Privileges[i].Luid.HighPart == luid.HighPart) {
            if (ptp->Privileges[i].Attributes  & SE_PRIVILEGE_ENABLED) {
                value = 1;
            } else {
                value = 0;
            }
        }
    }
    DEBUG_INFO("[%d] value [%d]", privstr,value);

    if (ptokenbuf) {
        HeapFree(GetProcessHeap(), 0, ptokenbuf);
    }
    ptokenbuf = NULL;
    AnsiToTchar(NULL, &ptpriv, &tprivsize);
    return value;
fail:
    if (ptokenbuf) {
        HeapFree(GetProcessHeap(), 0, ptokenbuf);
    }
    ptokenbuf = NULL;
    AnsiToTchar(NULL, &ptpriv, &tprivsize);
    SETERRNO(ret);
    return ret;
}


int __get_priv_value(const char* privstr)
{
    int ret;
    int val=0;
    HANDLE htoken=NULL;
    BOOL bret;

    bret = OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&htoken);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("open token query error[%d]", ret);
        goto fail;
    }

    ret = __get_priv_value_inner(htoken,privstr);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    val = ret;

    if (htoken != NULL && htoken != INVALID_HANDLE_VALUE) {
        CloseHandle(htoken);
    }
    htoken = NULL;
    return val;
fail:
    if (htoken != NULL && htoken != INVALID_HANDLE_VALUE) {
        CloseHandle(htoken);
    }
    htoken = NULL;
    SETERRNO(ret);
    return ret;
}


int enable_security_priv(void)
{
    return __handle_priv("SeSecurityPrivilege", 1);
}

int disable_security_priv(void)
{
    return __handle_priv("SeSecurityPrivilege", 0);
}

int is_security_priv(void)
{
    return __get_priv_value("SeSecurityPrivilege");
}

int enable_takeown_priv(void)
{
    return __handle_priv("SeTakeOwnershipPrivilege", 1);
}

int disable_takeown_priv(void)
{
    return __handle_priv("SeTakeOwnershipPrivilege", 0);
}

int is_takeown_priv(void)
{
    return __get_priv_value("SeTakeOwnershipPrivilege");
}


int enable_restore_priv(void)
{
    return __handle_priv("SeRestorePrivilege", 1);
}

int disable_restore_priv(void)
{
    return __handle_priv("SeRestorePrivilege", 0);
}

int is_restore_priv(void)
{
    return __get_priv_value("SeRestorePrivilege");
}


int enable_backup_priv(void)
{
    return __handle_priv("SeBackupPrivilege", 1);
}
int disable_backup_priv(void)
{
    return __handle_priv("SeBackupPrivilege", 0);
}

int is_backup_priv(void)
{
    return __get_priv_value("SeBackupPrivilege");
}


int enable_impersonate_priv(void)
{
    return __handle_priv("SeImpersonatePrivilege", 1);
}

int disable_impersonate_priv(void)
{
    return __handle_priv("SeImpersonatePrivilege", 0);
}

int is_impersonate_priv(void)
{
    return __get_priv_value("SeImpersonatePrivilege");
}


int enable_audit_priv(void)
{
    return __handle_priv("SeAuditPrivilege", 1);
}
int disable_audit_priv(void)
{
    return __handle_priv("SeAuditPrivilege", 0);
}

int is_audit_priv(void)
{
    return __get_priv_value("SeAuditPrivilege");
}


int enable_debug_priv(void)
{
    return __handle_priv("SeDebugPrivilege", 1);
}

int disable_debug_priv(void)
{
    return __handle_priv("SeDebugPrivilege", 0);
}

int is_debug_priv(void)
{
    return __get_priv_value("SeDebugPrivilege");
}


int enable_tcb_priv(void)
{
    return __handle_priv("SeTcbPrivilege", 1);
}

int disable_tcb_priv(void)
{
    return __handle_priv("SeTcbPrivilege", 0);
}

int is_tcb_priv(void)
{
    return __get_priv_value("SeTcbPrivilege");
}


int enable_token_debug_priv(HANDLE htoken)
{
    return __handle_priv_token(htoken, "SeDebugPrivilege", 1);
}

int disable_token_debug_priv(HANDLE htoken)
{
    return __handle_priv_token(htoken, "SeDebugPrivilege", 0);
}


int enable_token_tcb_priv(HANDLE htoken)
{
    return __handle_priv_token(htoken, "SeTcbPrivilege", 1);
}

int disable_token_tcb_priv(HANDLE htoken)
{
    return __handle_priv_token(htoken, "SeTcbPrivilege", 0);
}

int enable_shutdown_priv(void)
{
    return __handle_priv("SeShutdownPrivilege",1);
}

int disable_shutdown_priv(void)
{
    return __handle_priv("SeShutdownPrivilege",0);
}

int is_shutdown_priv(void)
{
    return __get_priv_value("SeShutdownPrivilege");   
}

int enable_create_global_priv(void)
{
    return __handle_priv("SeCreateGlobalPrivilege",1);
}

int disable_create_global_priv(void)
{
    return __handle_priv("SeCreateGlobalPrivilege",0);   
}

int is_create_global_priv(void)
{
    return __get_priv_value("SeCreateGlobalPrivilege");      
}