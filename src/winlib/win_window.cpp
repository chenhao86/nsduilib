
#include <win_window.h>
#include <win_err.h>
#include <win_uniansi.h>

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#pragma comment(lib,"User32.lib")

typedef struct __win_enum_callback {
	win_enum_func_t m_func;
	void* m_param;
} win_enum_callback_t,*pwin_enum_callback_t;

BOOL CALLBACK __inner_get_win_callback(HWND hwnd,LPARAM param)
{
	int ret;
	pwin_enum_callback_t pcallback = (pwin_enum_callback_t) param;
	if (pcallback == NULL || pcallback->m_func == NULL) {
		return FALSE;
	}
	ret = pcallback->m_func(hwnd,pcallback->m_param);
	if (ret < 0) {
		return FALSE;
	}
	return TRUE;
}

int get_win_handle(win_enum_func_t pcallback,void* param)
{
	int ret;
	BOOL bret;
	pwin_enum_callback_t pcallarg=NULL;

	if (pcallback == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(-ret);
		return ret;
	}

	pcallarg = (pwin_enum_callback_t) malloc(sizeof(*pcallarg));
	if (pcallarg == NULL) {
		GETERRNO(ret);
		ERROR_INFO("can not malloc[%d] error[%d]",sizeof(*pcallarg),ret);
		goto out;
	}
	memset(pcallarg,0,sizeof(*pcallarg));
	pcallarg->m_func = pcallback;
	pcallarg->m_param = param;

	bret = EnumWindows(__inner_get_win_callback,(LPARAM)pcallarg);
	if (!bret) {
		GETERRNO(ret);
		ERROR_INFO("enum window error[%d]",ret);
		goto out;
	}

	ret = 0;
out:
	if (pcallarg) {
		free(pcallarg);
	}
	pcallarg = NULL;
	SETERRNO(-ret);
	return ret;
}

typedef struct __win_class {
	char* m_classname;
	HWND *m_phwnd;
	int m_pid;
	int m_hwndsize;
	int m_hwndnum;
	int m_reserv1;
} win_class_t,*pwin_class_t;


int __get_window_type(HWND hwnd,void* param)
{
	pwin_class_t pwinclass = (pwin_class_t) param;
	int ok=0;
	int ret;
	TCHAR* tclassname=NULL;
	int tclasnamesize=0;
	int tclasslen=0;
	char* clsname=NULL;
	int clsnamesize=0;
	HWND* pwndtmp=NULL;
	int wndsize=0;
	int i;
	DWORD thrid,procid;
	if (pwinclass == NULL ) {
		return -1;
	}

	if ((pwinclass->m_classname == NULL || 
		strlen(pwinclass->m_classname) == 0) && 
		(pwinclass->m_pid < 0) ) {
		ok = 1;
	} else {
		while(1) {
			if (tclassname == NULL) {
				if (tclasslen == 0) {
					tclasslen = 16;
				}
				tclasnamesize = (int)(tclasslen * sizeof(TCHAR));
				tclassname = (TCHAR*) malloc((size_t)tclasnamesize);
				if (tclassname == NULL) {
					GETERRNO(ret);
					ERROR_INFO("can not malloc[%d] error[%d]",tclasnamesize,ret);
					goto out;
				}
			}
			SETERRNO(0);
			memset(tclassname,0,(size_t)tclasnamesize);
			ret = GetClassName(hwnd,tclassname,tclasslen);
			if (ret >= (tclasslen-1)) {
				/*it is only the name ,so we do not handle this*/
				tclasslen <<= 1;
				if (tclasslen == 0) {						
					tclasslen = 16;
				} else if (tclasslen > 1024) {
					/*it is so big so we should handle this*/
					DEBUG_INFO("so big for [0x%p]",hwnd);
					break;
				}
				if (tclassname) {
					free(tclassname);
				}
				tclassname = NULL;
				continue;
			}
			break;
		}

		/*now get the class*/
		ret = TcharToAnsi(tclassname,&clsname,&clsnamesize);
		if (ret < 0) {
			GETERRNO(ret);
			goto out;
		}

		procid = 0;
		thrid = GetWindowThreadProcessId(hwnd,&procid);
		if ((pwinclass->m_classname == NULL || strlen(pwinclass->m_classname) == 0 ||  _stricmp(clsname,pwinclass->m_classname) == 0) && 
			(pwinclass->m_pid < 0 || (int)procid == pwinclass->m_pid)) {
			ok = 1;
		}
	}

	if (ok) {
		if (pwinclass->m_hwndnum >= pwinclass->m_hwndsize) {
			wndsize = pwinclass->m_hwndsize;
			wndsize <<= 1;
			if (wndsize == 0) {
				wndsize = 4;
			}

			pwndtmp = (HWND*)malloc(sizeof(HWND)*wndsize);
			if (pwndtmp == NULL) {
				GETERRNO(ret);
				ERROR_INFO("can not malloc[%d] error[%d]",sizeof(HWND)*wndsize,ret);
				goto out;
			}
			memset(pwndtmp,0,sizeof(HWND)*wndsize);
			for (i=0;i<pwinclass->m_hwndnum;i++) {
				pwndtmp[i] = pwinclass->m_phwnd[i];
			}
			if (pwinclass->m_phwnd) {
				free(pwinclass->m_phwnd);
			}
			pwinclass->m_phwnd = pwndtmp;
			pwinclass->m_hwndsize = wndsize;
			pwndtmp = NULL;
		}

		pwinclass->m_phwnd[pwinclass->m_hwndnum] = hwnd;
		pwinclass->m_hwndnum ++;
		ret = 1;
	} else {
		ret = 0;
	}
out:
	TcharToAnsi(NULL,&clsname,&clsnamesize);
	if (tclassname) {
		free(tclassname);
	}
	tclassname = NULL;
	if (ret < 0){
		SETERRNO(-ret);
	} else {
		SETERRNO(0);
	}
	return ret;
}

int get_win_handle_by_classname(const char* typeclass,int pid,HWND *ppwnd[],int *pwinsize)
{
	pwin_class_t pwinclass = NULL;
	int ret = 0;
	int numret = 0;
	HWND* pretwin=NULL;
	int retsize=0;
	int i;

	if (typeclass == NULL) {
		if (ppwnd && *ppwnd) {
			free(*ppwnd);
		}
		if (ppwnd) {
			*ppwnd = NULL;
		}
		if (pwinsize) {
			*pwinsize = 0;
		}
		return 0;
	}

	if (ppwnd == NULL || pwinsize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	pretwin = *ppwnd;
	retsize = *pwinsize;

	pwinclass = (pwin_class_t) malloc(sizeof(*pwinclass));
	if (pwinclass == NULL) {
		GETERRNO(ret);
		ERROR_INFO("can not malloc[%d] error[%d]",sizeof(*pwinclass),ret);
		goto fail;
	}
	memset(pwinclass,0,sizeof(*pwinclass));
	pwinclass->m_classname = (char*)typeclass;
	pwinclass->m_pid = pid;
	pwinclass->m_hwndnum = 0;
	pwinclass->m_hwndsize = 0;
	pwinclass->m_phwnd = NULL;

	ret = get_win_handle(__get_window_type,pwinclass);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	numret = pwinclass->m_hwndnum;
	if (pwinclass->m_hwndnum > 0) {
		if (retsize < pwinclass->m_hwndnum || pretwin == NULL) {
			if (retsize < pwinclass->m_hwndnum) {
				retsize = pwinclass->m_hwndnum;
			}
			pretwin = (HWND*) malloc(sizeof(HWND)*retsize);
			if (pretwin == NULL) {
				GETERRNO(ret);
				ERROR_INFO("can not malloc [%d] error[%d]",sizeof(HWND)*retsize,ret);
				goto fail;
			}			
		}
		memset(pretwin,0,sizeof(HWND)*retsize);
		for (i=0;i<pwinclass->m_hwndnum;i++) {
			pretwin[i] = pwinclass->m_phwnd[i];
		}
	}

	if (pwinclass) {
		if (pwinclass->m_phwnd) {
			free(pwinclass->m_phwnd);
		}
		pwinclass->m_phwnd = NULL;
		pwinclass->m_hwndnum = 0;
		pwinclass->m_hwndsize = 0;
		pwinclass->m_classname = NULL;
		free(pwinclass);
	}
	pwinclass = NULL;

	if (*ppwnd && pretwin != *ppwnd) {
		free(*ppwnd);
	}

	*ppwnd = pretwin;
	*pwinsize = retsize;
	return numret;
fail:
	if (pwinclass) {
		if (pwinclass->m_phwnd) {
			free(pwinclass->m_phwnd);
		}
		pwinclass->m_phwnd = NULL;
		pwinclass->m_hwndnum = 0;
		pwinclass->m_hwndsize = 0;
		pwinclass->m_classname = NULL;
		free(pwinclass);
	}
	pwinclass = NULL;

	if (pretwin && pretwin != *ppwnd) {
		free(pretwin);
	}
	pretwin = NULL;
	SETERRNO(-ret);
	return ret;
}

typedef struct __get_proc_win {
	HANDLE *m_phds;
	int m_size;
	int m_len;
	int m_pid;
	int m_error;
} get_proc_win_t,*pget_proc_win_t;

BOOL CALLBACK enum_win_proc(HWND hwnd,LPARAM ctx)
{
	pget_proc_win_t pwin = (pget_proc_win_t)ctx;
	int ret;
	DWORD getpid;
	HANDLE* ptmp=NULL;
	DWORD dret;
	if (pwin->m_error) {
		return FALSE;
	}

	dret = GetWindowThreadProcessId(hwnd,&getpid);
	if (dret != 0) {
		if ((int)getpid == pwin->m_pid) {
			if (pwin->m_size <= pwin->m_len) {
				pwin->m_size <<= 1;
				if (pwin->m_size == 0) {
					pwin->m_size = 4;
				}

				ptmp = (HANDLE*)malloc(sizeof(HANDLE) * pwin->m_size);
				if (ptmp == NULL) {
					GETERRNO(ret);
					goto fail;
				}
				memset(ptmp,0, pwin->m_size * sizeof(HANDLE));
				if (pwin->m_len > 0) {
					memcpy(ptmp, pwin->m_phds,pwin->m_len * sizeof(HANDLE));
				}
				if (pwin->m_phds) {
					free(pwin->m_phds);
				}
				pwin->m_phds = ptmp;
				ptmp = NULL;
			}
			pwin->m_phds[pwin->m_len] = hwnd;
			pwin->m_len ++;
		}
	}

	return TRUE;
fail:
	pwin->m_error = ret;
	SETERRNO(ret);
	return FALSE;
}

void __free_pwin(pget_proc_win_t* ppwin)
{
	if (ppwin && *ppwin) {
		pget_proc_win_t pwin = *ppwin;
		if (pwin->m_phds) {
			free(pwin->m_phds);
		}
		pwin->m_phds = NULL;
		pwin->m_pid = 0;
		pwin->m_len = 0;
		pwin->m_size = 0;
		free(pwin);
		*ppwin = NULL;
	}
	return;
}

pget_proc_win_t __alloc_pwin(int pid)
{
	pget_proc_win_t pwin = NULL;
	int ret;

	pwin = (pget_proc_win_t)malloc(sizeof(*pwin));
	if (pwin == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	memset(pwin,0,sizeof(*pwin));
	pwin->m_pid = pid;

	return pwin;
fail:
	__free_pwin(&pwin);
	SETERRNO(ret);
	return NULL;
}


int get_window_from_pid(int pid, HWND** pphdl,int *psize)
{
	HWND* prethdl = NULL;
	int retsize=0;
	int ret;
	int retlen = 0;
	BOOL bret;
	pget_proc_win_t pwin=NULL;
	if (pid == 0) {
		if (pphdl && *pphdl) {
			free(*pphdl);
			*pphdl = NULL;
		}
		if (psize) {
			*psize = 0;
		}
	}

	if (pphdl == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}
	prethdl = *pphdl;
	retsize = *psize;

	pwin = __alloc_pwin(pid);
	if (pwin == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	bret = EnumWindows(enum_win_proc,(LPARAM)pwin);
	if (!bret) {
		GETERRNO(ret);
		ERROR_INFO("can not enum error[%d]", ret);
		goto fail;
	}

	if (pwin->m_error != 0) {
		ret = pwin->m_error;
		ERROR_INFO("enum error[%d]", ret);
		goto fail;
	}

	if (pwin->m_len > retsize) {
		retsize = pwin->m_len;
		prethdl = (HWND*)malloc(sizeof(HWND) * retsize);
		if (prethdl == NULL) {
			GETERRNO(ret);
			goto fail;
		}
	}

	retlen = pwin->m_len;
	if (retlen > 0) {
		memset(prethdl,0,sizeof(HWND)*retsize);
		memcpy(prethdl,pwin->m_phds,sizeof(HWND)*retlen);
	}

	if (*pphdl && *pphdl != prethdl) {
		free(*pphdl);
	}
	*pphdl = prethdl;
	*psize = retsize;
	__free_pwin(&pwin);
	return retlen;
fail:
	if (prethdl && prethdl != *pphdl) {
		free(prethdl);
	}
	prethdl = NULL;
	retsize = 0;
	__free_pwin(&pwin);
	SETERRNO(ret);
	return ret;
}


int get_window_text(HWND hd, char** pptext,int *psize)
{
	TCHAR* ptname=NULL;
	int tnamesize=0;
	int retlen=0;
	int ret;

	if (hd == NULL) {
		return TcharToAnsi(NULL,pptext,psize);
	}

	tnamesize = 256;

//try_again:
	ptname = (TCHAR*) malloc(sizeof(TCHAR) * tnamesize);
	if (ptname == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	ret = GetWindowText(hd,ptname,tnamesize);
	if (ret == 0) {
		GETERRNO(ret);
		ERROR_INFO("get [%p] text error[%d]", hd ,ret);
		goto fail;
	}

	ret = TcharToAnsi(ptname,pptext,psize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	retlen = ret;

	if (ptname) {
		free(ptname);
	}
	ptname = NULL;
	return retlen;
fail:
	if (ptname) {
		free(ptname);
	}
	ptname = NULL;
	SETERRNO(ret);
	return ret;
}