#ifndef __WIN_STR_OP_H__
#define __WIN_STR_OP_H__

#include <stdarg.h>

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C"{
#endif

WINLIB_API int find_endof_inbuf(void* pbuf,int bufsize);
WINLIB_API int vsnprintf_safe(char** ppbuf,int *pbufsize,const char* fmt,va_list ap);
WINLIB_API int snprintf_safe(char** ppbuf,int *pbufsize,const char* fmt,...);
WINLIB_API int append_vsnprintf_safe(char** ppbuf,int *pbufsize,const char* fmt,va_list ap);
WINLIB_API int append_snprintf_safe(char**ppbuf,int*pbufsize,const char* fmt,...);
WINLIB_API int str_match_wildcard(const char* regpat,const char* str);
WINLIB_API void str_lower_case(const char* pstr);
WINLIB_API void str_to_normalize_name(const char* pstr);
WINLIB_API char* str_in_str(const char* pstr,const char *search);
WINLIB_API int quote_string(char** ppquotestr,int *psize,const char* pstr,...);
WINLIB_API int quote_stringv(char** ppquotestr,int *psize,const char* pstr,va_list ap);
WINLIB_API int unquote_string(char** ppstr, int *psize, char* pinput);
WINLIB_API char* safe_strdup(const char* str);
WINLIB_API void str_upper_case(const char* pstr);
WINLIB_API void str_underscore_case(const char* pstr);
WINLIB_API int str_nocase_cmp(const char* pstr, const char* pcmpstr);
WINLIB_API int str_case_cmp(const char* pstr, const char* pcmpstr);
WINLIB_API int check_valid_simple_string(const char* value);
WINLIB_API int split_lines(const char* str, char*** ppplines, int *psize);

#ifdef __cplusplus
};
#endif


#endif /*__WIN_STR_OP_H__*/