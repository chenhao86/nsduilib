#ifndef __WIN_UNI_ANSI_H__
#define __WIN_UNI_ANSI_H__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C"  {
#endif

/**********************************************************
*    if pWideChar==NULL  will reset *ppChar=NULL and free memory
*    else set *ppChar by new and *pCharSize is the size of *ppChar
*    return value success >= 0 number of bytes in *ppChar
*    otherwise negative error code
**********************************************************/
WINLIB_API int UnicodeToAnsi(const wchar_t* pWideChar,char** ppChar,int*pCharSize);

/**********************************************************
*    if pChar==NULL  will reset *ppWideChar=NULL and free memory
*    else set *ppWideChar by new and *pWideCharSize is the size of *ppWideChar
*    return value success >= 0 number of bytes in *ppWideChar
*    otherwise negative error code
**********************************************************/
WINLIB_API int AnsiToUnicode(char* pChar,wchar_t **pWideChar,int*pWideCharSize);

/**********************************************************
*    if ptchar==NULL  will reset *ppChar=NULL and free memory
*    else set *ppChar by new and *pCharSize is the size of *ppChar
*    return value success >= 0 number of bytes in *ppChar
*    otherwise negative error code
**********************************************************/
WINLIB_API int TcharToAnsi(const TCHAR *ptchar, char** ppChar, int*pCharSize);

/**********************************************************
*    if pChar==NULL  will reset *pptchar=NULL and free memory
*    else set *pptchar by new and *pptchar is the size of *ptcharsize
*    return value success >= 0 number of bytes in *pptchar
*    otherwise negative error code
**********************************************************/
WINLIB_API int AnsiToTchar(const char *pChar,TCHAR **pptchar,int *ptcharsize);

/**********************************************************
*    if pUtf8==NULL will reset *ppchars=NULL and free memory
*    else set *ppchars by new and *pcharsize is the size of *ppchars
*    return value success >= 0 number of bytes in *ppchars not including the end of
*    '\0'
*    otherwise negative error code
**********************************************************/
WINLIB_API int Utf8ToAnsi(const char *pUtf8,char** ppchars,int*pcharsize);

/**********************************************************
*    if pchars==NULL will reset *ppUtf8=NULL and free memory
*    else set *ppUtf8 by new and *pUtf8size is the size of *ppUtf8
*    return value success >= 0 number of bytes in *ppUtf8 not including the end of
*    '\0'
*    otherwise negative error code
**********************************************************/
WINLIB_API int AnsiToUtf8(const char* pchars, char** ppUtf8,int *pUtf8size);


WINLIB_API int Utf8ToUnicode(const char* putf8, wchar_t** ppUni,int *punisize);
WINLIB_API int UnicodeToUtf8(const wchar_t* pUni, char** pputf8, int *putf8size);

WINLIB_API int Utf8ToTchar(const char* putf8, TCHAR** pptchar,int *ptcharsize);
WINLIB_API int TcharToUtf8(const TCHAR* ptchar, char** ppUtf8,int *pUtf8size);


#ifdef __cplusplus
};
#endif


#endif /*__WIN_UNI_ANSI_H__*/
