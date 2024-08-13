#ifndef  __WIN_OUTPUT_DEBUG_H__
#define  __WIN_OUTPUT_DEBUG_H__

//#include <stdio.h>
//#include <stdlib.h>
#include <win_err.h>

#define  BASE_LOG_FATAL        0
#define  BASE_LOG_ERROR        10
#define  BASE_LOG_WARN         20
#define  BASE_LOG_INFO         30
#define  BASE_LOG_DEBUG        40
#define  BASE_LOG_TRACE        50

#define  BASE_LOG_DEFAULT      BASE_LOG_ERROR


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#define WIN_CONSOLE_OUTPUT     1
#define WIN_BACKGROUND_OUTPUT  1

#define WINLIB_CONSOLE_DISABLED              1
#define WINLIB_DBWIN_DISABLED                2
#define WINLIB_FILE_DISABLED                 4

#ifdef __cplusplus
extern "C" {
#endif


typedef struct __output_debug_cfg {
	char** m_ppoutcreatefile; /*end with NULL*/
	char** m_ppoutappendfile; /*end with NULL*/
	int m_disableflag; /*disable console*/
	int m_reserv1;
} output_debug_cfg_t, *poutput_debug_cfg_t;


WINLIB_API int  InitOutput(int loglvl);
WINLIB_API int  InitOutputEx(int loglvl,poutput_debug_cfg_t pcfg);
WINLIB_API void FiniOutput(void);
WINLIB_API int error_out(const char* fmt, ...);
WINLIB_API int output_debug_string_handle(int loglvl, const char* file, int lineno, const char* fmt, ...);
WINLIB_API int output_buffer_fmt_handle(int loglvl, const char* file, int lineno, unsigned char* pBuffer, int buflen, const char* fmt, ...);
WINLIB_API int change_log_level(int loglvl);


#ifdef __cplusplus
};
#endif

#define FATAL_INFO(fmt,...)  output_debug_string_handle(BASE_LOG_FATAL,__FILE__,__LINE__,fmt,__VA_ARGS__)
#define ERROR_INFO(fmt,...)  output_debug_string_handle(BASE_LOG_ERROR,__FILE__,__LINE__,fmt,__VA_ARGS__)
#define WARN_INFO(fmt,...)   output_debug_string_handle(BASE_LOG_WARN,__FILE__,__LINE__,fmt,__VA_ARGS__)
#define INFO_INFO(fmt,...)   output_debug_string_handle(BASE_LOG_INFO,__FILE__,__LINE__,fmt,__VA_ARGS__)
#define DEBUG_INFO(fmt,...)  output_debug_string_handle(BASE_LOG_DEBUG,__FILE__,__LINE__,fmt,__VA_ARGS__)
#define TRACE_INFO(fmt,...)  output_debug_string_handle(BASE_LOG_TRACE,__FILE__,__LINE__,fmt,__VA_ARGS__)



#define  FATAL_BUFFER(ptr,blen)               output_buffer_fmt_handle(BASE_LOG_FATAL,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),NULL)
#define  ERROR_BUFFER(ptr,blen)               output_buffer_fmt_handle(BASE_LOG_ERROR,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),NULL)
#define  WARN_BUFFER(ptr,blen)                output_buffer_fmt_handle(BASE_LOG_WARN,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),NULL)
#define  INFO_BUFFER(ptr,blen)                output_buffer_fmt_handle(BASE_LOG_INFO,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),NULL)
#define  DEBUG_BUFFER(ptr,blen)               output_buffer_fmt_handle(BASE_LOG_DEBUG,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),NULL)
#define  TRACE_BUFFER(ptr,blen)               output_buffer_fmt_handle(BASE_LOG_TRACE,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),NULL)


#define  FATAL_BUFFER_FMT(ptr,blen,...)       output_buffer_fmt_handle(BASE_LOG_FATAL,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),__VA_ARGS__)
#define  ERROR_BUFFER_FMT(ptr,blen,...)       output_buffer_fmt_handle(BASE_LOG_ERROR,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),__VA_ARGS__)
#define  WARN_BUFFER_FMT(ptr,blen,...)        output_buffer_fmt_handle(BASE_LOG_WARN,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),__VA_ARGS__)
#define  INFO_BUFFER_FMT(ptr,blen,...)        output_buffer_fmt_handle(BASE_LOG_INFO,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),__VA_ARGS__)
#define  DEBUG_BUFFER_FMT(ptr,blen,...)       output_buffer_fmt_handle(BASE_LOG_DEBUG,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),__VA_ARGS__)
#define  TRACE_BUFFER_FMT(ptr,blen,...)       output_buffer_fmt_handle(BASE_LOG_TRACE,__FILE__,__LINE__,(unsigned char*)(ptr),(int)(blen),__VA_ARGS__)




#define  INIT_LOG(loglvl)  InitOutput(loglvl)
#define  FINI_LOG()        FiniOutput()


#endif /*__WIN_OUTPUT_DEBUG_H__*/
