
#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4820)
#pragma warning(disable:4514)

#include <win_time.h>
#include <win_proc.h>
#include <win_output_debug.h>
#include <win_strop.h>
#include <Windows.h>
#include <time.h>

#pragma warning(default:4820)
#pragma warning(default:4668)

#if defined(_MSC_VER)
#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif
#endif


#define  MAX_TICK_COUNT  (0xffffffffffffffffULL)

uint64_t get_current_ticks(void)
{
	ULONGLONG ticks;

	ticks = GetTickCount64();
	return ticks;
}

int need_wait_times(uint64_t sticks, uint64_t eticks, int timeout)
{
	uint64_t leftticks;
	if (eticks > sticks && eticks >= (sticks + timeout)) {
		return -1;
	}

	if ((eticks < sticks)) {
		leftticks = (MAX_TICK_COUNT - sticks);
		leftticks += eticks;
		if ((int)leftticks >= timeout) {
			return -1;
		}

		return (timeout - (int)leftticks);
	}

	return (int)(timeout - (eticks - sticks));
}

int sleep_mill(int mills)
{
	Sleep((DWORD)mills);
	return 0;
}

#define  EPOCH_SUBSTRACT             116444736000000000LL

uint64_t get_ms_from_epoch(void)
{
	uint64_t lret;
	FILETIME  ft;
	ULARGE_INTEGER li;
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	lret = li.QuadPart;

	lret -= EPOCH_SUBSTRACT;
	lret /= 10000;

	return lret;
}


int get_last_bootuptime(HANDLE hevt,uint64_t *pboottime)
{
    char* cmd=NULL;
    int cmdsize=0;
    char** pplines= NULL;
    int linesize=0,linelen=0;
    int ret;
    char* pout=NULL;
    int outsize=0;
    int exitcode=0;
    char* ptr=NULL;
    int plen=0;
    struct tm tmz;
    char tmstr[5];
    

    ret = snprintf_safe(&cmd,&cmdsize,"wmic.exe os get lastbootuptime");
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = run_cmd_event_output_single(hevt,NULL,0,&pout,&outsize,NULL,0,&exitcode,0,cmd);
    if (ret < 0) {
        GETERRNO(ret);
        ERROR_INFO("run [%s] error[%d]", cmd,ret);
        goto fail;
    }

    if (exitcode !=  0) {
        ret = exitcode;
        if (ret > 0) {
            ret = -ret;            
        }
        ERROR_INFO("run [%s] exitcode[%d]", cmd,exitcode);
        goto fail;
    }

    DEBUG_BUFFER_FMT(pout,outsize,"[%s] output",cmd);

    ret = split_lines(pout,&pplines,&linesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    linelen = ret;

    if (linelen < 2) {
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("[%s] linelen [%d] not valid",cmd,linelen);
        goto fail;
    }

    /*now to get the last time*/
    ptr = pplines[1];
    plen = 0;
    while(*ptr >= '0' && *ptr <= '9') {
        ptr ++;
        plen ++;
    }

    if (plen != 14) {
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("[%s] not valid ", pplines[1]);
        goto fail;
    }

    memset(&tmz,0,sizeof(tmz));
    ptr = pplines[1];
    /*for year*/
    memset(tmstr,0,sizeof(tmstr));
    memcpy(tmstr,ptr,4);
    tmz.tm_year = atoi(tmstr);
    tmz.tm_year -= 1900;
	
	/*for month*/
	ptr += 4;    
    memset(tmstr,0,sizeof(tmstr));
    memcpy(tmstr,ptr,2);
    tmz.tm_mon = atoi(tmstr);

	/*for month day*/
	ptr += 2;
    memset(tmstr,0,sizeof(tmstr));
    memcpy(tmstr,ptr,2);
    tmz.tm_mday = atoi(tmstr);

	/*for hour*/
	ptr += 2;
    memset(tmstr,0,sizeof(tmstr));
    memcpy(tmstr,ptr,2);
    tmz.tm_hour = atoi(tmstr);

	/*for minute*/
	ptr += 2;
    memset(tmstr,0,sizeof(tmstr));
    memcpy(tmstr,ptr,2);
    tmz.tm_min = atoi(tmstr);

	/*for second*/
	ptr += 2;
    memset(tmstr,0,sizeof(tmstr));
    memcpy(tmstr,ptr,2);
    tmz.tm_sec = atoi(tmstr);

    if (pboottime) {

    	*pboottime = (uint64_t)mktime(&tmz);
    	if (*pboottime == -1) {
    		GETERRNO(ret);
    		goto fail;
    	}

	    DEBUG_INFO("%d/%d/%d %d:%d:%d boottime [%ld]",tmz.tm_year,tmz.tm_mon,tmz.tm_mday,tmz.tm_hour,tmz.tm_min,tmz.tm_sec,*pboottime);
    }

    split_lines(NULL,&pplines,&linesize);
    run_cmd_event_output_single(hevt,NULL,0,&pout,&outsize,NULL,0,&exitcode,0,NULL);
    snprintf_safe(&cmd,&cmdsize,NULL);
    return 0;

fail:
    split_lines(NULL,&pplines,&linesize);
    run_cmd_event_output_single(hevt,NULL,0,&pout,&outsize,NULL,0,&exitcode,0,NULL);
    snprintf_safe(&cmd,&cmdsize,NULL);
    SETERRNO(ret);
    return ret;
}

int tm_to_str(struct tm* ptm, char** ppstr, int *psize)
{
    if (ptm == NULL) {
        return snprintf_safe(ppstr,psize,NULL);
    }

    return snprintf_safe(ppstr,psize,"%04d-%02d-%02d %02d:%02d:%02d",ptm->tm_year + 1900 ,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
}

#define SKIP_NUM(ptr)                                                                             \
do{                                                                                               \
    while(1) {                                                                                    \
        if ((*ptr)< '0' || (*ptr) > '9') {                                                        \
            break;                                                                                \
        }                                                                                         \
        ptr ++;                                                                                   \
    }                                                                                             \
}while(0)

#define MATCH_CHAR(ptr,ch)                                                                        \
do{                                                                                               \
    if ((*ptr) != ch) {                                                                           \
        ret = -ERROR_INVALID_PARAMETER;                                                           \
        goto fail;                                                                                \
    }                                                                                             \
    ptr ++;                                                                                       \
}while(0)

int tm_from_str(char* str, struct tm* ptm)
{
    int ret;
    char* pcurptr=NULL;
    struct tm *psettm=NULL;
    if (str == NULL || ptm == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    psettm = (struct tm*)malloc(sizeof(*psettm));
    if (psettm == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    pcurptr = str;

    memset(psettm, 0, sizeof(*psettm));
    psettm->tm_year = atoi(pcurptr);
    psettm->tm_year -= 1900;
    SKIP_NUM(pcurptr);
    MATCH_CHAR(pcurptr,'-');
    psettm->tm_mon = atoi(pcurptr);
    psettm->tm_mon -= 1;
    if (psettm->tm_mon < 0) {
        ret =-ERROR_INVALID_PARAMETER;
        goto fail;
    }
    SKIP_NUM(pcurptr);
    MATCH_CHAR(pcurptr,'-');
    psettm->tm_mday = atoi(pcurptr);
    SKIP_NUM(pcurptr);

    while(1) {
        if (*pcurptr != ' ') {
            break;
        }
        pcurptr ++;
    }

    psettm->tm_hour = atoi(pcurptr);
    SKIP_NUM(pcurptr);
    MATCH_CHAR(pcurptr,':');

    psettm->tm_min = atoi(pcurptr);
    SKIP_NUM(pcurptr);
    MATCH_CHAR(pcurptr,':');

    psettm->tm_sec = atoi(pcurptr);

    memcpy(ptm,psettm,sizeof(*psettm));
    if (psettm) {
        free(psettm);
    }
    psettm = NULL;
    return 0;
fail:
    if (psettm) {
        free(psettm);
    }
    psettm = NULL;

    SETERRNO(ret);
    return ret;
}

int tm_to_time(struct tm* ptm, time_t* ptime)
{
    int ret;
    if (ptm == NULL || ptime == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }
    *ptime = mktime(ptm);
    return 0;
}

int time_to_tm(time_t* ptime,struct tm *ptm)
{
    int ret;
    errno_t err;
    if (ptm == NULL || ptime == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    err = localtime_s(ptm,ptime);
    if (err != 0) {
        GETERRNO(ret);
        SETERRNO(ret);
        return ret;
    }
    return 0;
}

#pragma warning(default:4514)
#pragma warning(pop)