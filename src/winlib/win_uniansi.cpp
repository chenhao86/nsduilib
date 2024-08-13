
#include <win_uniansi.h>
#include <win_output_debug.h>
#include <win_err.h>

#pragma warning(push)
#pragma warning(disable:4514)

#include <assert.h>

#pragma warning(pop)

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

int UnicodeToAnsi(const wchar_t* pWideChar, char** ppChar, int*pCharSize)
{
    char* pRetChar = *ppChar;
    int retcharsize = *pCharSize;
    int ret, wlen, needlen;

    if (pWideChar == NULL) {
        if (*ppChar) {
            delete [] pRetChar;
        }
        *ppChar = NULL;
        *pCharSize = 0;
        return 0;
    }
    wlen = (int)wcslen(pWideChar);
    needlen = WideCharToMultiByte(CP_ACP, 0, pWideChar, wlen, NULL, 0, NULL, NULL);
    if (retcharsize <= needlen) {
        retcharsize = (needlen + 1);
        pRetChar = new char[(size_t)needlen + 1];
        assert(pRetChar);
    }

    ret = WideCharToMultiByte(CP_ACP, 0, pWideChar, wlen, pRetChar, retcharsize, NULL, NULL);
    if (ret != needlen) {
        ret = ERROR_INVALID_BLOCK;
        goto fail;
    }
    pRetChar[needlen] = '\0';

    if ((*ppChar) && (*ppChar) != pRetChar) {
        char* pTmpChar = *ppChar;
        delete [] pTmpChar;
    }
    *ppChar = pRetChar;
    *pCharSize = retcharsize;

    return needlen;
fail:
    if (pRetChar && pRetChar != (*ppChar)) {
        delete [] pRetChar;
    }
    pRetChar = NULL;
    retcharsize = 0;
    SetLastError((DWORD)ret);
    return -ret;
}


int AnsiToUnicode(char* pChar, wchar_t **ppWideChar, int*pWideCharSize)
{
    wchar_t *pRetWideChar = *ppWideChar;
    int retwidecharsize = *pWideCharSize;
    int ret, len, needlen;

    if (pChar == NULL) {
        if (*ppWideChar) {
            delete [] pRetWideChar;
        }
        *ppWideChar = NULL;
        *pWideCharSize = 0;
        return 0;
    }

    len = (int) strlen(pChar);
    needlen = MultiByteToWideChar(CP_ACP, 0, pChar, len, NULL, 0);
    if (retwidecharsize <= needlen) {
        retwidecharsize = needlen + 1;
        pRetWideChar = new wchar_t[(size_t)retwidecharsize];
        assert(pRetWideChar);
    }

    ret = MultiByteToWideChar(CP_ACP, 0, pChar, len, pRetWideChar, retwidecharsize);
    if (ret != needlen) {
        ret = ERROR_INVALID_BLOCK;
        goto fail;
    }
    pRetWideChar[needlen] = '\0';

    if ( (*ppWideChar) && (*ppWideChar) != pRetWideChar) {
        wchar_t *pTmpWideChar = *ppWideChar;
        delete [] pTmpWideChar;
    }
    *ppWideChar = pRetWideChar;
    *pWideCharSize = retwidecharsize;
    return ret;
fail:
    if (pRetWideChar && pRetWideChar != (*ppWideChar)) {
        delete [] pRetWideChar;
    }
    pRetWideChar = NULL;
    retwidecharsize = 0;
    SetLastError((DWORD)ret);
    return -ret;
}

#ifndef _UNICODE
int _chartoansi(const char *ptchar, char** ppChar, int*pCharSize)
{
    size_t needlen = 0;
    size_t needsize = (size_t)(*pCharSize);
    int ret;
    char* pRetChar = *ppChar;

    if (ptchar == NULL) {
        /*if null, we just free memory*/
        if (pRetChar) {
            free(pRetChar);
        }
        *ppChar = NULL;
        *pCharSize = 0;
        return 0;
    }

    needlen = strlen(ptchar);
    if (pRetChar == NULL || *pCharSize <= (int)needlen) {
        pRetChar = (char*) malloc(needlen + 1);
        if (pRetChar == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        needsize = needlen + 1;
    }
    memset(pRetChar, 0, needsize);
    memcpy(pRetChar, ptchar, needlen);
    if (pRetChar != *ppChar && *ppChar != NULL) {
        free(*ppChar);
    }

    *ppChar = pRetChar;
    *pCharSize = (int)needsize;

    return  (int)needlen;
fail:
    if (pRetChar != *ppChar && pRetChar != NULL) {
        free(pRetChar);
    }
    pRetChar = NULL;
    return ret;
}

#endif


int TcharToAnsi(const TCHAR *ptchar, char** ppChar, int*pCharSize)
{
    int ret;
#ifdef _UNICODE
    ret = UnicodeToAnsi(ptchar, ppChar, pCharSize);
#else
    ret = _chartoansi(ptchar, ppChar, pCharSize);
#endif
    return ret;
}

int AnsiToTchar(const char *pChar, TCHAR **pptchar, int *ptcharsize)
{
    int ret;
#ifdef _UNICODE
    ret = AnsiToUnicode((char*)pChar, pptchar, ptcharsize);
#else
    ret = _chartoansi(pChar, pptchar, ptcharsize);
#endif
    return ret;
}

int Utf8ToAnsi(const char *pUtf8, char** ppchars, int*pcharsize)
{
    int ret;
    int retlen = 0;
    char* pretchars = NULL;
    int retsize = 0;
    int needlen = 0;
    int len = 0;
    wchar_t* punicode = NULL;
    int unisize = 0, unilen;
    if (pUtf8 == NULL) {
        if (ppchars && *ppchars) {
            free(*ppchars);
            *ppchars = NULL;
        }
        if (pcharsize) {
            *pcharsize = 0;
        }
        return 0;
    }
    if (ppchars == NULL || pcharsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretchars = *ppchars;
    retsize = *pcharsize;


    len = (int)strlen(pUtf8);
    unilen = MultiByteToWideChar(CP_UTF8, 0, pUtf8, len, NULL, 0);
    if ((unisize <= (int)(unilen * sizeof(wchar_t)) ) || punicode == NULL) {
        if (unisize <= (int)(unilen * sizeof(wchar_t))) {
            unisize = (int)((unilen + 1) * sizeof(wchar_t));
        }
        punicode = (wchar_t*)malloc((size_t)unisize);
        if (punicode == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }
    memset(punicode , 0, (size_t)unisize);

    unilen = MultiByteToWideChar(CP_UTF8, 0, pUtf8, len, punicode, (int)(unisize / sizeof(wchar_t)));
    if (unilen < 0 || unilen >= (int)(unisize / sizeof(wchar_t))) {
        GETERRNO(ret);
        goto fail;
    }

    /*now change*/
    needlen = WideCharToMultiByte(CP_ACP, 0, punicode, unilen, NULL, 0, NULL, NULL);
    if (needlen >= retsize || pretchars == NULL) {
        if (retsize <= needlen) {
            retsize = needlen + 1;
        }
        pretchars = (char*)malloc((size_t)retsize);
        if (pretchars == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }
    memset(pretchars, 0, (size_t)retsize);

    retlen = WideCharToMultiByte(CP_ACP, 0, punicode, unilen, pretchars, retsize, NULL, NULL);
    if (retlen != needlen) {
        GETERRNO(ret);
        goto fail;
    }

    if (punicode) {
        free(punicode);
    }
    punicode = NULL;
    unisize = 0;
    unilen = 0;

    if (*ppchars && *ppchars != pretchars) {
        free(*ppchars);
    }
    *ppchars = pretchars;
    *pcharsize = retsize;

    SETERRNO(0);
    return retlen;
fail:
    if (punicode) {
        free(punicode);
    }
    punicode = NULL;
    unisize = 0;
    unilen = 0;

    if (pretchars && pretchars != *ppchars) {
        free(pretchars);
    }
    pretchars = NULL;
    SETERRNO(ret);
    return ret;
}

int AnsiToUtf8(const char* pchars, char** ppUtf8, int *pUtf8size)
{
    int ret;
    int retlen = 0;
    int needlen = 0;
    char* pretutf8 = NULL;
    int retsize = 0;
    wchar_t* punicode = NULL;
    int unisize = 0, unilen;
    if (pchars == NULL) {
        if (ppUtf8 && *ppUtf8) {
            free(*ppUtf8);
            *ppUtf8 = NULL;
        }
        if (pUtf8size) {
            *pUtf8size = 0;
        }
        return 0;
    }
    if (ppUtf8 == NULL || pUtf8size == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretutf8 = *ppUtf8;
    retsize = *pUtf8size;


    unilen = MultiByteToWideChar(CP_ACP, 0, pchars, -1, NULL, 0);
    if (unisize <= (int)(unilen*sizeof(wchar_t)) || punicode == NULL) {
        if (unisize <= (int)(unilen* sizeof(wchar_t))) {
            unisize = (int)((unilen + 1) * sizeof(wchar_t));
        }
        punicode = (wchar_t*)malloc((size_t)unisize);
        if (punicode == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }
    memset(punicode , 0, (size_t)unisize);

    unilen = MultiByteToWideChar(CP_ACP, 0, pchars, -1, punicode, (int)(unisize / sizeof(wchar_t)));
    if (unilen < 0 || unilen >= (int)(unisize/ sizeof(wchar_t))) {
        GETERRNO(ret);
        goto fail;
    }

    /*now change*/
    needlen = WideCharToMultiByte(CP_UTF8, 0, punicode, -1, NULL, 0, NULL, NULL);
    if (needlen >= retsize || pretutf8 == NULL) {
        if (retsize <= needlen) {
            retsize = needlen + 1;
        }
        pretutf8 = (char*)malloc((size_t)retsize);
        if (pretutf8 == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }
    memset(pretutf8, 0, (size_t)retsize);

    retlen = WideCharToMultiByte(CP_UTF8, 0, punicode, -1, pretutf8, retsize, NULL, NULL);
    if (retlen != needlen) {
        GETERRNO(ret);
        goto fail;
    }

    retlen = (int)strlen(pretutf8);

    if (punicode) {
        free(punicode);
    }
    punicode = NULL;
    unisize = 0;
    unilen = 0;

    if (*ppUtf8 && *ppUtf8 != pretutf8) {
        free(*ppUtf8);
    }
    *ppUtf8 = pretutf8;
    *pUtf8size = retsize;

    SETERRNO(0);
    return retlen;
fail:
    if (punicode) {
        free(punicode);
    }
    punicode = NULL;
    unisize = 0;
    unilen = 0;

    if (pretutf8 && pretutf8 != *ppUtf8) {
        free(pretutf8);
    }
    pretutf8 = NULL;
    SETERRNO(ret);
    return ret;
}


int Utf8ToUnicode(const char* putf8, wchar_t** ppUni,int *punisize)
{
    int ret;
    wchar_t* pretuni=NULL;
    int retsize=0;
    int retlen=0;
    int utflen=0;

    if (putf8 == NULL) {
        if (ppUni && *ppUni) {
            free(*ppUni);
            *ppUni = NULL;
        }
        if (punisize) {
            *punisize = 0;
        }
        return 0;
    }

    if (ppUni == NULL || punisize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretuni = *ppUni;
    retsize = *punisize;

    utflen = (int)strlen(putf8);
    if (retsize < (int)((utflen +1)* sizeof(wchar_t))) {
        retsize = (int)((utflen +1)* sizeof(wchar_t));
        pretuni = (wchar_t*) malloc((size_t)retsize);
        if (pretuni == NULL) {
            GETERRNO(ret);
            goto fail;
        }        
    }
    memset(pretuni, 0 ,(size_t)retsize);

    SETERRNO(0);
    ret = MultiByteToWideChar(CP_UTF8,0,putf8,(int)strlen(putf8),pretuni, (int)((retsize / sizeof(wchar_t)) -1));
    if (ret == 0) {
        GETERRNO_DIRECT(ret);
        if (ret != 0) {
            goto fail;
        }
    }
    retlen = (int)(ret * sizeof(wchar_t));
    if (*ppUni && *ppUni != pretuni) {
        free(*ppUni);
    }
    *ppUni = pretuni;
    *punisize = retsize;
    return retlen;
fail:
    if (pretuni && pretuni != *ppUni) {
        free(pretuni);
    }
    pretuni = NULL;
    SETERRNO(ret);
    return ret;
}

int UnicodeToUtf8(const wchar_t* pUni, char** pputf8, int *putf8size)
{
    int ret;
    char* pretutf8=NULL;
    int retsize=0;
    int retlen=0;
    int wlen = 0;
    int res;

    if (pUni == NULL) {
        if (pputf8 && *pputf8) {
            free(*pputf8);
            *pputf8 = NULL;
        }
        if (putf8size) {
            *putf8size = 0;
        }
        return 0;
    }

    if (pputf8 == NULL || putf8size == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    pretutf8 = *pputf8;
    retsize = *putf8size;

    wlen = (int)wcslen(pUni);
    if (retsize < ((wlen  + 1)* 4 )) {
        retsize = (wlen + 1)  *4;
        pretutf8 = (char*)malloc((size_t)retsize);
        if (pretutf8 == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }

    SETERRNO(0);
    ret = WideCharToMultiByte(CP_UTF8,0,pUni,-1,pretutf8,retsize,NULL,NULL);
    if (ret  == 0) {
        GETERRNO_DIRECT(res);
        if (res != 0) {
            ret = -res;
            if (ret > 0) {
                ret = -ret;
            }
            goto fail;
        }
    }
    retlen = ret;

    if (*pputf8 && *pputf8 != pretutf8) {
        free(*pputf8);
    }
    *pputf8 = pretutf8;
    *putf8size = retsize;
    return retlen;
fail:
    if (pretutf8 && pretutf8 != *pputf8) {
        free(pretutf8);
    }
    pretutf8 = NULL;
    SETERRNO(ret);
    return ret;
}

int Utf8ToTchar(const char* putf8, TCHAR** pptchar,int *ptcharsize)
{
    char* ansi=NULL;
    int ansisize=0;
    int ret;
    if (putf8 == NULL) {
        return AnsiToTchar(NULL,pptchar,ptcharsize);
    }

    ret = Utf8ToAnsi(putf8,&ansi,&ansisize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToTchar(ansi,pptchar,ptcharsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    Utf8ToAnsi(NULL,&ansi,&ansisize);

    return ret;
fail:    
    Utf8ToAnsi(NULL,&ansi,&ansisize);
    SETERRNO(ret);
    return ret;
}

int TcharToUtf8(const TCHAR* ptchar, char** ppUtf8,int *pUtf8size)
{
    char* ansi=NULL;
    int ansisize=0;
    int ret;
    if (ptchar == NULL) {
        return AnsiToUtf8(NULL,ppUtf8,pUtf8size);
    }

    ret = TcharToAnsi(ptchar,&ansi,&ansisize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToUtf8(ansi,ppUtf8,pUtf8size);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    TcharToAnsi(NULL,&ansi,&ansisize);
    return ret;
fail:
    TcharToAnsi(NULL,&ansi,&ansisize);
    SETERRNO(ret);
    return ret;
}