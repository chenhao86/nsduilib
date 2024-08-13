#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4820)

#include <win_args.h>
#include <win_uniansi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <win_err.h>
#include <win_output_debug.h>
#include <win_strop.h>

#pragma warning(pop)

#if _MSC_VER >= 1910
#pragma warning(push)
/*disable Spectre warnings*/
#pragma warning(disable:5045)
#endif


void free_args(char*** pppargs)
{
    int i;
    char **ppargs;
    int lensize = 0;
    if (pppargs == NULL) {
        return;
    }

    ppargs = *pppargs;
    if (ppargs == NULL) {
        return ;
    }
    for (i = 0; ppargs[i]; i++) {
        lensize = (int)strlen(ppargs[i]);
        lensize ++;
        TcharToAnsi(NULL, &(ppargs[i]), &lensize);
    }

    free(ppargs);
    *pppargs = NULL;
    return;
}

char** copy_args(int argc, TCHAR *argv[])
{
    int i;
    int ret;
    char** ppargs = NULL;
    size_t argssize = 0;
    int cursize = 0;

    /*we include the end to indicate*/
    argssize = sizeof(ppargs[0]) * (argc + 1);

    ppargs = (char**) malloc(argssize);
    if (ppargs == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not alloc(%d) error(%d)", argssize, ret);
        goto fail;
    }
    memset(ppargs, 0, argssize);

    for (i = 0; i < argc; i++) {
        cursize = 0;
        ret = TcharToAnsi(argv[i], &(ppargs[i]), &cursize);
        if (ret < 0) {
            ERROR_INFO("can not convert[%d] args error(%d)", i, ret);
            goto fail;
        }
    }

    return ppargs;
fail:
    free_args(&ppargs);
    SETERRNO(-ret);
    return NULL;
}

int  parse_number(char* str, uint64_t *pnum, char** ppend)
{
    int ret;
    int base = 10;
    char* pcurptr = str;
    char* pretptr = str;
    uint64_t val;

    if (_strnicmp(str, "x", 1) == 0) {
        pcurptr ++;
        base = 16;
    } else if (_strnicmp(str, "0x", 2) == 0) {
        pcurptr += 2;
        base = 16;
    }

    val = strtoull(pcurptr, &pretptr, base);
    if (val == 0) {
        if (pretptr == pcurptr) {
            ret = -ERROR_INVALID_PARAMETER;
            goto fail;
        }
    } else if (val == ULLONG_MAX) {
        if (pretptr > (pcurptr + 32)) {
            ret = -ERROR_INVALID_PARAMETER;
            goto fail;
        }
    }

    *pnum = val;
    if (ppend) {
        *ppend = pretptr;
    }
    return 1;
fail:
    SETERRNO(-ret);
    return ret;
}

int  parse_int(char* str, int64_t* pnum, char**ppend)
{
    int ret;
    int base = 10;
    char* pcurptr = str;
    char* pretptr = str;
    int64_t val;


    val = strtoll(pcurptr, &pretptr, base);
    if (val == 0) {
        if (pretptr == pcurptr) {
            ret = -ERROR_INVALID_PARAMETER;
            goto fail;
        }
    } else if (val == ULLONG_MAX) {
        if (pretptr > (pcurptr + 32)) {
            ret = -ERROR_INVALID_PARAMETER;
            goto fail;
        }
    }

    *pnum = val;
    if (ppend) {
        *ppend = pretptr;
    }
    return 1;
fail:
    SETERRNO(-ret);
    return ret;
}

int  parse_long_double(char* str, long double *pdbl, char** ppend)
{
    int ret;
    long double retdbl;
    char* pretstr = NULL;

    if (pdbl == NULL || str == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    errno = 0;
    retdbl = strtold(str, &pretstr);
    if (str == pretstr) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    } else if (errno == ERANGE) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }
    *pdbl = retdbl;
    if (ppend) {
        *ppend = pretstr;
    }
    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

#define ADD_CPY_CHAR(c)                                                                           \
do{                                                                                               \
    if (cpylen >= cpysize || copystr == NULL) {                                                   \
        cpysize <<= 1;                                                                            \
        if (cpysize == 0) {                                                                       \
            cpysize = 4;                                                                          \
        }                                                                                         \
        ptmpcopy = (char*)malloc((size_t)cpysize);                                                \
        if (ptmpcopy == NULL) {                                                                   \
            GETERRNO(ret);                                                                        \
            ERROR_INFO("malloc [%d] error[%d]", cpysize, ret);                                    \
            goto fail;                                                                            \
        }                                                                                         \
        memset(ptmpcopy,0,(size_t)cpysize);                                                       \
        if (cpylen > 0) {                                                                         \
            memcpy(ptmpcopy, copystr, (size_t)cpylen);                                            \
        }                                                                                         \
        if (copystr != NULL && copystr != *pcopyback) {                                           \
            free(copystr);                                                                        \
        }                                                                                         \
        copystr = ptmpcopy;                                                                       \
        ptmpcopy = NULL;                                                                          \
    }                                                                                             \
    copystr[cpylen] = c;                                                                          \
    cpylen ++;                                                                                    \
}while(0)

int __find_quote_string(char* pstart, char** ppend,char**pcopyback, int *pcpysize, int quoted)
{
    char* pptr=pstart;
    char* copystr=*pcopyback;
    int cpylen=0;
    int cpysize=*pcpysize;
    char* ptmpcopy = NULL;
    int ret = 0;

    while(*pptr != '\0') {
        if (quoted > 0) {
            if ((*pptr == '\'' && quoted == 1) ||
                *pptr == '"' && quoted == 2) {
                if (ppend) {
                    *ppend = (pptr+1);
                }
                break;
            } 
            if (*pptr == '\\') {
                pptr ++;
                ADD_CPY_CHAR(*pptr);
            } else {
                ADD_CPY_CHAR(*pptr);
            }
        } else if (*pptr == ' ' || 
            *pptr == '\t') {
            if (ppend) {
                *ppend = pptr;
            }
            break;
        } else {
            ADD_CPY_CHAR(*pptr);
        }
        pptr ++;
    }

    if (quoted == 1 && *pptr != '\'' ||
        quoted == 2 && *pptr != '"') {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (quoted == 0) {
        if (ppend) {
            *ppend = pptr;
        }
    }

    if (*pcopyback && *pcopyback != copystr) {
        free(*pcopyback);
    }
    *pcopyback = copystr;
    *pcpysize = cpysize;
    return cpylen;
fail:
    if (copystr && copystr != *pcopyback) {
        free(copystr);
    }
    copystr = NULL;
    SETERRNO(ret);
    return ret;
}

int split_argv(char* pcmd,char***pppargv, int* pargc)
{
    int i;
    char** ppargv=NULL;
    int argc=0;
    int ret=0;
    int cnt = 0;
    char* pcurpatr=NULL;
    char* pstartptr=NULL;
    char* pendstr=NULL;
    int isquoted=0;
    char** pptmpargv=NULL;
    char* pcopystr = NULL;
    int cpysize = 0;
    if (pcmd == NULL) {
        if (pargc != NULL && pppargv != NULL && *pppargv != NULL) {
            ppargv = *pppargv;
            for (i=0;i<*pargc;i++) {
                if (ppargv[i]) {
                    free(ppargv[i]);
                    ppargv[i] = NULL;
                }
            }
            *pppargv = NULL;
        }

        if (pargc != NULL) {
            *pargc = 0;
        }
        return 0;
    }

    if (pppargv == NULL || pargc ==NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    ppargv = *pppargv;
    argc = *pargc;
    if (ppargv && argc > 0) {
        for (i=0;i<argc;i++) {
            if (ppargv[i]) {
                free(ppargv[i]);
                ppargv[i] = NULL;
            }
        }
    }

    pcurpatr = pcmd;
    while (*pcurpatr != '\0') {
        while(*pcurpatr == ' ' || *pcurpatr == '\t') {
            pcurpatr ++;
        }
        if (*pcurpatr == '\0') {
            break;
        }
        isquoted = 0;
        if (*pcurpatr == '\'' ||
            *pcurpatr == '"') {
            isquoted = 1;
            if (*pcurpatr == '"') {
                isquoted = 2;
            }
            pcurpatr ++;
        }

        pstartptr = pcurpatr;
        if (pcopystr) {
            memset(pcopystr,0, (size_t)cpysize);
        }
        ret = __find_quote_string(pstartptr,&pendstr,&pcopystr,&cpysize,isquoted);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
        if (ppargv == NULL || cnt >= argc) {
            if (cnt >= argc) {
                argc = cnt << 1;
                if (argc == 0) {
                    argc = 4;
                }
            }
            pptmpargv = (char**)malloc(sizeof(pptmpargv[0]) * argc);
            if (pptmpargv == NULL) {
                GETERRNO(ret);
                ERROR_INFO("can not alloc [%d] error[%d]", sizeof(pptmpargv[0]) * argc, ret);
                goto fail;
            }
            memset(pptmpargv, 0 , sizeof(pptmpargv[0]) * argc);
            for (i=0;i<cnt;i++) {
                pptmpargv[i] = ppargv[i];
            }
            if (ppargv != NULL && ppargv != *pppargv) {
                free(ppargv);
            }
            ppargv = pptmpargv;
            pptmpargv = NULL;
        }
        ppargv[cnt] = _strdup(pcopystr);
        if (ppargv[cnt] == NULL)  {
            GETERRNO(ret);
            ERROR_INFO("strdup [%s] error[%d]", pcopystr, ret);
            goto fail;
        }
        cnt ++;
        pcurpatr = pendstr;
    }

    if (pcopystr) {
        free(pcopystr);
    }
    pcopystr = NULL;

    if (*pppargv != NULL && *pppargv != ppargv) {
        free(*pppargv);
    }
    *pppargv = ppargv;
    *pargc = argc;
    return cnt;
fail:
    if (pcopystr) {
        free(pcopystr);
    }
    pcopystr = NULL;
    if (ppargv != NULL) {
        for (i=0;ppargv[i] != NULL ;i++) {
            if (ppargv[i] != NULL) {
                free(ppargv[i]);
                ppargv[i] = NULL;
            }
        }
    }

    if (ppargv != NULL && ppargv != *pppargv) {
        free(ppargv);       
    }
    ppargv = NULL;
    SETERRNO(ret);
    return ret;
}

char** copy_argv(char** argv)
{
    char** retargv=NULL , **pptmp=NULL;
    int i,llen=0;
    int size=4;
    int ret;

    retargv = (char**) malloc(sizeof(*retargv) * size);
    if (retargv == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    memset(retargv, 0 , sizeof(*retargv) * size);

    for (i=0;argv[i] != NULL;i++) {
        if (llen >= (size - 1)){
            size <<= 1;
            pptmp = (char**) malloc(sizeof(*pptmp) * size);
            if (pptmp == NULL) {
                GETERRNO(ret);
                goto fail;
            }
            memset(pptmp, 0, sizeof(*pptmp) *size);
            if (llen > 0) {
                memcpy(pptmp, retargv, sizeof(*pptmp) * llen);
            }
            if (retargv != NULL) {
                free(retargv);
            }
            retargv = pptmp;
            pptmp = NULL;
        }
        retargv[i] = _strdup(argv[i]);
        if (retargv[i] == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        llen ++;
    }
    return retargv;
fail:
    if (pptmp) {
        free(pptmp);
    }
    pptmp = NULL;
    free_argv(&retargv);
    SETERRNO(ret);
    return NULL;
}

void free_argv(char*** pppargv)
{
    char** argv;
    int i;
    if (pppargv && *pppargv != NULL) {
        argv = *pppargv;
        for(i=0;argv[i] != NULL;i++) {
            free(argv[i]);
            argv[i] = NULL;
        }
        free(argv);
        *pppargv = NULL;
    }
    return;
}

#if _MSC_VER >= 1910
#pragma warning(pop)
#endif