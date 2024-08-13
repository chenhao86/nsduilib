
#include <win_regex.h>
#include <win_err.h>


#pragma warning(push)

#pragma warning(disable:4820)
#pragma warning(disable:4530)
#pragma warning(disable:4514)
#pragma warning(disable:4577)

#if _MSC_VER >= 1910
#pragma warning(disable:4625)
#pragma warning(disable:4626)
#pragma warning(disable:5027)
#pragma warning(disable:5026)
#pragma warning(disable:4774)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <regex>

#pragma warning(pop)

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#define __REGEX_DEBUG__   1

#ifdef __REGEX_DEBUG__
#define  REGEX_MAGIC            0x32daccf9

#define  CHECK_REGEX_MAGIC(preg)    (((preg)->m_magic == REGEX_MAGIC) ? 1 : 0)
#define  SET_REGEX_MAGIC(preg)      do { (preg)->m_magic = REGEX_MAGIC; } while(0)

#else /*__REGEX_DEBUG__*/

#define  CHECK_REGEX_MAGIC(preg)    (1)
#define  SET_REGEX_MAGIC(preg)      do { } while(0)

#endif /*__REGEX_DEBUG__*/

typedef struct __regex_header {
#ifdef __REGEX_DEBUG__
    uint32_t  m_magic;
    uint32_t  m_reserv1;
#endif /*__REGEX_DEBUG__*/
    std::regex* m_pregex;
} regex_header_t, *pregex_header_t;

void __free_regex(pregex_header_t* ppreg)
{
    if (ppreg && *ppreg) {
        pregex_header_t preg = *ppreg;
        if (!CHECK_REGEX_MAGIC(preg)) {
#ifdef __REGEX_DEBUG__
            ERROR_INFO("not valid magic 0x%lx", preg->m_magic);
#endif /*__REGEX_DEBUG__*/
        }
        if (preg->m_pregex) {
            delete preg->m_pregex;
        }
        preg->m_pregex = NULL;
        free(preg);
        *ppreg = NULL;
    }
    return;
}

pregex_header_t __alloc_regex()
{
    pregex_header_t preg = NULL;
    int ret;
    preg = (pregex_header_t) malloc(sizeof(*preg));
    if (preg == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc %d error[%d]", sizeof(*preg), ret);
        goto fail;
    }
    memset(preg, 0, sizeof(*preg));
    SET_REGEX_MAGIC(preg);
    preg->m_pregex = NULL;

    return preg;
fail:
    __free_regex(&preg);
    SETERRNO(ret);
    return NULL;
}

int regex_compile(const char* restr, int flags, void**ppreg)
{
    pregex_header_t preg = NULL;
    int ret;
    std::regex_constants::syntax_option_type type = std::regex_constants::syntax_option_type::ECMAScript;

    if (restr == NULL) {
        if (ppreg != NULL) {
            __free_regex((pregex_header_t*)ppreg);
        }
        return 0;
    }

    if (ppreg == NULL || *ppreg != NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (flags != REGEX_NONE) {
        if (flags & REGEX_IGNORE_CASE) {
            type = std::regex_constants::syntax_option_type::icase;
        }
    }

    preg = __alloc_regex();
    if (preg == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    preg->m_pregex = new std::regex(restr, type);
    if (preg->m_pregex == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    *ppreg = preg;

    return 0;
fail:
    __free_regex(&preg);
    SETERRNO(ret);
    return ret;
}

int regex_exec(void* preg1,const char* instr, int** ppstartpos, int **ppendpos, int * psize)
{
    int *pretstartpos = NULL;
    int *pretendpos=NULL;
    int retsize = 0;
    int ret;
    int retlen=0;
    int i;
    bool bret;
    std::cmatch cm;
    pregex_header_t preg;

    if (preg1 == NULL || instr == NULL) {
        if (ppstartpos && *ppstartpos) {
            free(*ppstartpos);
            *ppstartpos = NULL;
        }
        if (ppendpos && *ppendpos) {
        	free(*ppendpos);
        	*ppendpos = NULL;
        }
        if (psize != NULL) {
            *psize = 0;
        }
        return 0;
    }

    preg = (pregex_header_t) preg1;
    if (ppstartpos == NULL || ppendpos == NULL || psize == NULL || !CHECK_REGEX_MAGIC(preg)) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    pretstartpos = *ppstartpos;
    pretendpos = *ppendpos;
    retsize = *psize;

    bret = std::regex_search(instr,cm,*(preg->m_pregex));
    if (bret) {
    	if (retsize < (int)cm.size() || pretstartpos == NULL || pretendpos == NULL) {
    		if (retsize < (int)cm.size()) {
    			retsize = (int)cm.size();
    		}
    		pretstartpos = (int*) malloc(sizeof(int)* retsize);
    		pretendpos = (int*) malloc(sizeof(int)* retsize);
    		if (pretstartpos == NULL || pretendpos == NULL) {
    			GETERRNO(ret);
    			ERROR_INFO("alloc %d error[%d]", sizeof(int)*retsize,ret);
    			goto fail;
    		}
    	}
    	memset(pretstartpos, 0, sizeof(int)*retsize);
    	memset(pretendpos, 0, sizeof(int)*retsize);

    	for (i = 0 ;i< (int)cm.size();i++) {
    		//DEBUG_INFO("pos[%d] len[%d]", (int)cm.position((uint64_t)i),(int)cm.length((uint64_t)i));
    		pretstartpos[i] = (int)cm.position((uint64_t)i);
    		pretendpos[i] = (int)(cm.position((uint64_t)i) + cm.length((uint64_t)i));
    	}
    	retlen = (int)cm.size();
    }

    if (pretstartpos != *ppstartpos && *ppstartpos != NULL) {
    	free(*ppstartpos);
    }

    if (pretendpos != *ppendpos && *ppendpos != NULL) {
    	free(*ppendpos);
    }

    *ppstartpos = pretstartpos;
    *ppendpos = pretendpos;
    *psize = retsize;
    return retlen;


fail:
	if (pretstartpos && pretstartpos != *ppstartpos) {
		free(pretstartpos);
	}
	pretstartpos = NULL;

	if (pretendpos && pretendpos != *ppendpos) {
		free(pretendpos);
	}
	pretendpos = NULL;
	SETERRNO(ret);
	return ret;
}

#define   EXPAND_POS()                                                                             \
do{                                                                                                \
            if (retlen >= retsize) {                                                               \
                retsize <<= 1;                                                                     \
                if (retsize == 0) {                                                                \
                    retsize = 4;                                                                   \
                }                                                                                  \
                ASSERT_IF(ptmpstart == NULL);                                                      \
                ASSERT_IF(ptmpend == NULL);                                                        \
                ptmpstart = (int*) malloc(sizeof(*ptmpstart) * retsize);                           \
                if (ptmpstart == NULL) {                                                           \
                    GETERRNO(ret);                                                                 \
                    goto fail;                                                                     \
                }                                                                                  \
                memset(ptmpstart, 0 ,sizeof(*ptmpstart) * retsize);                                \
                ptmpend = (int*) malloc(sizeof(*ptmpend) * retsize);                               \
                if (ptmpend == NULL) {                                                             \
                    GETERRNO(ret);                                                                 \
                    goto fail;                                                                     \
                }                                                                                  \
                memset(ptmpend, 0, sizeof(*ptmpend) * retsize);                                    \
                if (retlen > 0) {                                                                  \
                    memcpy(ptmpstart, pretstartpos, sizeof(*ptmpstart) * retlen);                  \
                    memcpy(ptmpend, pretendpos, sizeof(*ptmpend) * retlen);                        \
                }                                                                                  \
                if (pretstartpos != NULL && pretstartpos != *ppstartpos) {                         \
                    free(pretstartpos);                                                            \
                }                                                                                  \
                pretstartpos = NULL;                                                               \
                if (pretendpos != NULL && pretendpos != *ppendpos) {                               \
                    free(pretendpos);                                                              \
                }                                                                                  \
                pretendpos = NULL;                                                                 \
                pretstartpos = ptmpstart;                                                          \
                pretendpos = ptmpend;                                                              \
                ptmpstart = NULL;                                                                  \
                ptmpend = NULL;                                                                    \
            }                                                                                      \
} while(0)

int regex_split(void* preg1, const char* instr, int** ppstartpos, int **ppendpos, int *psize)
{
    int *pretstartpos = NULL;
    int *pretendpos=NULL;
    int retsize = 0;
    int ret;
    int retlen=0;
    bool bret;
    std::cmatch cm;
    pregex_header_t preg;
    int *ptmpstart=NULL,*ptmpend=NULL;
    char* pcurptr;
    int startidx = 0,endidx = 0;
    int curstart=0,curend=0;

    if (preg1 == NULL || instr == NULL) {
        if (ppstartpos && *ppstartpos) {
            free(*ppstartpos);
            *ppstartpos = NULL;
        }
        if (ppendpos && *ppendpos) {
            free(*ppendpos);
            *ppendpos = NULL;
        }
        if (psize != NULL) {
            *psize = 0;
        }
        return 0;
    }

    preg = (pregex_header_t) preg1;
    if (ppstartpos == NULL || ppendpos == NULL || psize == NULL || !CHECK_REGEX_MAGIC(preg)) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    pretstartpos = *ppstartpos;
    pretendpos = *ppendpos;
    retsize = *psize;

    startidx = 0;
    endidx = (int)strlen(instr);

    while (startidx < endidx) {
        pcurptr = (char*)&(instr[startidx]);
        //DEBUG_INFO("[%d]search [%s]",startidx,pcurptr);
        bret = std::regex_search(pcurptr,cm,*(preg->m_pregex));
        if (bret) {
            //DEBUG_INFO("pos [%d] len[%d]", cm.position(0),cm.length(0));
            curstart = (int)cm.position(0);
            curend = curstart + (int)cm.length(0);
            if (curstart == 0) {
                /*this is small*/
                startidx += curend;
                continue;
            }

            if (curend == 0) {
                /*nothing to match*/
                break;
            }
            EXPAND_POS();

            pretstartpos[retlen] = startidx;
            pretendpos[retlen] = (startidx + curstart);
            retlen ++;
            startidx += curend;
            //DEBUG_INFO("startidx [%d]", startidx);
        } else {
            /*that the end of the string, so we should give this*/
            EXPAND_POS();
            pretstartpos[retlen] = startidx;
            pretendpos[retlen] = endidx;
            retlen ++;
            startidx = endidx;
        }
    }


    ASSERT_IF(ptmpstart == NULL);
    ASSERT_IF(ptmpend == NULL);
    if (pretstartpos != *ppstartpos && *ppstartpos != NULL) {
        free(*ppstartpos);
    }

    if (pretendpos != *ppendpos && *ppendpos != NULL) {
        free(*ppendpos);
    }

    *ppstartpos = pretstartpos;
    *ppendpos = pretendpos;
    *psize = retsize;
    return retlen;


fail:
    if (ptmpstart) {
        free(ptmpstart);
    }
    ptmpstart = NULL;

    if (ptmpend) {
        free(ptmpend);
    }
    ptmpend = NULL;

    if (pretstartpos && pretstartpos != *ppstartpos) {
        free(pretstartpos);
    }
    pretstartpos = NULL;

    if (pretendpos && pretendpos != *ppendpos) {
        free(pretendpos);
    }
    pretendpos = NULL;
    SETERRNO(ret);
    return ret;
}