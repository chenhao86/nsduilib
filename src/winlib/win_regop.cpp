#include <win_regop.h>
#include <win_types.h>
#include <win_output_debug.h>
#include <win_uniansi.h>
#include <win_priv.h>
#include <tchar.h>


#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#define  REG_OP_MAGIC     0x448213
#pragma warning(disable:4996)

typedef struct _regop_t {
    uint32_t m_magic;
    uint32_t m_reserv1;
    HKEY m_reghdl;
    char* m_rootname;
    char* m_name;
} regop_t, *pregop_t;

#define GETLRET(ret,lret) do{ ret = lret; if (ret > 0) { ret = -ret ;} if (ret == 0 ){ret = -1;} } while(0)

void __close_regop(pregop_t* ppregop)
{
    if (ppregop != NULL && *ppregop != NULL) {
        pregop_t pregop =  * ppregop;
        if (pregop->m_magic == REG_OP_MAGIC) {
            LONG lret;
            if (pregop->m_reghdl != NULL) {
                lret = RegCloseKey(pregop->m_reghdl);
                if (lret != ERROR_SUCCESS) {
                    ERROR_INFO("can not close key (0x%p) error(%ld)", pregop->m_reghdl, lret);
                }
                pregop->m_reghdl = NULL;
            }
            if (pregop->m_name) {
                free(pregop->m_name);
            }
            pregop->m_name = NULL;

            if (pregop->m_rootname) {
                free(pregop->m_rootname);
            }
            pregop->m_rootname = NULL;

            free(pregop);
        } else {
            ERROR_INFO("0x%p not valid regop", pregop);
        }
        *ppregop = NULL;
    }
    return ;
}

void close_hklm(void** ppregop)
{
    return __close_regop((pregop_t*)ppregop);
}

void* __open_reg(HKEY hkey, const char* psubkey, REGSAM keyaccess)
{
    pregop_t pregop = NULL;
    int ret;
    LONG lret;
    TCHAR *ptsub = NULL;
    int tsubsize = 0;
    pregop = (pregop_t)malloc(sizeof(*pregop));
    if (pregop == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not alloc(%d) error(%d)", sizeof(*pregop), ret);
        goto fail;
    }
    memset(pregop, 0, sizeof(*pregop));
    pregop->m_magic = REG_OP_MAGIC;
    pregop->m_reghdl = NULL;
    pregop->m_name = NULL;
    pregop->m_name = _strdup(psubkey);
    if (pregop->m_name == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    if (hkey == HKEY_LOCAL_MACHINE) {
        pregop->m_rootname = _strdup("HKEY_LOCAL_MACHINE");
    } else if (hkey == HKEY_CLASSES_ROOT) {
        pregop->m_rootname = _strdup("HKEY_CLASSES_ROOT");
    } else if (hkey == HKEY_CURRENT_CONFIG) {
        pregop->m_rootname = _strdup("HKEY_CURRENT_CONFIG");
    } else if (hkey == HKEY_CURRENT_USER) {
        pregop->m_rootname = _strdup("HKEY_CURRENT_USER");
    } else if (hkey == HKEY_USERS) {
        pregop->m_rootname = _strdup("HKEY_USERS");
    } else if (hkey == HKEY_PERFORMANCE_DATA) {
        pregop->m_rootname = _strdup("HKEY_PERFORMANCE_DATA");
    } else if (hkey == HKEY_PERFORMANCE_NLSTEXT) {
        pregop->m_rootname = _strdup("HKEY_PERFORMANCE_NLSTEXT");
    } else {
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("not valid hkey [%p]", hkey);
        goto fail;
    }


    if (pregop->m_rootname == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToTchar(psubkey, &ptsub, &tsubsize);
    if (ret < 0) {
        goto fail;
    }

    lret = RegOpenKeyEx(hkey, ptsub, 0, keyaccess, &(pregop->m_reghdl));
    if (lret != ERROR_SUCCESS) {
        GETLRET(ret, lret);
        if (ret != -ERROR_FILE_NOT_FOUND) {
            ERROR_INFO("can not open([%s].%s) with access (0x%x) error(%d)",pregop->m_rootname, psubkey, keyaccess, ret);
        }
        goto fail;
    }

    AnsiToTchar(NULL, &ptsub, &tsubsize);
    return pregop;
fail:
    AnsiToTchar(NULL, &ptsub, &tsubsize);
    __close_regop(&pregop);
    SETERRNO(-ret);
    return NULL;
}

HKEY __name_to_hkey(const char* pkeyname)
{
    HKEY hkey = NULL;
    int ret=0;
    if (_stricmp(pkeyname, "HKEY_LOCAL_MACHINE") == 0) {
        hkey = HKEY_LOCAL_MACHINE;
    } else if (_stricmp(pkeyname, "HKEY_CLASSES_ROOT") == 0) {
        hkey = HKEY_CLASSES_ROOT;
    } else if (_stricmp(pkeyname, "HKEY_CURRENT_CONFIG") == 0) {
        hkey = HKEY_CURRENT_CONFIG;
    } else if (_stricmp(pkeyname, "HKEY_CURRENT_USER") == 0) {
        hkey = HKEY_CURRENT_USER;
    } else if (_stricmp(pkeyname, "HKEY_USERS") == 0) {
        hkey = HKEY_USERS;
    } else if (_stricmp(pkeyname, "HKEY_PERFORMANCE_DATA") == 0) {
        hkey = HKEY_PERFORMANCE_DATA;
    } else if (_stricmp(pkeyname, "HKEY_PERFORMANCE_NLSTEXT") == 0) {
        hkey = HKEY_PERFORMANCE_NLSTEXT;
    } else {
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("not vali keyname [%s]", pkeyname);
        goto fail;
    }

    return hkey;
fail:
    SETERRNO(ret);
    return NULL;
}

REGSAM __access_to_regsam(int accessmode)
{
    REGSAM regaccess = 0;
    if (accessmode & ACCESS_KEY_READ) {
        regaccess |= KEY_READ;
    }

    if (accessmode & ACCESS_KEY_WRITE) {
        regaccess |= KEY_WRITE;
    }

    if (accessmode & ACCESS_KEY_ALL) {
        regaccess = KEY_ALL_ACCESS ;
    }

    return regaccess;
}

void* open_reg_key(const char* pkeyname, const char* psubkey, int accessmode)
{
    HKEY hkey = NULL;
    int ret;
    REGSAM regaccess = 0;


    hkey = __name_to_hkey(pkeyname);
    if (hkey == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    regaccess = __access_to_regsam(accessmode);

    return __open_reg(hkey, psubkey, regaccess);
fail:
    SETERRNO(ret);
    return NULL;
}


void* open_hklm(const char* psubkey, int accessmode)
{
    REGSAM regaccess = 0;
    if (accessmode & ACCESS_KEY_READ) {
        regaccess |= KEY_READ;
    }

    if (accessmode & ACCESS_KEY_WRITE) {
        regaccess |= KEY_WRITE;
    }

    if (accessmode & ACCESS_KEY_ALL) {
        regaccess = KEY_ALL_ACCESS ;
    }
    return __open_reg(HKEY_LOCAL_MACHINE, psubkey, regaccess);
}

int __query_key_value(pregop_t pregop, const char* path, LPDWORD lptype, LPBYTE pdata, int datasize)
{
    DWORD retdatasize = 0;
    int ret;
    LONG lret;
    TCHAR* ptpath = NULL;
    int tpathsize = 0;

    ret = AnsiToTchar(path, &ptpath, &tpathsize);
    if (ret < 0) {
        goto fail;
    }

    retdatasize = (DWORD)datasize;
    lret = RegQueryValueEx(pregop->m_reghdl, ptpath, NULL, lptype, pdata, &retdatasize);
    if (lret != ERROR_SUCCESS) {
        GETLRET(ret, lret);
        if (ret != -ERROR_MORE_DATA) {
            ERROR_INFO("can not query (%s) error(%d)", path, ret);
        }
        goto fail;
    }

    AnsiToTchar(NULL, &ptpath, &tpathsize);
    return (int)retdatasize;
fail:
    AnsiToTchar(NULL, &ptpath, &tpathsize);
    SETERRNO(-ret);
    return ret;
}

int __set_key_value(pregop_t pregop, const char* path, DWORD regtype, void* pdata, int datasize)
{
    TCHAR* ptpath = NULL;
    int tpathsize = 0;
    LONG lret;
    int ret;

    ret = AnsiToTchar(path, &ptpath, &tpathsize);
    if (ret < 0) {
        goto fail;
    }

    lret = RegSetValueEx(pregop->m_reghdl, ptpath, 0, regtype, (const BYTE*)pdata, (DWORD)datasize);
    if (lret != ERROR_SUCCESS) {
        GETLRET(ret, lret);
        ERROR_INFO("set [%s] value error[%d]", path, ret);
        goto fail;
    }

    AnsiToTchar(NULL, &ptpath, &tpathsize);

    return datasize;
fail:
    AnsiToTchar(NULL, &ptpath, &tpathsize);
    SETERRNO(ret);
    return ret;
}


int query_hklm_string(void* pregop, const char* path, char** ppretval, int *pretsize)
{
    pregop_t pinner = (pregop_t) pregop;
    TCHAR *ptval = NULL;
    int tvalsize = 0;
    char* pansival = NULL;
    int ansivalsize = 0;
    int nret;
    DWORD regtype = REG_EXPAND_SZ;
    char* pretval = NULL;
    int retvalsize = 0;
    int ret;

    if (path == NULL) {
        if (ppretval && *ppretval) {
            free(*ppretval);
            *ppretval = NULL;
        }
        if (pretsize) {
            *pretsize = 0;
        }
        return 0;
    }

    if (ppretval == NULL || pretsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretval = *ppretval;
    retvalsize = *pretsize;

    if (pinner == NULL || pinner->m_magic != REG_OP_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    tvalsize = 1024;
try_again:
    if (ptval) {
        free(ptval);
    }
    ptval = NULL;
    ptval = (TCHAR*)malloc((size_t)tvalsize);
    if (ptval == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not alloc(%d) error(%d)", tvalsize, ret);
        goto fail;
    }

    ret = __query_key_value(pinner, path, &regtype, (LPBYTE)ptval, tvalsize);
    if (ret < 0) {
        if (ret == -ERROR_MORE_DATA) {
            tvalsize <<= 1;
            goto try_again;
        }
        goto fail;
    }

    /*now all is ok*/
    ret = TcharToAnsi(ptval, &pansival, &ansivalsize);
    if (ret < 0) {
        goto fail;
    }
    nret = (int)strlen(pansival) + 1;

    if (retvalsize < nret || pretval == NULL) {
        if (retvalsize < nret) {
            retvalsize = nret;
        }
        pretval = (char*)malloc((size_t)retvalsize);
        if (pretval == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc(%d) error(%d)", retvalsize, ret);
            goto fail;
        }
    }

    strncpy_s(pretval, (size_t)nret, pansival, (size_t)nret);
    if (*ppretval && *ppretval != pretval) {
        free(*ppretval);
    }
    *ppretval = pretval;
    *pretsize = retvalsize;

    TcharToAnsi(NULL, &pansival, &ansivalsize);
    if (ptval) {
        free(ptval);
    }
    ptval = NULL;
    tvalsize = 0;
    return nret;
fail:
    TcharToAnsi(NULL, &pansival, &ansivalsize);
    if (ptval) {
        free(ptval);
    }
    ptval = NULL;
    tvalsize = 0;
    SETERRNO(-ret);
    return ret;
}


int query_hklm_binary(void* pregop, const char* path, void** ppdata, int *pdatasize)
{
    pregop_t pinner = (pregop_t) pregop;
    void *ptval = NULL;
    int tvalsize = 0;
    int nret;
    DWORD regtype ;
    void* pretval = *ppdata;
    int retsize = *pdatasize;
    int ret;

    if (path == NULL) {
        if (ppdata != NULL && (*ppdata) != NULL) {
            free(*ppdata);
            *ppdata = NULL;
        }
        if (pdatasize) {
            *pdatasize = 0;
        }
        return 0;
    }

    if (ppdata == NULL || pdatasize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretval = *ppdata;
    retsize = *pdatasize;

    if (pinner == NULL || pinner->m_magic != REG_OP_MAGIC) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    tvalsize = 1024;
try_again:
    if (ptval) {
        free(ptval);
    }
    ptval = NULL;
    ptval = malloc((size_t)tvalsize);
    if (ptval == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not alloc(%d) error(%d)", tvalsize, ret);
        goto fail;
    }

    regtype = REG_BINARY;
    ret = __query_key_value(pinner, path, &regtype, (LPBYTE)ptval, tvalsize);
    if (ret < 0) {
        if (ret == -ERROR_MORE_DATA) {
            tvalsize <<= 1;
            goto try_again;
        }
        goto fail;
    }

    if (regtype != REG_BINARY) {
        ret = -ERROR_INVALID_DATATYPE;
        ERROR_INFO("[%s] not valid type [0x%lx:%ld]", path, regtype, regtype);
        goto fail;
    }
    nret = ret;

    if (retsize < nret || pretval != NULL) {
        if (pretval != NULL && pretval != *ppdata) {
            free(pretval);
        }
        pretval = NULL;
        retsize = nret;
        pretval = malloc((size_t)nret);
        if (pretval == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc %d error[%d]", nret, ret);
            goto fail;
        }
    }
    memcpy(pretval, ptval, (size_t)nret);

    if (*ppdata && *ppdata != pretval) {
        free(*ppdata);
    }
    *ppdata = pretval;
    *pdatasize = retsize;

    if (ptval != NULL) {
        free(ptval);
    }
    ptval = NULL;

    return nret;
fail:
    if (pretval != NULL && pretval != *ppdata) {
        free(pretval);
    }
    pretval = NULL;
    retsize = 0;

    if (ptval) {
        free(ptval);
    }
    ptval = NULL;
    tvalsize = 0;
    SETERRNO(-ret);
    return ret;
}


int set_hklm_binary(void* pregop1, const char* path, void* pdata, int size)
{
    pregop_t pregop = (pregop_t) pregop1;
    int ret;
    if (pdata == NULL || pregop == NULL  || pregop->m_magic != REG_OP_MAGIC || path == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    return __set_key_value(pregop, path, REG_BINARY, pdata, size);
}

int __inner_set_reg_sz(void* pregop1, const char* path, char* valstr)
{
    TCHAR* ptval = NULL;
    int valsize = 0;
    int vallen = 0;
    int ret;
    int nret;
    pregop_t pregop = (pregop_t) pregop1;

    if (pregop == NULL  || pregop->m_magic != REG_OP_MAGIC || path == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }


    ret = AnsiToTchar(valstr, &ptval, &valsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    vallen = (int)_tcslen(ptval);
    ret = __set_key_value(pregop, path, REG_SZ, ptval, (int)((vallen + 1) * sizeof(TCHAR)));
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    nret = ret;


    AnsiToTchar(NULL, &ptval, &valsize);
    return nret;
fail:
    AnsiToTchar(NULL, &ptval, &valsize);
    SETERRNO(ret);
    return ret;    
}

int set_hklm_string(void* pregop1, const char* path, char* valstr)
{
    return __inner_set_reg_sz(pregop1,path,valstr);
}

int set_hklm_sz(void* pregop1, const char* path, char* valstr)
{
    TCHAR* ptval = NULL;
    int valsize = 0;
    int vallen = 0;
    int ret;
    int nret;
    pregop_t pregop = (pregop_t) pregop1;

    if (pregop == NULL  || pregop->m_magic != REG_OP_MAGIC || path == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }


    ret = AnsiToTchar(valstr, &ptval, &valsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    vallen = (int)_tcslen(ptval);
    ret = __set_key_value(pregop, path, REG_SZ, ptval, (int)((vallen + 1) * sizeof(TCHAR)));
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    nret = ret;


    AnsiToTchar(NULL, &ptval, &valsize);
    return nret;
fail:
    AnsiToTchar(NULL, &ptval, &valsize);
    SETERRNO(ret);
    return ret;
}

int enum_hklm_keys(void* pregop1, char*** pppitems, int* psize)
{
    int i;
    pregop_t pregop = (pregop_t) pregop1;
    char** ppretitems = NULL;
    char** pptmp = NULL;
    int retsize = 0;
    TCHAR* ptname = NULL;
    int tnamesize = 2;
    DWORD retnamesize = 0;
    LSTATUS status;
    DWORD di;
    char* ansistr = NULL;
    int ansisize = 0;
    int ret;
    int retlen = 0;
    TCHAR* clsname = NULL;
    DWORD clssize = 0, retclssize = 0;
    DWORD keysize = 0, maxkeylen = 0, maxclslen = 0, valsize = 0, maxvalnamelen = 0, maxvallen = 0;


    if (pregop1 == NULL) {
        if (pppitems && *pppitems) {
            char** ppitmes = (*pppitems);
            for (i = 0; ppitmes[i] != NULL ; i++) {
                free(ppitmes[i]);
                ppitmes[i] = NULL;
            }
            free(*pppitems);
            *pppitems = NULL;
        }
        if (psize) {
            *psize = 0;
        }
        return 0;
    }

    if (pppitems == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ppretitems = *pppitems;
    retsize = *psize;

    for (i = 0; i < retsize && ppretitems != NULL; i++) {
        if (ppretitems[i] != NULL) {
            free(ppretitems[i]);
            ppretitems[i] = NULL;
        }
    }

    clssize = MAX_PATH;
    clsname = (TCHAR*) malloc(sizeof(clsname[0]) * clssize);
    if (clsname == NULL) {
        GETERRNO(ret);
        goto fail;
    }


    retclssize = clssize;
    status = RegQueryInfoKey(pregop->m_reghdl, clsname, &retclssize,
                             NULL, &keysize, &maxkeylen, &maxclslen, &valsize, &maxvalnamelen, &maxvallen, NULL, NULL);
    if (status != ERROR_SUCCESS) {
        GETERRNO(ret);
        ERROR_INFO("query info error[%ld][%d]", status, ret);
        goto fail;
    }

    DEBUG_INFO("[%s] keysize[%ld] maxkeylen [%ld] maxclslen [%ld] valsize [%ld] maxvalnamelen [%ld] maxvallen [%ld]",
               pregop->m_name, keysize, maxkeylen, maxclslen, valsize, maxvalnamelen, maxvallen);

    tnamesize = (int)(maxkeylen + 1);
    ptname = (TCHAR*)malloc(sizeof(ptname[0]) * tnamesize);
    if (ptname == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    for (di = 0; di < keysize; di++) {
        retnamesize = (DWORD)tnamesize;
        status = RegEnumKeyEx(pregop->m_reghdl,
                              di, ptname, &retnamesize, NULL, NULL, NULL, NULL);
        if (status != ERROR_SUCCESS) {
            GETERRNO(ret);
            ERROR_INFO("enum [%d] error[%d]", di, ret);
            goto fail;
        }

        /*
            now to get the name
            to make di + 1 for it will let the last element NULL
        */
        if (retsize <= (int)(di + 1)) {
            if (retsize == 0) {
                retsize = 4;
            } else {
                retsize <<= 1;
            }

            pptmp = (char**) malloc(sizeof(pptmp[0]) * retsize);
            if (pptmp == NULL) {
                GETERRNO(ret);
                goto fail;
            }
            memset(pptmp, 0, sizeof(pptmp[0]) * retsize);
            if (retlen > 0) {
                memcpy(pptmp, ppretitems, sizeof(pptmp[0]) * retlen);
            }

            if (ppretitems != NULL && ppretitems != *pppitems) {
                free(ppretitems);
            }
            ppretitems = pptmp;
            pptmp = NULL;
        }

        ret = TcharToAnsi(ptname, &ansistr, &ansisize);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
        ASSERT_IF(ppretitems[retlen] == NULL) ;
        ppretitems[retlen] = _strdup(ansistr);
        if (ppretitems[retlen] == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        retlen ++;
    }





    TcharToAnsi(NULL, &ansistr, &ansisize);
    if (pptmp) {
        free(pptmp);
    }
    pptmp = NULL;

    if (ptname) {
        free(ptname);
    }
    ptname = NULL;

    if (clsname) {
        free(clsname);
    }
    clsname = NULL;

    if (*pppitems && *pppitems != ppretitems) {
        free(*pppitems);
    }

    *pppitems = ppretitems;
    *psize = retsize;
    return retlen;
fail:
    TcharToAnsi(NULL, &ansistr, &ansisize);
    if (pptmp) {
        free(pptmp);
    }
    pptmp = NULL;

    if (ptname) {
        free(ptname);
    }
    ptname = NULL;

    if (clsname) {
        free(clsname);
    }
    clsname = NULL;

    if (ppretitems) {
        for (i = 0; ppretitems[i] != NULL; i++) {
            free(ppretitems[i]);
            ppretitems[i] = NULL;
        }
    }
    if (ppretitems != NULL && ppretitems != *pppitems)  {
        free(ppretitems);
    }
    ppretitems = NULL;

    SETERRNO(ret);
    return ret;
}

int enum_hklm_values(void* pregop1, char*** pppitems, int* psize)
{
    int i;
    pregop_t pregop = (pregop_t) pregop1;
    char** ppretitems = NULL;
    char** pptmp = NULL;
    int retsize = 0;
    TCHAR* ptname = NULL;
    int tnamesize = 2;
    DWORD retnamesize = 0;
    LSTATUS status;
    DWORD di;
    char* ansistr = NULL;
    int ansisize = 0;
    int ret;
    int retlen = 0;
    TCHAR* clsname = NULL;
    DWORD clssize = 0, retclssize = 0;
    DWORD keysize = 0, maxkeylen = 0, maxclslen = 0, valsize = 0, maxvalnamelen = 0, maxvallen = 0;


    if (pregop1 == NULL) {
        if (pppitems && *pppitems) {
            char** ppitmes = (*pppitems);
            for (i = 0; ppitmes[i] != NULL ; i++) {
                free(ppitmes[i]);
                ppitmes[i] = NULL;
            }
            free(*pppitems);
            *pppitems = NULL;
        }
        if (psize) {
            *psize = 0;
        }
        return 0;
    }

    if (pppitems == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ppretitems = *pppitems;
    retsize = *psize;

    for (i = 0; i < retsize && ppretitems != NULL; i++) {
        if (ppretitems[i] != NULL) {
            free(ppretitems[i]);
            ppretitems[i] = NULL;
        }
    }

    clssize = MAX_PATH;
    clsname = (TCHAR*) malloc(sizeof(clsname[0]) * clssize);
    if (clsname == NULL) {
        GETERRNO(ret);
        goto fail;
    }


    retclssize = clssize;
    status = RegQueryInfoKey(pregop->m_reghdl, clsname, &retclssize,
                             NULL, &keysize, &maxkeylen, &maxclslen, &valsize, &maxvalnamelen, &maxvallen, NULL, NULL);
    if (status != ERROR_SUCCESS) {
        GETERRNO(ret);
        ERROR_INFO("query info error[%ld][%d]", status, ret);
        goto fail;
    }

    DEBUG_INFO("[%s] keysize[%ld] maxkeylen [%ld] maxclslen [%ld] valsize [%ld] maxvalnamelen [%ld] maxvallen [%ld]",
               pregop->m_name, keysize, maxkeylen, maxclslen, valsize, maxvalnamelen, maxvallen);

    tnamesize = (int)(maxvalnamelen + 1);
    ptname = (TCHAR*)malloc(sizeof(ptname[0]) * tnamesize);
    if (ptname == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    for (di = 0; di < valsize; di++) {
        retnamesize = (DWORD)tnamesize;
        status = RegEnumValue(pregop->m_reghdl,
                              di, ptname, &retnamesize, NULL, NULL, NULL, NULL);
        if (status != ERROR_SUCCESS) {
            GETERRNO(ret);
            ERROR_INFO("enum [%d] error[%d]", di, ret);
            goto fail;
        }

        /*
            now to get the name
            to plus 1 for it will make SURE the last one is NULL
        */
        if (retsize <= (int)(di + 1)) {
            if (retsize == 0) {
                retsize = 4;
            } else {
                retsize <<= 1;
            }

            pptmp = (char**) malloc(sizeof(pptmp[0]) * retsize);
            if (pptmp == NULL) {
                GETERRNO(ret);
                goto fail;
            }
            memset(pptmp, 0, sizeof(pptmp[0]) * retsize);
            if (retlen > 0) {
                memcpy(pptmp, ppretitems, sizeof(pptmp[0]) * retlen);
            }

            if (ppretitems != NULL && ppretitems != *pppitems) {
                free(ppretitems);
            }
            ppretitems = pptmp;
            pptmp = NULL;
        }

        ret = TcharToAnsi(ptname, &ansistr, &ansisize);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
        ASSERT_IF(ppretitems[retlen] == NULL) ;
        ppretitems[retlen] = _strdup(ansistr);
        if (ppretitems[retlen] == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        retlen ++;
    }

    TcharToAnsi(NULL, &ansistr, &ansisize);
    if (pptmp) {
        free(pptmp);
    }
    pptmp = NULL;

    if (ptname) {
        free(ptname);
    }
    ptname = NULL;

    if (clsname) {
        free(clsname);
    }
    clsname = NULL;

    if (*pppitems && *pppitems != ppretitems) {
        free(*pppitems);
    }

    *pppitems = ppretitems;
    *psize = retsize;
    return retlen;
fail:
    TcharToAnsi(NULL, &ansistr, &ansisize);
    if (pptmp) {
        free(pptmp);
    }
    pptmp = NULL;

    if (ptname) {
        free(ptname);
    }
    ptname = NULL;

    if (clsname) {
        free(clsname);
    }
    clsname = NULL;

    if (ppretitems) {
        for (i = 0; ppretitems[i] != NULL; i++) {
            free(ppretitems[i]);
            ppretitems[i] = NULL;
        }
    }
    if (ppretitems != NULL && ppretitems != *pppitems)  {
        free(ppretitems);
    }
    ppretitems = NULL;

    SETERRNO(ret);
    return ret;
}

int delete_hklm_value(void* pregop1, const char* path)
{
    pregop_t pregop = (pregop_t) pregop1;
    TCHAR* ptname = NULL;
    int namesize = 0;
    LSTATUS lret;
    int ret;


    if (pregop1 == NULL || path == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ret = AnsiToTchar(path, &ptname, &namesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    SETERRNO(0);
    lret = RegDeleteValue(pregop->m_reghdl, ptname);
    if (lret != ERROR_SUCCESS) {
        GETERRNO_DIRECT(ret);
        if (ret != 0 && lret != 2) {
            ERROR_INFO("can not delete [%s].[%s] error[%d] [%ld]", pregop->m_name, path, ret, lret);
            goto fail;
        }
    }

    AnsiToTchar(NULL, &ptname, &namesize);
    return 0;
fail:
    AnsiToTchar(NULL, &ptname, &namesize);
    SETERRNO(ret);
    return ret;
}

int delete_reg_value(void* pregop1, const char* path)
{
    return delete_hklm_value(pregop1,path);
}

int delete_reg_key(void* pregop1, const char* psubkey)
{
    pregop_t pregop = (pregop_t) pregop1;
    int ret;
    TCHAR* ptkeyname = NULL;
    int tnamesize = 0;
    LSTATUS lret;

    if (pregop == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ret = AnsiToTchar(psubkey, &ptkeyname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    SETERRNO(0);
    lret = RegDeleteTree(pregop->m_reghdl, ptkeyname);
    if (lret != ERROR_SUCCESS) {
        GETERRNO_DIRECT(ret);
        DEBUG_INFO("delete [%s].[%s] lret [%ld] ret [%d]", pregop->m_rootname, psubkey, lret,ret);
        if (lret != 2 && ret != 0) {
            ERROR_INFO("can not delete [%s].[%s] error[%d] [%ld]", pregop->m_rootname, psubkey, ret, lret);
            goto fail;
        }
    }

    AnsiToTchar(NULL, &ptkeyname, &tnamesize);
    return 1;
fail:
    AnsiToTchar(NULL, &ptkeyname, &tnamesize);
    SETERRNO(ret);
    return ret;
}

int set_hklm_dword(void* pregop1, const char* path, uint32_t value)
{
    pregop_t pregop = (pregop_t) pregop1;
    int ret;
    if (pregop == NULL  || pregop->m_magic != REG_OP_MAGIC || path == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    return __set_key_value(pregop, path, REG_DWORD, &value, sizeof(value));
}

int query_hklm_dword(void* pregop1,const char* path,uint32_t* pvalue)
{
    int ret;
    DWORD regtype ;
    pregop_t pregop = (pregop_t) pregop1;
    
    regtype = REG_DWORD;
    ret = __query_key_value(pregop,path,&regtype, (LPBYTE)pvalue,sizeof(*pvalue));
    if (ret < 0) {
        return ret;
    }
    if (regtype != REG_DWORD) {
        ret = -ERROR_INVALID_DATATYPE;
        SETERRNO(ret);
        return ret;
    }
    return ret;
}


void* __create_reg(HKEY hkey, const char* psubkey, REGSAM keyaccess)
{
    char* path =NULL;
    int pathsize=0;
    TCHAR* tpath=NULL;
    int tpathsize=0;
    char* pcurptr=NULL;
    int namesize = 0;
    int ret;
    LSTATUS lret;
    HKEY nkey = NULL;

    pathsize = (int)strlen(psubkey) + 1;
    path = (char*)malloc((size_t)pathsize);
    if (path == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    pcurptr = (char*)psubkey;
    while(1) {
        if ((*pcurptr == '\\' || *pcurptr == 0x0) && namesize > 0) {
            memset(path,0,(size_t)pathsize);
            /*now we should give the name */
            memcpy(path,psubkey,(size_t)namesize);
            ret = AnsiToTchar(path,&tpath,&tpathsize);
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
            if (nkey != NULL) {
                RegCloseKey(nkey);
            }
            nkey = NULL;
            DEBUG_INFO("path [%s]",path);
            lret = RegCreateKey(hkey,tpath,&nkey);
            if (lret != ERROR_SUCCESS) {
                GETERRNO(ret);
                ERROR_INFO("create [%s] error %ld %d",path,lret,ret);
                goto fail;
            }

            RegCloseKey(nkey);
            nkey = NULL;

            if (*pcurptr == 0x0) {
                break;
            }
        }
        pcurptr += 1;
        namesize += 1;
    }

    if (nkey != NULL) {
        RegCloseKey(nkey);
    }
    nkey = NULL;

    AnsiToTchar(NULL,&tpath,&tpathsize);
    if (path) {
        free(path);
    }
    path = NULL;

    return __open_reg(hkey,psubkey,keyaccess);
fail:
    if (nkey != NULL) {
        RegCloseKey(nkey);
    }
    nkey = NULL;

    AnsiToTchar(NULL,&tpath,&tpathsize);
    if (path) {
        free(path);
    }
    path = NULL;
    SETERRNO(ret);
    return NULL;
}

void* create_reg_key(const char* pkeyname,const char* psubkey,int accessmode)
{
    void* pregop = NULL;
    HKEY hkey = NULL;
    REGSAM regaccess = 0;
    int ret;
    
    pregop = open_reg_key(pkeyname,pkeyname,accessmode);
    if (pregop != NULL) {
        return pregop;
    }

    hkey = __name_to_hkey(pkeyname);
    if (hkey == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    regaccess = __access_to_regsam(accessmode);
    return __create_reg(hkey,psubkey, regaccess);
fail:
    close_reg_key(&pregop);
    SETERRNO(ret);
    return NULL;

}

void close_reg_key(void** ppregop)
{
    __close_regop((pregop_t*)ppregop);
    return;
}

int set_reg_sz(void* pregop1, const char* path, char* valstr)
{
    return __inner_set_reg_sz(pregop1,path,valstr);
}

int exist_reg_key(const char* pkeyname,const char* psubkey)
{
    void* pregop = NULL;
    HKEY hkey = NULL;
    REGSAM regaccess = 0;    

    hkey = __name_to_hkey(pkeyname);
    if (hkey == NULL) {
        return 0;
    }
    regaccess = __access_to_regsam(ACCESS_KEY_READ);
    pregop = __open_reg(hkey,psubkey,regaccess);
    if (pregop != NULL) {
        close_reg_key(&pregop);
        return 1;
    }
    return 0;
}

int save_hive(char* file,char* keyname,char* subkey)
{
    HKEY hkey;
    int ret;
    TCHAR* tfile=NULL;
    int tfsize=0;
    LSTATUS lret;
    int enblbackup = 0;
    pregop_t pregop = NULL;
    REGSAM regaccess = 0;

    hkey = __name_to_hkey(keyname);
    if (hkey == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    regaccess = __access_to_regsam(ACCESS_KEY_READ);

    pregop = (pregop_t)__open_reg(hkey,subkey,regaccess);
    if (pregop == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToTchar(file,&tfile,&tfsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = enable_backup_priv();
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    enblbackup = 1;


    lret = RegSaveKey(pregop->m_reghdl,tfile,NULL);
    if (lret != ERROR_SUCCESS) {
        GETERRNO(ret);
        ERROR_INFO("save [%s].[%s] => [%s] error[%d] lret[%d]",keyname,subkey,file,ret,lret);
        goto fail;
    }


    if (enblbackup != 0) {
        disable_backup_priv();
    }
    enblbackup = 0;

    AnsiToTchar(NULL,&tfile,&tfsize);
    __close_regop(&pregop);
    return 0;
fail:
    if (enblbackup != 0) {
        disable_backup_priv();
    }
    enblbackup = 0;
    AnsiToTchar(NULL,&tfile,&tfsize);
    __close_regop(&pregop);
    SETERRNO(ret);
    return ret;
}

int load_hive(char* file,char* keyname, char* subkey)
{
    TCHAR* tfile=NULL;
    int tfsize=0;
    TCHAR* tsub=NULL;
    int tsubsize=0;
    int ret;
    LSTATUS lret;
    int enblrestore=0;
    int enblbackup=0;
    int enbldbg=0;
    HKEY hkey = NULL;

    if (file == NULL || keyname == NULL || subkey == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ret = AnsiToTchar(file,&tfile,&tfsize);
    if (ret <0) {
        GETERRNO(ret);
        goto fail;
    }

    hkey = __name_to_hkey(keyname);
    if (hkey == NULL) {
        GETERRNO(ret);
        goto fail;
    }


    ret = AnsiToTchar(subkey,&tsub,&tsubsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = enable_restore_priv();
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    enblrestore = 1;

    ret = enable_backup_priv();
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    enblbackup = 1;

    ret = enable_debug_priv();
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    enbldbg = 1;

    lret = RegLoadKey(hkey,tsub,tfile);
    if (lret != ERROR_SUCCESS) {
        GETERRNO(ret);
        ERROR_INFO("RegLoadKey [%s] => [%s].[%s] error [%d] lret [%d]",file,keyname,subkey,ret,lret);
        goto fail;
    }


    if (enbldbg != 0) {
        disable_debug_priv();
    }
    enbldbg = 0;

    if (enblbackup != 0) {
        disable_backup_priv();
    }
    enblbackup = 0;
    if (enblrestore != 0) {
        disable_restore_priv();
    }
    enblrestore =  0;
    AnsiToTchar(NULL,&tsub,&tsubsize);
    AnsiToTchar(NULL,&tfile,&tfsize);


    return 0;
fail:
    if (enbldbg != 0) {
        disable_debug_priv();
    }
    enbldbg = 0;

    if (enblbackup != 0) {
        disable_backup_priv();
    }
    enblbackup = 0;
    if (enblrestore != 0) {
        disable_restore_priv();
    }
    enblrestore =  0;
    AnsiToTchar(NULL,&tsub,&tsubsize);
    AnsiToTchar(NULL,&tfile,&tfsize);
    SETERRNO(ret);
    return ret;
}

int unload_hive(char* keyname, char* subkey)
{
    TCHAR* tsub =NULL;
    int tsubsize=0;
    int ret;
    int enblbackup = 0;
    int enblrestore = 0;
    LSTATUS lret;
    HKEY hkey = NULL;

    if (keyname == NULL || subkey == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ret = AnsiToTchar(subkey,&tsub,&tsubsize);
    if (ret <0 ) {
        GETERRNO(ret);
        goto fail;
    }

    hkey = __name_to_hkey(keyname);
    if (hkey == NULL) {
        GETERRNO(ret);
        goto fail;
    }



    ret = enable_restore_priv();
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    enblrestore = 1;

    ret = enable_backup_priv();
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    enblbackup = 1;

    lret = RegUnLoadKey(hkey,tsub);
    if (lret != ERROR_SUCCESS) {
        GETERRNO(ret);
        ERROR_INFO("Unload [%s].[%s] error[%d] lret[%d]",keyname,subkey,ret,lret);
        goto fail;
    }

    if (enblbackup != 0) {
        disable_backup_priv();
    }
    enblbackup = 0;
    if (enblrestore != 0) {
        disable_restore_priv();
    }
    enblrestore =  0;
    AnsiToTchar(NULL,&tsub,&tsubsize);

    return 0;
fail:
    if (enblbackup != 0) {
        disable_backup_priv();
    }
    enblbackup = 0;
    if (enblrestore != 0) {
        disable_restore_priv();
    }
    enblrestore =  0;
    AnsiToTchar(NULL,&tsub,&tsubsize);
    SETERRNO(ret);
    return ret;
}