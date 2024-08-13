



#include <win_envop_inner.h>
#include <win_envop.h>
#include <win_err.h>
#include <win_uniansi.h>

#pragma warning(push)
#pragma warning(disable:4820)

#include <psapi.h>

#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4820)
#pragma warning(disable:4365)
#pragma warning(disable:4514)

#include <wdbgexts.h>
#pragma warning(pop)


#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib,"Advapi32.lib")

int get_env_variable(char* envvar, char** ppenvval, int* pvalsize)
{
    char* penv = NULL;
    char* pretval = NULL;
    int retsize = 0;
    int vallen = 0, ret = 0;
    size_t slen = 0;
    size_t envnum = 0;


    if (envvar == NULL) {
        if (ppenvval && *ppenvval) {
            free(*ppenvval);
        }
        if (ppenvval) {
            *ppenvval = NULL;
        }
        if (pvalsize) {
            *pvalsize = 0;
        }
        return 0;
    }

    if (ppenvval == NULL || pvalsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    pretval = *ppenvval;
    retsize = *pvalsize;

    ret = _dupenv_s(&penv, &envnum, envvar);
    if (ret != 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (envnum == 0) {
        ret = -ERROR_NOT_FOUND;
        goto fail;
    }

    slen = strlen(penv) + 1;
    if ((int)slen > retsize || pretval == NULL) {
        retsize = (int)slen;
        pretval = (char*)malloc((size_t)retsize);
        if (pretval == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc[%d] error[%d]", retsize, ret);
            goto fail;
        }
    }

    strncpy_s(pretval, (size_t) retsize, penv, (size_t)retsize);

    if (*ppenvval && *ppenvval != pretval) {
        free(*ppenvval);
    }
    *ppenvval = pretval;
    *pvalsize = retsize;
    //if (penv) {
    /*because penv != NULL will here*/
    free(penv);
    //}
    penv = NULL;

    return vallen;
fail:
    if (pretval && pretval != *ppenvval) {
        free(pretval);
    }
    pretval = NULL;
    if (penv) {
        free(penv);
    }
    penv = NULL;


    SETERRNO(-ret);
    return ret;
}

int get_computer_name(int type, char** ppname, int *pnamesize)
{
    TCHAR *ptname = NULL;
    DWORD tnamesize = 0;
    DWORD dret;
    char* pretname = NULL;
    int retnamesize = 0;
    int ret;
    BOOL bret;
    int retlen = 0;
    char* ptmpname = NULL;
    int tmpnamesize = 0;
    COMPUTER_NAME_FORMAT  format = ComputerNamePhysicalDnsDomain;

    if (type == COMPUTER_NAME_NONE) {
        if (ppname && *ppname != NULL) {
            free(*ppname);
            *ppname = NULL;
        }
        if (pnamesize != NULL) {
            *pnamesize = 0;
        }
        return 0;
    }

    if (ppname == NULL || pnamesize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    switch (type) {
    case COMPUTER_NAME_DNS:
        format = ComputerNamePhysicalDnsDomain;
        break;
    case COMPUTER_NAME_NETBIOS:
        format = ComputerNamePhysicalNetBIOS;
        break;
    case COMPUTER_NAME_PHYS:
        format = ComputerNamePhysicalDnsHostname;
        break;
    default:
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretname = *ppname;
    retnamesize = *pnamesize;

    tnamesize = 256;
try_again:
    if (ptname) {
        free(ptname);
    }
    ptname = NULL;
    ptname = (TCHAR*) malloc((tnamesize * sizeof(TCHAR)));
    if (ptname == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    dret = tnamesize;
    bret = GetComputerNameEx(format, ptname, &dret);
    if (!bret) {
        GETERRNO(ret);
        if (ret == -ERROR_BUFFER_OVERFLOW) {
            tnamesize <<= 1;
            goto try_again;
        }
        ERROR_INFO("can not get computer name error[%d]", ret);
        goto fail;
    }

    ret = TcharToAnsi(ptname, &ptmpname, &tmpnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    retlen = (int)strlen(ptmpname);
    if (retlen >= retnamesize || pretname == NULL) {
        if (retlen >= retnamesize) {
            retnamesize = retlen + 3;
        }
        pretname = (char*) malloc((size_t)retnamesize);
    }
    memset(pretname, 0 , (size_t)retnamesize);
    strncpy_s(pretname, (size_t)retnamesize, ptmpname, (size_t)retnamesize);

    TcharToAnsi(NULL, &ptmpname, &tmpnamesize);
    if (ptname != NULL) {
        free(ptname);
    }
    ptname = NULL;
    tnamesize = 0;

    if (*ppname && *ppname != pretname) {
        free(*ppname);
    }
    *ppname = pretname;
    *pnamesize = retnamesize;

    return retlen;
fail:
    TcharToAnsi(NULL, &ptmpname, &tmpnamesize);
    if (ptname != NULL) {
        free(ptname);
    }
    ptname = NULL;
    tnamesize = 0;

    if (pretname != NULL && pretname != *ppname) {
        free(pretname);
    }
    pretname = NULL;
    retnamesize = 0;
    SETERRNO(ret);
    return ret;
}

int set_computer_name(int type, char* pname)
{
    TCHAR* ptname = NULL;
    int tnamesize = 0;
    int ret;
    BOOL bret;
    COMPUTER_NAME_FORMAT format = ComputerNamePhysicalDnsDomain;

    switch (type) {
    case COMPUTER_NAME_DNS:
        format = ComputerNamePhysicalDnsDomain;
        break;
    case COMPUTER_NAME_NETBIOS:
        format = ComputerNamePhysicalNetBIOS;
        break;
    case COMPUTER_NAME_PHYS:
        format = ComputerNamePhysicalDnsHostname;
        break;
    default:
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ret = AnsiToTchar(pname, &ptname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    bret = SetComputerNameEx(format, ptname);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("set [%s] computer name error[%d]",  pname, ret);
        goto fail;
    }

    AnsiToTchar(NULL, &ptname, &tnamesize);
    return 0;
fail:
    AnsiToTchar(NULL, &ptname, &tnamesize);
    SETERRNO(ret);
    return ret;
}


int get_codepage(void)
{
    UINT cp = 0;
    int ret = 0;

    cp = GetConsoleCP();
    if (cp == 0) {
        GETERRNO(ret);
        ERROR_INFO("can not get cp error[%d]", ret);
        goto fail;
    }

    return (int)cp;
fail:
    SETERRNO(ret);
    return ret;
}

int set_codepage(int cp)
{
    BOOL bret;
    int ret;
    int oldcp = 0;

    oldcp = get_codepage();
    if (oldcp < 0) {
        GETERRNO(ret);
        goto fail;
    }

    bret = SetConsoleCP((UINT)cp);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not set [%d] error[%d]", cp, ret);
        goto fail;
    }

    bret = SetConsoleOutputCP((UINT)cp);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not set [%d] output error[%d]", cp, ret);
        goto fail;
    }


    return 0;
fail:
    if (oldcp > 0)  {
        SetConsoleCP((UINT)cp);
        SetConsoleOutputCP((UINT)cp);
    }
    oldcp = 0;
    SETERRNO(ret);
    return ret;
}

int get_current_user(int freed, char** ppuser, int *psize)
{
    TCHAR* ptuser = NULL;
    int tusersize = 0;
    DWORD tuserlen = 0;
    int ret;
    int retlen = 0;
    BOOL bret;

    if (freed) {
        return TcharToAnsi(NULL, ppuser, psize);
    }
    if (ppuser == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    tusersize = 32;
try_again:
    if (ptuser) {
        free(ptuser);
    }
    ptuser = NULL;
    ptuser = (TCHAR*)malloc(sizeof(*ptuser) * tusersize);
    if (ptuser == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc %d error[%d]", sizeof(*ptuser) * tusersize, ret);
        goto fail;
    }
    memset(ptuser , 0 , sizeof(*ptuser) * tusersize);

    tuserlen = (DWORD)tusersize;
    bret = GetUserName(ptuser, &tuserlen);
    if (!bret) {
        GETERRNO(ret);
        if (ret == -ERROR_INSUFFICIENT_BUFFER) {
            tusersize = (int)(tuserlen << 1);
            goto try_again;
        }
        ERROR_INFO("get user name error[%d]" , ret);
        goto fail;
    }
    ret = TcharToAnsi(ptuser, ppuser, psize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    retlen = ret;

    if (ptuser) {
        free(ptuser);
    }
    ptuser = NULL;
    tusersize = 0;
    return retlen;
fail:
    if (ptuser) {
        free(ptuser);
    }
    ptuser = NULL;
    tusersize = 0;
    SETERRNO(ret);
    return ret;

}

int get_executable_wholepath(int freed, char** ppath, int *psize)
{
    TCHAR* ptpath = NULL;
    int tsize = 0;
    int retlen = 0;
    DWORD dret = 0;
    int ret;
    if (freed) {
        TcharToAnsi(NULL, ppath, psize);
        return 0;
    }

    if (ppath == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    tsize = 32;

try_again:
    if (ptpath) {
        free(ptpath);
    }
    ptpath = NULL;
    ptpath = (TCHAR*) malloc(sizeof(*ptpath) * tsize);
    if (ptpath == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc [%d] error[%d]", sizeof(*ptpath)* tsize, ret);
        goto fail;
    }
    memset(ptpath, 0 , sizeof(*ptpath) * tsize);

    dret = GetModuleFileName(NULL, ptpath, (DWORD)tsize);
    if (dret == 0) {
        GETERRNO(ret);
        if (ret == -ERROR_INSUFFICIENT_BUFFER) {
            tsize <<= 1;
            goto try_again;
        }
        ERROR_INFO("module name error[%d]", ret);
        goto fail;
    } else if (dret >= (DWORD)tsize) {
        tsize <<= 1;
        goto try_again;
    }

    ret = TcharToAnsi(ptpath, ppath, psize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    retlen = ret;
    DEBUG_INFO("path %s",*ppath);
    if (ptpath) {
        free(ptpath);
    }
    ptpath = NULL;
    tsize = 0;
    return retlen;
fail:
    if (ptpath) {
        free(ptpath);
    }
    ptpath = NULL;
    tsize = 0;
    SETERRNO(ret);
    return ret;
}

int get_executable_dirname(int freed, char** ppath, int *psize)
{
    DWORD dret;
    TCHAR* ptpath = NULL;
    char* pathansi = NULL;
    int tsize = 0;
    int apsize = 0, aplen = 0;
    int cpylen = 0;
    int retsize = 0;
    char* pretpath = NULL;
    char* ptr;
    int ret;
    if (freed) {
        if (ppath && *ppath) {
            free(*ppath);
            *ppath = NULL;
        }
        if (psize) {
            *psize = 0;
        }
        return 0;
    }

    tsize = 32;
get_wholeagain:
    if (ptpath) {
        free(ptpath);
    }
    ptpath = NULL;
    ptpath = (TCHAR*) malloc(sizeof(*ptpath) * tsize);
    if (ptpath == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc [%d] error[%d]", sizeof(*ptpath)* tsize, ret);
        goto fail;
    }
    memset(ptpath, 0 , sizeof(*ptpath) * tsize);

    dret = GetModuleFileName(NULL, ptpath, (DWORD)tsize);
    if (dret == 0) {
        GETERRNO(ret);
        if (ret == -ERROR_INSUFFICIENT_BUFFER) {
            tsize <<= 1;
            goto get_wholeagain;
        }
        ERROR_INFO("module name error[%d]", ret);
        goto fail;
    } else if (dret >= (DWORD)tsize) {
        tsize <<= 1;
        goto get_wholeagain;
    }


    ret = TcharToAnsi(ptpath, &pathansi, &apsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    aplen = ret;
    cpylen = aplen;
    ptr = pathansi + aplen;
    cpylen --;
    ptr --;

    while (cpylen > 0 && *ptr != '\\') {
        cpylen --;
        ptr --;
    }


    pretpath = *ppath;
    retsize = *psize;

    if (pretpath == NULL || retsize <= cpylen) {
        if (retsize <= cpylen) {
            retsize = cpylen + 1;
        }
        pretpath = (char*)malloc((size_t)retsize);
        if (pretpath == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc %d error[%d]", retsize, ret);
            goto fail;
        }
    }
    memset(pretpath, 0, (size_t)retsize);
    memcpy(pretpath, pathansi, (size_t)cpylen);

    if (*ppath && *ppath != pretpath) {
        free(*ppath);
    }
    *ppath = pretpath;
    *psize = retsize;

    TcharToAnsi(NULL, &pathansi, &apsize);
    if (ptpath) {
        free(ptpath);
    }
    ptpath = NULL;

    return cpylen;
fail:
    /*we can not get pretpath != NULL && pretpath != *ppath ,so not 
      free(pretpath)*/
    pretpath = NULL;
    retsize = 0;

    TcharToAnsi(NULL, &pathansi, &apsize);
    if (ptpath) {
        free(ptpath);
    }
    ptpath = NULL;
    SETERRNO(ret);
    return ret;
}


int get_desktop_session(void)
{
    DWORD dwsess;
    int ret;

    dwsess = WTSGetActiveConsoleSessionId();
    if (dwsess == 0xffffffff) {
        ret = -ERROR_NO_SUCH_LOGON_SESSION;
        goto fail;
    }
    SETERRNO(0);
    return (int)dwsess;
fail:
    SETERRNO(ret);
    return ret;
}

int win_arch_type()
{
    UINT uret;
    int ret = 0;
    int bufsize = 0;
    TCHAR* pBuf = NULL;

    bufsize = 1024;
try_again:
    if (pBuf) {
        free(pBuf);
    }
    pBuf = NULL;
    pBuf = (TCHAR*)malloc((size_t)bufsize);
    if (pBuf == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not alloc(%d) error(%d)", bufsize, ret);
        goto fail;
    }

    uret = GetSystemWow64Directory(pBuf, bufsize / sizeof(TCHAR));
    if (uret == 0) {
        GETERRNO(ret);
        if (ret == -ERROR_CALL_NOT_IMPLEMENTED) {
            goto iswin32;
        } else if (ret == -ERROR_INSUFFICIENT_BUFFER) {
            bufsize <<= 1;
            goto try_again;
        }
        ERROR_INFO("can not call get wow directory error(%d)", ret);
        goto fail;
    }
    if (pBuf) {
        free(pBuf);
    }
    bufsize = 0;
    return WIN64_ARCH;
iswin32:
    if (pBuf) {
        free(pBuf);
    }
    bufsize = 0;
    return WIN32_ARCH;
fail:
    if (pBuf) {
        free(pBuf);
    }
    bufsize = 0;
    SETERRNO(-ret);
    return ret;
}


int user_password_ok(const char* user, const char* password)
{
    BOOL bret;
    int ret=0;
    HANDLE hd=NULL;
    DWORD logontype[] = {LOGON32_LOGON_INTERACTIVE,LOGON32_LOGON_BATCH};
    int i;
    int succ=0;

    for (i=0;i<(sizeof(logontype) / sizeof(logontype[0])); i ++) {
        bret = LogonUserA(user,NULL,password,logontype[i],LOGON32_PROVIDER_DEFAULT,&hd);
        if (bret) {
            succ = 1;
            break;
        }
        GETERRNO(ret);
        if ((password == NULL || strlen(password) == 0 ) && ret == -ERROR_ACCOUNT_RESTRICTION) {
            succ = 1;
            break;
        }
    }

    if (succ == 0) {
        GETERRNO(ret);
        goto fail;
    }

    CloseHandle(hd);
    hd = NULL;

    return 0;
fail:
    SETERRNO(ret);
    return ret;
}



typedef NTSYSCALLAPI NTSTATUS  (NTAPI *NtSetSecurityObject_func_t)(HANDLE Handle,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR SecurityDescriptor);

typedef NTSYSCALLAPI NTSTATUS  (NTAPI *NtQuerySecurityObject_func_t)( _In_ HANDLE Handle,    _In_ SECURITY_INFORMATION SecurityInformation, _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,    _In_ ULONG Length,    _Out_ PULONG LengthNeeded);


static NtSetSecurityObject_func_t NtSetSecurityObject_orig = NULL;
static NtQuerySecurityObject_func_t NtQuerySecurityObject_orig = NULL;
static HMODULE ntdll_hdl=NULL;

int init_nt_envop_funcs(void)
{
    int ret;
    fini_nt_envop_funcs();
    ntdll_hdl = LoadLibraryA("ntdll.dll");
    if (ntdll_hdl == NULL) {
        GETERRNO(ret);
        ERROR_INFO("load ntdll.dll error[%d]", ret);
        goto fail;
    }

    NtSetSecurityObject_orig = reinterpret_cast<NtSetSecurityObject_func_t>(reinterpret_cast<void*>(::GetProcAddress(ntdll_hdl,"NtSetSecurityObject")));
    if (NtSetSecurityObject_orig == NULL) {
        GETERRNO(ret);
        ERROR_INFO("failed NtSetSecurityObject error[%d]", ret);
        goto fail;
    }

    NtQuerySecurityObject_orig = reinterpret_cast<NtQuerySecurityObject_func_t>(reinterpret_cast<void*>(::GetProcAddress(ntdll_hdl,"NtQuerySecurityObject")));
    if (NtQuerySecurityObject_orig == NULL) {
        GETERRNO(ret);
        ERROR_INFO("failed NtQuerySecurityObject error[%d]", ret);
        goto fail;
    }

    return 0;
fail:
    fini_nt_envop_funcs();
    SETERRNO(ret);
    return ret;
}

void fini_nt_envop_funcs(void)
{
    NtSetSecurityObject_orig = NULL;
    NtQuerySecurityObject_orig = NULL;
    if (ntdll_hdl != NULL) {
        FreeLibrary(ntdll_hdl);
    }
    ntdll_hdl = NULL;
    return ;
}


NTSTATUS
NTAPI
NtSetSecurityObjectFake(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    if (NtSetSecurityObject_orig == NULL) {
        return NTSTATUS_FLT_NOT_INITIALIZED;
    }
    return NtSetSecurityObject_orig(Handle,SecurityInformation,SecurityDescriptor);
}


NTSTATUS
NTAPI
NtQuerySecurityObjectFake(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    )
{
    if (NtQuerySecurityObject_orig == NULL) {
        return NTSTATUS_FLT_NOT_INITIALIZED;
    }
    return NtQuerySecurityObject_orig(Handle,SecurityInformation,SecurityDescriptor,Length,LengthNeeded);
}