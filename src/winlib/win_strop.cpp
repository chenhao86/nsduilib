
#include <win_output_debug.h>
#include <win_strop.h>
#include <win_err.h>
#include <win_types.h>

#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4820)
#pragma warning(disable:4514)

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#pragma warning(pop)

#if _MSC_VER >= 1910
#pragma warning(push)
/*disable Spectre warnings*/
#pragma warning(disable:5045)
#endif

int find_endof_inbuf(void* pbuf, int bufsize)
{
    int idx = 0;
    unsigned char* pptr = (unsigned char*)pbuf;

    for (idx = 0; idx < bufsize; idx++) {
        if (pptr[idx] == 0x0) {
            return idx;
        }
    }
    return -1;
}

int vsnprintf_safe(char** ppbuf, int *pbufsize, const char* fmt, va_list ap)
{
    char* pRetBuf = NULL;
    size_t retsize = 0;
    int nret;
    int ret;
    va_list origap;

    if (fmt == NULL) {
        if (*ppbuf) {
            free(*ppbuf);
        }
        *ppbuf = NULL;
        *pbufsize = 0;
        return 0;
    }

    if (ppbuf ==NULL || pbufsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pRetBuf = *ppbuf;
    retsize = (size_t)(*pbufsize);

    if (pRetBuf == NULL || retsize < 32) {
        if (retsize < 32) {
            retsize = 32;
        }
        pRetBuf = (char*)malloc(retsize);
        if (pRetBuf == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc(%d) error(%d)", retsize, ret);
            goto fail;
        }
    }
    va_copy(origap, ap);
try_again:
    va_copy(ap,origap);
    ret = vsnprintf(pRetBuf, retsize - 1, fmt, ap);
    if (ret == -1 || ret >= (int)(retsize - 1)) {
        retsize <<= 1;
        if (pRetBuf != *ppbuf) {
            free(pRetBuf);
        }
        pRetBuf = NULL;
        pRetBuf = (char*)malloc(retsize);
        if (pRetBuf == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc(%d) error(%d)", retsize, ret);
            goto fail;
        }
        goto try_again;
    }

    nret = ret + 1;
    if (*ppbuf && *ppbuf != pRetBuf) {
        free(*ppbuf);
    }
    *ppbuf = pRetBuf;
    *pbufsize = (int)retsize;

    return nret;
fail:
    /*if (pRetBuf && pRetBuf != *ppbuf) {
        free(pRetBuf);
    }*/
    /*because fail go here for pRetBuf == NULL ,so not free the buffer*/
    pRetBuf = NULL;
    retsize = 0;
    SETERRNO(-ret);
    return ret;
}

int snprintf_safe(char** ppbuf, int *pbufsize, const char* fmt, ...)
{
    va_list ap;
    if (fmt == NULL) {
        if (*ppbuf) {
            free(*ppbuf);
        }
        *ppbuf = NULL;
        *pbufsize = 0;
        return 0;
    }

    va_start(ap, fmt);
    return vsnprintf_safe(ppbuf, pbufsize, fmt, ap);
}

int append_vsnprintf_safe(char** ppbuf, int *pbufsize, const char* fmt, va_list ap)
{
    char* pRetBuf = *ppbuf;
    char* pTmpBuf = NULL;
    char* pcurptr = NULL;
    size_t tmpsize;
    size_t retsize = (size_t)(*pbufsize);
    int nret, ret;
    size_t leftsize = retsize;
    size_t cntsize = 0;
    va_list origap;

    if (fmt == NULL) {
        if (*ppbuf) {
            free(*ppbuf);
        }
        *ppbuf = NULL;
        *pbufsize = 0;
        return 0;
    }

    if (pRetBuf == NULL || retsize < 32) {
        if (retsize < 32) {
            retsize = 32;
        }
        pRetBuf = (char*)malloc(retsize);
        if (pRetBuf == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc(%d) error(%d)", retsize, ret);
            goto fail;
        }
    }

    if (*ppbuf) {
        cntsize = strlen(*ppbuf);
    }

    if (cntsize > 0  ) {
        if (pRetBuf != *ppbuf) {
            memcpy(pRetBuf, *ppbuf, cntsize);
        }
        pRetBuf[cntsize] = 0x0;
        pcurptr = &(pRetBuf[cntsize]);
        leftsize = retsize - cntsize;
    } else {
        pcurptr = pRetBuf;
        leftsize = retsize;
    }

    va_copy(origap, ap);
try_again:
    va_copy(ap,origap);
    ret = vsnprintf(pcurptr, leftsize - 1, fmt, ap);
    if (ret == -1 || ret >= (int)(leftsize - 1)) {
        tmpsize = retsize << 1;
        pTmpBuf = (char*)malloc(tmpsize);
        if (pTmpBuf == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not alloc(%d) error(%d)", tmpsize, ret);
            goto fail;
        }

        if (cntsize > 0 && *ppbuf != NULL) {
            memcpy(pTmpBuf, *ppbuf, cntsize);
            pTmpBuf[cntsize] = 0x0;
            pcurptr = &(pTmpBuf[cntsize]);
            leftsize = tmpsize - cntsize;
        } else {
            pTmpBuf[0] = 0;
            pcurptr = pTmpBuf;
            leftsize = tmpsize;
        }

        if (pRetBuf && pRetBuf != *ppbuf) {
            free(pRetBuf);
        }
        pRetBuf = NULL;
        pRetBuf = pTmpBuf;
        pTmpBuf = NULL;
        retsize = tmpsize;
        tmpsize = 0;
        goto try_again;
    }

    nret = ret + 1;
    nret += (int)cntsize;

    if (*ppbuf && *ppbuf != pRetBuf) {
        free(*ppbuf);
    }
    *ppbuf = pRetBuf;
    *pbufsize = (int)retsize;
    return nret;
fail:
    if (pRetBuf && pRetBuf != *ppbuf) {
        free(pRetBuf);
    }
    pRetBuf = NULL;
    SETERRNO(-ret);
    return ret;
}

int append_snprintf_safe(char**ppbuf, int*pbufsize, const char* fmt, ...)
{
    va_list ap;
    if (fmt == NULL) {
        if (*ppbuf) {
            free(*ppbuf);
        }
        *ppbuf = NULL;
        *pbufsize = 0;
        return 0;
    }
    va_start(ap, fmt);
    return append_vsnprintf_safe(ppbuf, pbufsize, fmt, ap);
}

void __make_lowercase(const char* pstr)
{
    char* pcurptr = (char*) pstr;

    while (pcurptr && *pcurptr != 0x0) {
        if (*pcurptr >= 'A' && *pcurptr <= 'Z') {
            *pcurptr = *pcurptr - 'A' + 'a';
        }
        pcurptr ++;
    }
    return ;
}

void str_lower_case(const char* pstr)
{
    return __make_lowercase(pstr);
}

char* str_in_str(const char* pstr, const char *search)
{
    return (char*)strstr(pstr, search);
}

int str_match_wildcard(const char* regpat, const char* str)
{
    int bmatched = 0;
    char* pcurstr = NULL;

    char* pcurpat = NULL, *pnextpat = NULL;
    char* pcopypat = NULL;
    char* pcopystr = NULL;
    size_t patlen, slen;
    int ret;
    addr_t curpatlen;
    char* pmatchstr = NULL;

    if (regpat == NULL || str == NULL) {
        return 0;
    }

    /*now we copy the regular pattern*/
    patlen = strlen(regpat) + 1;
    pcopypat = (char*)malloc(patlen);
    if (pcopypat == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can alloc(%d) error(%d)", patlen, ret);
        goto out;
    }

    slen = strlen(str) + 1;
    pcopystr = (char*)malloc(slen);
    if (pcopystr == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can alloc(%d) error(%d)", slen, ret);
        goto out;
    }
    memcpy(pcopypat, regpat, patlen);
    memcpy(pcopystr, str, slen);

    __make_lowercase(pcopypat);
    __make_lowercase(pcopystr);

    pcurstr = pcopystr;
    pcurpat = pcopypat;



    for (; *pcurpat != 0x0;) {
        pnextpat = pcurpat;
        while (*pnextpat != 0x0 && *pnextpat != '*') {
            pnextpat ++;
        }

        curpatlen = (addr_t)pnextpat - (addr_t)pcurpat;
        if (curpatlen > 0) {
            /*it means we have something in the pattern,so we should give it to match*/
            if (*pcurstr == 0x0 || strlen(pcurstr) < curpatlen) {
                /*nothing to match*/
                bmatched = 0;
                goto out;
            }
            /*now search for it*/
            if (*pnextpat == '*') {
                *pnextpat = 0x0;
                pnextpat ++;
            }
            pmatchstr = strstr(pcurstr, pcurpat);
            if (pmatchstr == NULL) {
                /*nothing match*/
                bmatched = 0;
                goto out;
            }
            /*ok ,we match this skip this*/
            pcurstr = pmatchstr + curpatlen;
            pcurpat = pnextpat;

        } else {
            /*nothing to do*/
            if (*pnextpat == 0x0) {
                break;
            } else if (*pnextpat == '*') {
                *pnextpat = 0x0;
                pnextpat ++;
                pcurpat = pnextpat;
                continue;
            }

        }
    }

    bmatched = 1;

out:
    if (pcopypat) {
        free(pcopypat);
    }
    pcopypat = NULL;
    if (pcopystr) {
        free(pcopystr);
    }
    pcopystr = NULL;
    return bmatched;
}


int quote_stringv(char** ppquotestr, int *psize, const char* pstr, va_list ap)
{
    char* pret = NULL;
    int retsize = 0;
    char* ptmp = NULL;
    int tmpsize = 0;
    int ret;
    int cnt;
    char* pcursrc = NULL, *pcurdst = NULL;;

    if (pstr == NULL) {
        if (ppquotestr && *ppquotestr) {
            free(*ppquotestr);
            *ppquotestr = NULL;
        }
        if (psize) {
            *psize = 0;
        }
        return 0;
    }

    if (ppquotestr == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pret = *ppquotestr;
    retsize = *psize;

    ret = vsnprintf_safe(&ptmp, &tmpsize, pstr, ap);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    /*we include the " at begin or end and the every two*/
    if (pret == NULL || retsize < ((ret * 2) + 3))  {
        if (retsize < ((ret * 2) + 3)) {
            retsize = ((ret * 2) + 3);
        }
        pret = (char*) malloc((size_t)retsize);
        if (pret == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc [%d] error[%d]", retsize, ret);
            goto fail;
        }
    }


    memset(pret , 0 , (size_t)retsize);
    pcursrc = ptmp;
    pcurdst = pret;
    *pcurdst = '"';
    pcurdst ++;

    while (*pcursrc) {
        if (*pcursrc == '\\' ||
                *pcursrc == '"') {
            *pcurdst = '\\';
            pcurdst ++;
        }
        *pcurdst = *pcursrc;
        pcurdst ++;
        pcursrc ++;
    }

    *pcurdst = '"';
    pcurdst ++;
    /*at the end of the size*/
    *pcurdst = '\0';

    cnt = (int)(pcurdst - pret);
    assert(cnt < retsize);
    if (pret != *ppquotestr && *ppquotestr) {
        free(*ppquotestr);
    }

    *ppquotestr = pret;
    *psize = retsize;
    return cnt;
fail:
    if (pret != NULL && pret != *ppquotestr) {
        free(pret);
    }
    pret = NULL;
    retsize = 0;
    vsnprintf_safe(&ptmp, &tmpsize, NULL, ap);
    SETERRNO(ret);
    return ret;
}

int quote_string(char** ppquotestr, int *psize, const char* pstr, ...)
{
    va_list ap;
    if (pstr == NULL) {
        if (ppquotestr && *ppquotestr) {
            free(*ppquotestr);
            *ppquotestr = NULL;
        }
        if (psize) {
            *psize = 0;
        }
        return 0;
    }
    va_start(ap, pstr);
    return quote_stringv(ppquotestr, psize, pstr, ap);
}

char* safe_strdup(const char* str)
{
    size_t len = strlen(str);
    char* pretstr = NULL;

    pretstr = (char*) malloc(len + 1);
    if (pretstr == NULL) {
        return NULL;
    }
    memcpy(pretstr, str, len + 1);
    return pretstr;
}

void __make_uppercase(const char* pstr)
{
    char* pcurptr = (char*) pstr;

    while (pcurptr && *pcurptr != 0x0) {
        if (*pcurptr >= 'a' && *pcurptr <= 'z') {
            *pcurptr = *pcurptr - 'a' + 'A';
        }
        pcurptr ++;
    }
    return ;
}

void str_upper_case(const char* pstr)
{
    __make_uppercase(pstr);
    return;
}

void __make_under_score(const char* pstr)
{
    char* pcurptr = (char*) pstr;

    while (pcurptr && *pcurptr != 0x0) {
        if (*pcurptr == '-') {
            *pcurptr = '_';
        }
        pcurptr ++;
    }
    return ;
}

void str_underscore_case(const char* pstr)
{
    __make_under_score(pstr);
    return;
}

int str_nocase_cmp(const char* pstr, const char* pcmpstr)
{
    return _stricmp(pstr, pcmpstr);
}

int str_case_cmp(const char* pstr, const char* pcmpstr)
{
    return strcmp(pstr, pcmpstr);
}


#define MIN_STR_SIZE    0x100

int __get_basenum(unsigned char* ptr, unsigned char* pnum, int base)
{
    int i = 0;
    int ret;
    unsigned char* pcurptr = ptr;
    unsigned short cnum = 0;
    int maxbits = 0;
    if (pcurptr == NULL || pnum == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (base == 8 ) {
        maxbits = 3;
    } else if (base == 10) {
        maxbits = 3;
    } else if (base == 16) {
        maxbits = 2;
    } else {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    cnum = 0;
    pcurptr = ptr;
    for (i = 0; i < maxbits; i++, pcurptr ++) {
        if (base == 8) {
            if (*pcurptr >= '0' && *pcurptr <= '7') {
                cnum *= 8;
                cnum += *pcurptr - '0';
            } else {
                break;
            }
        } else if (base == 10) {
            if (*pcurptr >= '0' && *pcurptr <= '9') {
                cnum *= 10;
                cnum += (*pcurptr - '0');
            } else {
                break;
            }
        } else if (base == 16) {
            if (*pcurptr >= '0' && *pcurptr <= '9') {
                cnum *= 16;
                cnum += (*pcurptr - '0');
            } else if (*pcurptr >= 'a' && *pcurptr <= 'f') {
                cnum *= 16;
                cnum += (*pcurptr - 'a');
            } else if (*pcurptr >= 'A' && *pcurptr <= 'F') {
                cnum *= 16;
                cnum += (*pcurptr - 'A');
            } else {
                break;
            }
        }
    }

    if (cnum > 255) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    *pnum = (unsigned char)cnum;
    return i;
fail:
    SETERRNO(ret);
    return ret;
}

int unquote_string(char** ppstr, int *psize, char* pinput)
{
    unsigned char* pretstr = NULL;
    size_t retsize = 0;
    unsigned char* ptmpbuf = NULL;
    int ret;
    size_t retlen = 0;
    unsigned char* pcurptr = (unsigned char*)pinput;
    int quoted = 0;
    unsigned char addnum = 0;

    if (pinput == NULL) {
        if (ppstr != NULL && *ppstr != NULL) {
            free(*ppstr);
            *ppstr = NULL;
        }
        if (psize != NULL) {
            *psize = 0;
        }
        return 0;
    }

    if (ppstr == NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretstr = (unsigned char*)*ppstr;
    retsize = (size_t) * psize;

    if (retsize < MIN_STR_SIZE || pretstr == NULL) {
        if (retsize < MIN_STR_SIZE) {
            retsize = MIN_STR_SIZE;
        }
        pretstr = (unsigned char*) malloc(retsize);
        if (pretstr == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc %d error[%d]", retsize, ret);
            goto fail;
        }
    }
    memset(pretstr, 0, retsize);
    retlen = 0;
    if (*pcurptr == '"') {
        quoted = 1;
        pcurptr ++;
    }

    while (*pcurptr != '\0') {
        if (retlen >= retsize) {
            retsize <<= 1;
            ptmpbuf = (unsigned char*) malloc(retsize);
            if (ptmpbuf == NULL) {
                GETERRNO(ret);
                ERROR_INFO("alloc %d error[%d]", retsize, ret);
                goto fail;
            }
            memset(ptmpbuf, 0 , retsize);
            if (retlen > 0) {
                memcpy(ptmpbuf, pretstr, retlen);
            }
            if (pretstr != NULL && pretstr != (unsigned char*)*ppstr) {
                free(pretstr);
            }
            pretstr = ptmpbuf;
            ptmpbuf = NULL;
        }

        if (*pcurptr != '\\') {
            if (quoted && *pcurptr == '"' && pcurptr[1] != '\0') {
                ret = -ERROR_INVALID_PARAMETER;
                goto fail;
            } else if (quoted && *pcurptr == '"' && pcurptr[1] == '\0') {
                quoted = 0;
                break;
            }
            pretstr[retlen] = *pcurptr;
            pcurptr ++;
            retlen ++;
        } else if (quoted) {
            pcurptr ++;
            if (pcurptr[0] == 'b') {
                pretstr[retlen] = '\0';
                retlen --;
            } else if (pcurptr[0] == 't') {
                pretstr[retlen] = '\t';
                retlen ++;
            } else if (pcurptr[0] == 'n') {
                pretstr[retlen] = '\n';
                retlen ++;
            } else if (pcurptr[0] == 'r') {
                pretstr[retlen] = '\r';
                retlen ++;
            } else if (pcurptr[0] == 'x') {
                addnum = 0;
                pcurptr ++;
                ret = __get_basenum(pcurptr, &addnum, 16);
                if (ret < 0) {
                    GETERRNO(ret);
                    ERROR_INFO("parse error %s at [%s]", pinput, pcurptr);
                    goto fail;
                }
                pcurptr += ret;
                pretstr[retlen] = addnum;
                retlen ++;
                continue;
            } else if (pcurptr[0] == '0') {
                addnum = 0;
                ret = __get_basenum(pcurptr, &addnum, 8);
                if (ret < 0) {
                    GETERRNO(ret);
                    ERROR_INFO("parse %s error at [%s]", pinput, pcurptr);
                    goto fail;
                }
                pcurptr += ret;
                pretstr[retlen] = addnum;
                retlen ++;
                continue;
            } else if (pcurptr[0] == '"') {
                pretstr[retlen] = *pcurptr;
                retlen ++;
            } else if (pcurptr[0] >= '1' && pcurptr[0] <= '9') {
                addnum = 0;
                ret = __get_basenum(pcurptr, &addnum, 10) ;
                if (ret < 0) {
                    GETERRNO(ret);
                    ERROR_INFO("parse %s error at [%s]", pinput, pcurptr);
                    goto fail;
                }
                pcurptr += ret;
                pretstr[retlen] = addnum;
                retlen ++;
                continue;
            } else if (pcurptr[0] == 'a') {
                pretstr[retlen] = '\a';
                retlen ++;
            } else if (pcurptr[0] == 'v') {
                pretstr[retlen] = '\v';
                retlen ++;
            }
            pcurptr ++;
        } else {
            pretstr[retlen] = *pcurptr;
            pcurptr ++;
            retlen ++;
        }
    }

    if (quoted) {
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("not close quote string [%s]", pinput);
        goto fail;
    }


    if (*ppstr != NULL && (unsigned char*)*ppstr != pretstr) {
        free(*ppstr);
    }
    *ppstr = (char*)pretstr;
    *psize = (int)retsize;

    return (int)retlen;
fail:
    if (pretstr != NULL && pretstr != (unsigned char*)*ppstr) {
        free(pretstr);
    }
    pretstr = NULL;
    SETERRNO(ret);
    return ret;
}

int check_valid_simple_string(const char* value)
{
    int ret = 1;
    char* pcurptr = (char*)value;
    int quoted = 0;
    if (*pcurptr == '"') {
        quoted = 1;
        pcurptr ++;
    }
    for (; *pcurptr != '\0'; pcurptr ++) {
        if (*pcurptr >= '0' && *pcurptr <= '9') {
            continue;
        }
        if ( (*pcurptr >= 'a' && *pcurptr <= 'z') ||
                (*pcurptr >= 'A' && *pcurptr <= 'Z')) {
            continue;
        }
        if (*pcurptr == '_' || *pcurptr == '-' ||
                *pcurptr == ' ' || *pcurptr == '\t') {
            continue;
        }

        if (quoted && *pcurptr == '\\' && pcurptr[1] != '\0') {
            pcurptr ++;
            continue;
        }
        if (quoted && *pcurptr != '"') {
            continue;
        }

        if (quoted && *pcurptr == '"') {
            if (pcurptr[1] != '\0') {
                ret = 0;
                break;
            }
            quoted = 0;
            continue;
        }

        /*ok for the continue*/
        ret = 0;
        break;
    }

    if (quoted) {
        /*this means it is error*/
        return 0;
    }
    return ret;
}

int __inner_free(char*** ppplines, int *psize)
{
    char** pplines;
    int retsize;
    int i;
    if (ppplines != NULL && *ppplines != NULL && psize != NULL) {
        retsize = *psize;
        pplines = *ppplines;
        for (i = 0; i < retsize ; i ++) {
            if (pplines[i] != NULL) {
                free(pplines[i]);
                pplines[i] = NULL;
            }
        }
        free(pplines);
        *ppplines = NULL;
    }
    if (psize != NULL) {
        *psize = 0;
    }
    return 0;
}

int split_lines(const char* str, char*** ppplines, int *psize)
{
    int ret;
    size_t cursize = 0;
    int retsize = 0;
    int retlen = 0;
    char** ppretlines = NULL;
    char** pptmpbuf = NULL;
    char* pcurptr = NULL, *plastptr;
    char* pgetcurptr;
    int mustchk = 0;


    if (str == NULL) {
        return __inner_free(ppplines, psize);
    }


    if (ppplines == NULL || psize == NULL) {
        ret =  -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    /*now first to get lines number*/
    pcurptr = (char*)str;
    plastptr = pcurptr;


    retsize = 4;
    retlen = 0;
    ppretlines = (char**) malloc(sizeof(*ppretlines) * retsize);
    if (ppretlines == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc %d error[%d]", sizeof(*ppretlines)*retsize, ret);
        goto fail;
    }
    memset(ppretlines, 0 , sizeof(*ppretlines)*retsize);

    for (; *pcurptr != '\0'; pcurptr++) {
        if (*pcurptr == '\n') {
            pgetcurptr = pcurptr;
            pgetcurptr --;
            if (*pgetcurptr == '\r') {
                pgetcurptr --;
            }

            if (retlen >= retsize) {
                retsize <<= 1;
                pptmpbuf = (char**) malloc(sizeof(*ppretlines) * retsize);
                if (pptmpbuf == NULL) {
                    GETERRNO(ret);
                    ERROR_INFO("alloc %d error[%d]", sizeof(*ppretlines)*retsize, ret);
                    goto fail;
                }
                memset(pptmpbuf, 0 , sizeof(*pptmpbuf)*retsize);
                if (retlen > 0) {
                    memcpy(pptmpbuf, ppretlines, sizeof(*ppretlines)*retlen);
                }
                if (ppretlines != NULL) {
                    free(ppretlines);
                }
                ppretlines = pptmpbuf;
                pptmpbuf = NULL;
            }

            cursize = (size_t)(pgetcurptr - plastptr) + 1;
            ppretlines[retlen] = (char*) malloc((cursize + 2));
            if (ppretlines[retlen] == NULL) {
                GETERRNO(ret);
                ERROR_INFO("alloc %d error[%d]", cursize + 2, ret);
                goto fail;
            }
            memset(ppretlines[retlen], 0, cursize + 2);
            //DEBUG_INFO("[%d]cursize [%d]", retlen, cursize);
            if (cursize > 0) {
                memcpy(ppretlines[retlen], plastptr, cursize);
            }
            plastptr = (pcurptr + 1);
            pgetcurptr = ppretlines[retlen];
            mustchk = 0;
            while (*pgetcurptr != '\0') {
                if (mustchk) {
                    if (*pgetcurptr != '\r' &&
                            *pgetcurptr != '\0') {
                        WARN_INFO("[%d][%d][%s][0x%02x]",retlen,(int)(pgetcurptr - ppretlines[retlen]),ppretlines[retlen],(unsigned char)*pgetcurptr);
                    }
                } else if (*pgetcurptr == '\r') {
                    *pgetcurptr = '\0';
                    mustchk = 1;
                }
                pgetcurptr ++;
            }

            retlen ++;
        }
    }

    /*it is the last lines*/
    pgetcurptr = pcurptr;
    pgetcurptr --;
    if (*pgetcurptr == '\r') {
        pgetcurptr --;
    }

    if (retlen >= retsize) {
        retsize <<= 1;
        pptmpbuf = (char**) malloc(sizeof(*ppretlines) * retsize);
        if (pptmpbuf == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc %d error[%d]", sizeof(*ppretlines)*retsize, ret);
            goto fail;
        }
        memset(pptmpbuf, 0 , sizeof(*pptmpbuf)*retsize);
        if (retlen > 0) {
            memcpy(pptmpbuf, ppretlines, sizeof(*ppretlines)*retlen);
        }
        if (ppretlines != NULL) {
            free(ppretlines);
        }
        ppretlines = pptmpbuf;
        pptmpbuf = NULL;
    }

    cursize = (size_t)(pgetcurptr - plastptr) + 1;
    ppretlines[retlen] = (char*) malloc(cursize + 2);
    if (ppretlines[retlen] == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc %d error[%d]", cursize + 2, ret);
        goto fail;
    }
    memset(ppretlines[retlen], 0, cursize + 2);
    if (cursize > 0) {
        memcpy(ppretlines[retlen], plastptr, cursize);
    }

    plastptr = (pcurptr + 1);
    pgetcurptr = ppretlines[retlen];
    mustchk = 0;
    while (*pgetcurptr != '\0') {
        if (mustchk) {
            if (*pgetcurptr != '\r' &&
                    *pgetcurptr != '\0') {
                WARN_INFO("[%d][%d][%s][0x%02x]",retlen,(int)(pgetcurptr - ppretlines[retlen]),ppretlines[retlen],(unsigned char)*pgetcurptr);
            }
        } else if (*pgetcurptr == '\r') {
            *pgetcurptr = '\0';
            mustchk = 1;
        }
        pgetcurptr ++;
    }


    retlen ++;

    __inner_free(ppplines, psize);
    *ppplines = ppretlines;
    *psize = retsize;
    return retlen;
fail:
    __inner_free(&ppretlines, &retsize);
    SETERRNO(ret);
    return ret;
}

void str_to_normalize_name(const char* pstr)
{
    char* pcur = (char*)pstr;

    while(*pcur != 0x0) {
        if (*pcur >= '0' && *pcur <= '9') {

        } else if (*pcur >= 'a' && *pcur <= 'z') {

        } else if (*pcur >= 'A' && *pcur <= 'Z' || *pcur == '_') {

        } else {
            *pcur = '_';
        }
        pcur ++;
    }
    return;
}

#if _MSC_VER >= 1910
#pragma warning(pop)
#endif