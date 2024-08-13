#pragma warning(push)

#pragma warning(disable:4514)
#pragma warning(disable:4820)
#pragma warning(disable:4668)

#include <win_output_debug.h>
#include <win_output_debug_cfg.h>
#include <win_uniansi.h>
#include <stdio.h>
#include <time.h>

#pragma warning(pop)

#if _MSC_VER >= 1910
#pragma warning(push)
/*disable Spectre warnings*/
#pragma warning(disable:5045)
#endif


#if 1
#define _OUTPUT_DEBUG_ERROR(...)  do{fprintf(stderr,"[%s:%d]:",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\n");fflush(stderr);}while(0)
#else
#define _OUTPUT_DEBUG_ERROR(...)  do{}while(0)
#endif


#include "win_output_debug_cfg.cpp"
#include "win_output_debug_file.cpp"


static int st_output_loglvl = BASE_LOG_DEFAULT;
static CRITICAL_SECTION st_outputcs;
static int st_output_inited = 0;
static std::vector<DebugOutIO*>* st_debugout_ios = NULL;


typedef int (*output_func_t)(char* pbuf);

void __free_output_hds(void)
{
    if (st_debugout_ios != NULL) {
        while (st_debugout_ios->size() > 0) {
            DebugOutIO* pout = st_debugout_ios->at(0);
            st_debugout_ios->erase(st_debugout_ios->begin());
            delete pout;
            pout = NULL;
        }
        delete st_debugout_ios;
        st_debugout_ios = NULL;
    }
    return;
}

const char* _get_loglevel_note(int loglvl)
{
    if (loglvl < BASE_LOG_ERROR) {
        return "<FATAL>";
    } else if (loglvl >= BASE_LOG_ERROR && loglvl < BASE_LOG_WARN) {
        return "<ERROR>";
    } else if (loglvl >= BASE_LOG_WARN && loglvl < BASE_LOG_INFO) {
        return "<WARN>";
    } else if (loglvl >= BASE_LOG_INFO && loglvl < BASE_LOG_DEBUG) {
        return "<INFO>";
    } else if (loglvl >= BASE_LOG_DEBUG && loglvl < BASE_LOG_TRACE) {
        return "<DEBUG>";
    }
    return "<TRACE>";
}





#define  MINI_FMT_SIZE           32

int __inner_time_format(int freed, char** ppfmt, size_t *pfmtsize)
{
    char* pretfmt = NULL;
    size_t retsize = 0;
    int retlen = 0;
    int ret;
    size_t sret;
    struct tm nowtime;
    __time64_t nowt;
    if (freed) {
        if (ppfmt && *ppfmt) {
            free(*ppfmt);
            *ppfmt = NULL;
        }
        if (pfmtsize) {
            *pfmtsize = 0;
        }
        return 0;
    }
    if (ppfmt == NULL || pfmtsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    retsize = *pfmtsize;
    pretfmt = *ppfmt;

    if (pretfmt == NULL || retsize < MINI_FMT_SIZE) {
        if (retsize < MINI_FMT_SIZE) {
            retsize = MINI_FMT_SIZE;
        }
        pretfmt = (char*)malloc(retsize);
        if (pretfmt == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }

    memset(pretfmt , 0, retsize);

    _time64(&nowt);
    ret = _localtime64_s(&nowtime, &nowt);
    if (ret != 0) {
        GETERRNO(ret);
        goto fail;
    }
try_again:
    sret = strftime(pretfmt, retsize - 1, "%Y-%m-%d %H:%M:%S", &nowtime);
    if (sret == 0) {
        retsize <<= 1;
        if (pretfmt && pretfmt != *ppfmt) {
            free(pretfmt);
        }
        pretfmt = NULL;
        pretfmt = (char*)malloc(retsize);
        if (pretfmt == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        goto try_again;
    }
    retlen = (int)sret;

    if (*ppfmt && *ppfmt != pretfmt) {
        free(*ppfmt);
    }
    *ppfmt = pretfmt;
    *pfmtsize = retsize;
    return retlen;
fail:
    /*no pretfmt here*/
    if (pretfmt != NULL && pretfmt != *ppfmt) {
        free(pretfmt);
    }
    pretfmt = NULL;
    SETERRNO(ret);
    return ret;
}

int __inner_output_console(int loglvl, const char* file, int lineno, const char* fmt, va_list ap)
{
    int ret;
    int retsize = 0;
    char* fmttime = NULL;
    size_t timesize = 0;
    char* msg = NULL;
    int msgsize = 0;
    char* locstr = NULL;
    int locsize = 0;
    char* timestr = NULL;
    int tmsize = 0;
    uint32_t i;
    DebugOutIO* pout = NULL;


    ret = __inner_time_format(0, &fmttime, &timesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = str_append_vsnprintf_safe(&msg, &msgsize, fmt, ap);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = str_append_snprintf_safe(&locstr, &locsize, "[%s:%d]", file, lineno);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = str_append_snprintf_safe(&timestr, &tmsize, "time(0x%08x):%s", (unsigned int)GetTickCount(), fmttime);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    EnterCriticalSection(&st_outputcs);
    for (i = 0; st_debugout_ios != NULL && i < st_debugout_ios->size(); i++) {
        pout = st_debugout_ios->at(i);
        ret = pout->write_log(loglvl, locstr, timestr, _get_loglevel_note(loglvl), msg);
        if (ret < 0) {
            GETERRNO(ret);
            LeaveCriticalSection(&st_outputcs);
            goto fail;
        }

        if (retsize < ret) {
            retsize = ret;
        }
    }


    LeaveCriticalSection(&st_outputcs);


    str_append_vsnprintf_safe(&msg, &msgsize, NULL, NULL);
    str_append_snprintf_safe(&locstr, &locsize, NULL);
    str_append_snprintf_safe(&timestr, &tmsize, NULL);
    __inner_time_format(1, &fmttime, &timesize);
    return retsize;
fail:
    str_append_vsnprintf_safe(&msg, &msgsize, NULL, NULL);
    str_append_snprintf_safe(&locstr, &locsize, NULL);
    str_append_snprintf_safe(&timestr, &tmsize, NULL);
    __inner_time_format(1, &fmttime, &timesize);
    SETERRNO(ret);
    return ret;

}

int output_debug_string_handle(int loglvl, const char* file, int lineno, const char* fmt, ...)
{
    va_list ap;
    if (st_output_inited == 0) {
        return 0;
    }
    va_start(ap, fmt);
    return __inner_output_console(loglvl, file, lineno, fmt, ap);
}


int __inner_out_buffer(int loglvl, const char* file, int lineno, unsigned char* pBuffer, int buflen, const char* fmt, va_list ap)
{
    int ret;
    int retsize = 0;
    char* fmttime = NULL;
    size_t timesize = 0;
    char* msg = NULL;
    int msgsize = 0;
    char* locstr = NULL;
    int locsize = 0;
    char* timestr = NULL;
    int tmsize = 0;
    uint32_t i;
    DebugOutIO* pout = NULL;


    ret = __inner_time_format(0, &fmttime, &timesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = str_append_snprintf_safe(&msg, &msgsize, "buffer[0x%p] size[%d:0x%x]", pBuffer, buflen, buflen);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    if (fmt != NULL) {
        ret = str_append_vsnprintf_safe(&msg, &msgsize, " ", ap);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
        ret = str_append_vsnprintf_safe(&msg, &msgsize, fmt, ap);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
    }

    ret = str_append_snprintf_safe(&locstr, &locsize, "[%s:%d]", file, lineno);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = str_append_snprintf_safe(&timestr, &tmsize, "time(0x%08x):%s", (unsigned int)GetTickCount(), fmttime);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    EnterCriticalSection(&st_outputcs);
    if (st_debugout_ios != NULL) {
        for (i = 0; i < st_debugout_ios->size(); i++) {
            pout = st_debugout_ios->at(i);
            ret = pout->write_buffer_log(loglvl, locstr, timestr, _get_loglevel_note(loglvl), msg, pBuffer, buflen);
            if (ret < 0) {
                GETERRNO(ret);
                LeaveCriticalSection(&st_outputcs);
                goto fail;
            }

            if (retsize < ret) {
                retsize = ret;
            }
        }
    }


    LeaveCriticalSection(&st_outputcs);


    str_append_vsnprintf_safe(&msg, &msgsize, NULL, NULL);
    str_append_snprintf_safe(&locstr, &locsize, NULL);
    str_append_snprintf_safe(&timestr, &tmsize, NULL);
    __inner_time_format(1, &fmttime, &timesize);
    return retsize;
fail:
    str_append_vsnprintf_safe(&msg, &msgsize, NULL, NULL);
    str_append_snprintf_safe(&locstr, &locsize, NULL);
    str_append_snprintf_safe(&timestr, &tmsize, NULL);
    __inner_time_format(1, &fmttime, &timesize);
    SETERRNO(ret);
    return ret;
}


int output_buffer_fmt_handle(int loglvl, const char* file, int lineno, unsigned char* pBuffer, int buflen, const char* fmt, ...)
{
    va_list ap = NULL;
    if (st_output_inited == 0) {
        return 0;
    }
    if (fmt != NULL) {
        va_start(ap, fmt);
    }
    return __inner_out_buffer(loglvl, file, lineno, pBuffer, buflen, fmt, ap);
}


int error_out(const char* fmt, ...)
{
    va_list ap;
    int ret = 0;
    va_start(ap, fmt);
    ret += vfprintf(stderr, fmt, ap);
    ret += fprintf(stderr, "\n");
    return ret;
}


void __fini_output_cfg(void)
{
    if (st_output_inited > 0) {
        EnterCriticalSection(&st_outputcs);
        __free_output_hds();
        LeaveCriticalSection(&st_outputcs);
    }
    DeleteCriticalSection(&st_outputcs);
    st_output_inited = 0;
}

int change_log_level(int loglvl)
{
    int retval;
    int ret;
    unsigned int idx=0;

    if (loglvl < BASE_LOG_FATAL || loglvl > BASE_LOG_TRACE) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (st_output_inited == 0) {
        return st_output_loglvl;
    }

    EnterCriticalSection(&st_outputcs);
    retval = st_output_loglvl;
    idx = 0;
    if (st_debugout_ios != NULL) {
        while (st_debugout_ios->size() > idx) {
            DebugOutIO* pout = st_debugout_ios->at(idx);
            pout->set_level(loglvl);
            idx += 1;
        }        
    }
    LeaveCriticalSection(&st_outputcs);
    return retval;
}


int __init_output_cfg(poutput_debug_cfg_t pcfg)
{
    int i;
    int ret;
    DebugOutIO* pout = NULL;
    OutfileCfg* poutcfg = NULL;
    if (st_output_inited == 0) {
        InitializeCriticalSection(&st_outputcs);
        st_output_inited = 1;    
    }
    
    EnterCriticalSection(&st_outputcs);
    __free_output_hds();

    ASSERT_IF(st_debugout_ios == NULL);
    st_debugout_ios = new std::vector<DebugOutIO*>();

    if (pcfg != NULL) {
        if (pcfg->m_ppoutcreatefile) {
            for (i = 0; pcfg->m_ppoutcreatefile[i] != NULL; i++) {
                if (poutcfg == NULL) {
                    poutcfg = new OutfileCfg();
                }
                ret = poutcfg->set_file_type(pcfg->m_ppoutcreatefile[i], WINLIB_DEBUGOUT_FILE_TRUNC, 0, 0);
                if (ret < 0) {
                    GETERRNO(ret);
                    goto fail;
                }

                ret = poutcfg->set_level(st_output_loglvl);
                if (ret < 0) {
                    GETERRNO(ret);
                    goto fail;
                }

                ret = poutcfg->set_format(WINLIB_OUTPUT_ALL_MASK);
                if (ret < 0) {
                    GETERRNO(ret);
                    goto fail;
                }

                pout = get_cfg_out(poutcfg);
                if (pout == NULL) {
                    GETERRNO(ret);
                    goto fail;
                }
                st_debugout_ios->push_back(pout);
                pout = NULL;
            }
        }

        if (pcfg->m_ppoutappendfile) {
            for (i = 0; pcfg->m_ppoutappendfile[i] != NULL ; i++) {
                if (poutcfg == NULL) {
                    poutcfg = new OutfileCfg();
                }
                ret = poutcfg->set_file_type(pcfg->m_ppoutappendfile[i], WINLIB_DEBUGOUT_FILE_APPEND, 0, 0);
                if (ret < 0) {
                    GETERRNO(ret);
                    goto fail;
                }

                ret = poutcfg->set_level(st_output_loglvl);
                if (ret < 0) {
                    GETERRNO(ret);
                    goto fail;
                }

                ret = poutcfg->set_format(WINLIB_OUTPUT_ALL_MASK);
                if (ret < 0) {
                    GETERRNO(ret);
                    goto fail;
                }

                pout = get_cfg_out(poutcfg);
                if (pout == NULL) {
                    GETERRNO(ret);
                    goto fail;
                }
                st_debugout_ios->push_back(pout);
                pout = NULL;
            }
        }
    }

    if (pcfg == NULL || (pcfg->m_disableflag & WINLIB_DBWIN_DISABLED) == 0) {
        if (poutcfg == NULL) {
            poutcfg = new OutfileCfg();
        }

        ret = poutcfg->set_file_type(NULL, WINLIB_DEBUGOUT_FILE_BACKGROUND, 0, 0);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }

        ret = poutcfg->set_level(st_output_loglvl);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }

        ret = poutcfg->set_format(WINLIB_OUTPUT_ALL_MASK);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }

        pout = get_cfg_out(poutcfg);
        if (pout == NULL) {
            GETERRNO(ret);
            goto fail;
        }

        st_debugout_ios->push_back(pout);
        pout = NULL;
    }

    if (pcfg == NULL || (pcfg->m_disableflag & WINLIB_CONSOLE_DISABLED) == 0) {
        if (poutcfg == NULL) {
            poutcfg = new OutfileCfg();
        }

        ret = poutcfg->set_file_type(NULL, WINLIB_DEBUGOUT_FILE_STDERR, 0, 0);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }

        ret = poutcfg->set_level(st_output_loglvl);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }

        ret = poutcfg->set_format(WINLIB_OUTPUT_ALL_MASK);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }

        pout = get_cfg_out(poutcfg);
        if (pout == NULL) {
            GETERRNO(ret);
            goto fail;
        }

        st_debugout_ios->push_back(pout);
        pout = NULL;
    }

    if (poutcfg) {
        delete poutcfg;
    }
    poutcfg = NULL;

    LeaveCriticalSection(&st_outputcs);
    return 0;
fail:
    if (poutcfg) {
        delete poutcfg;
    }
    poutcfg = NULL;
    if (pout) {
        delete pout;
    }
    pout = NULL;
    __free_output_hds();
    LeaveCriticalSection(&st_outputcs);
    SETERRNO(ret);
    return ret;
}


int InitOutput(int loglvl)
{
    int ret;
    st_output_loglvl = loglvl;
    ret = __init_output_cfg(NULL);
    if (ret < 0) {
        GETERRNO(ret);
        __fini_output_cfg();
        SETERRNO(ret);
        return ret;
    }
    return 0;
}

void FiniOutput(void)
{
    __fini_output_cfg();
    return;
}



int InitOutputEx(int loglvl, poutput_debug_cfg_t pcfg)
{
    int ret;
    st_output_loglvl = loglvl;
    ret = __init_output_cfg(pcfg);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    return 0;
fail:
    __free_output_hds();
    SETERRNO(ret);
    return ret;
}

int InitOutputEx2(OutputCfg* pcfgs)
{
    int ret;
    OutfileCfg* pcfg = NULL;
    int i;
    DebugOutIO* pout = NULL;
    if (st_output_inited == 0) {
        InitializeCriticalSection(&st_outputcs);
        st_output_inited = 1;    
    }
    
    EnterCriticalSection(&st_outputcs);
    __free_output_hds();
    ASSERT_IF(st_debugout_ios == NULL);
    st_debugout_ios =  new std::vector<DebugOutIO*>();
    for (i = 0;; i++) {
        pcfg = pcfgs->get_config(i);
        if (pcfg == NULL) {
            break;
        }
        pout = get_cfg_out(pcfg);
        if (pout == NULL) {
            GETERRNO(ret);
            goto fail;
        }
        st_debugout_ios->push_back(pout);
    }

    LeaveCriticalSection(&st_outputcs);
    return 0;
fail:
    __free_output_hds();
    LeaveCriticalSection(&st_outputcs);
    SETERRNO(ret);
    return ret;
}

#if _MSC_VER >= 1910
#pragma warning(pop)
#endif