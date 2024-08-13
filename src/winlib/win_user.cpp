#include <win_user.h>
#include <win_output_debug.h>
#include <win_uniansi.h>
#include <win_strop.h>
#include <win_proc.h>

#pragma warning(push)
#pragma warning(disable:4820)

#include <Lm.h>

#pragma warning(pop)

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#pragma comment(lib, "netapi32.lib")


int user_change_password(char* user, char* oldpassword,char* newpassword)
{
	wchar_t* puuser=NULL, *puoldpass=NULL,*punewpass=NULL,*pudomain=NULL;
	int usersize=0, oldsize=0,newsize=0,domainsize=0;
	int ret;
	NET_API_STATUS  status;

	if (user == NULL || oldpassword == NULL || newpassword == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	ret = AnsiToUnicode("\\\\.",&pudomain,&domainsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = AnsiToUnicode(user,&puuser,&usersize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = AnsiToUnicode(oldpassword,&puoldpass,&oldsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = AnsiToUnicode(newpassword,&punewpass,&newsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	status = NetUserChangePassword(pudomain, puuser,puoldpass,punewpass);
	if (status != NERR_Success) {
		GETERRNO(ret);
		ERROR_INFO("can not change user[%s] from old [%s] =>  new[%s] error[%ld]", user, oldpassword,newpassword, status);
		goto fail;
	}

	AnsiToUnicode(NULL,&pudomain,&domainsize);
	AnsiToUnicode(NULL,&puuser,&usersize);
	AnsiToUnicode(NULL,&puoldpass,&oldsize);
	AnsiToUnicode(NULL,&punewpass,&newsize);
	return 0;

fail:
	AnsiToUnicode(NULL,&pudomain,&domainsize);
	AnsiToUnicode(NULL,&puuser,&usersize);
	AnsiToUnicode(NULL,&puoldpass,&oldsize);
	AnsiToUnicode(NULL,&punewpass,&newsize);
	SETERRNO(ret);
	return ret;
}


int get_user_info(int freed,HANDLE exithd,puser_info_t* ppuser,int* psize)
{
	char* pout=NULL;
	int outsize=0;
	char* perr=NULL;
	int errsize=0;
	int ret;
	char* cmd=NULL;
	int cmdsize=0;
	int cnt = 0;
	puser_info_t pretinfo=NULL;
	int retsize=0;
	int exitcode=0;
	char** pplines =NULL;
	int linesize=0;
	int linelen=0;
	int i;
	int nameoff=0;
	int namesize=0;
	int sidoff=0;
	int sidsize=0;
	char* pcurptr=NULL;
	int curoff=0;
	puser_info_t ptmp =NULL;
	char* pname = NULL;
	char* psid = NULL;

	if (freed) {
		if (ppuser && *ppuser != NULL) {
			free(*ppuser);
			*ppuser = NULL;
		}
		if (psize) {
			*psize = 0;
		}

		return 0;
	}

	if (ppuser == NULL || psize ==NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	pretinfo = *ppuser;
	retsize = *psize;

	ret = snprintf_safe(&cmd,&cmdsize,"wmic.exe useraccount get name,sid");
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = run_cmd_event_output_single(exithd,NULL,0,&pout,&outsize,&perr,&errsize,&exitcode,0,cmd);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO("run [%s] error [%d]",cmd,ret);
		goto fail;
	}

	if (exitcode != 0) {
		ret = exitcode;
		if (ret > 0) {
			ret = -ret;
		}
		ERROR_BUFFER_FMT(pout,outsize,"[%s] output",cmd);
		ERROR_BUFFER_FMT(perr,errsize,"[%s] errout",cmd);
		goto fail;
	}

	ret = split_lines(pout,&pplines,&linesize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	linelen = ret;
	DEBUG_INFO("linelen [%d]",linelen);

	if (linelen < 2) {
		ret = -ERROR_INVALID_PARAMETER;
		ERROR_BUFFER_FMT(pout,outsize,"[%s] out not valid",cmd);
		goto fail;
	}

	for(i=0;i<linelen;i++) {
		if (i== 0) {
			curoff = 0;
			pcurptr = pplines[i];
			DEBUG_INFO("pcurptr %p",pcurptr);
			while (*pcurptr != 0) {
				if (_strnicmp(pcurptr,"name",4) == 0) {
					nameoff = curoff;
					namesize = 4;
					pcurptr += 4;
					curoff += 4;
					while(*pcurptr != 0 ) {
						if(*pcurptr != ' ') {
							break;
						}
						pcurptr += 1;
						namesize += 1;
						curoff += 1;
					}
					DEBUG_INFO("pcurptr %p",pcurptr);
				} else if (_strnicmp(pcurptr,"sid",3) == 0) {
					sidoff = curoff;
					sidsize = 3;
					pcurptr += 3;
					curoff += 3;
					while(*pcurptr != 0 ) {
						if (*pcurptr != ' ') {
							break;
						}
						pcurptr += 1;
						sidsize += 1;
						curoff += 1;
					}
					DEBUG_INFO("pcurptr %p",pcurptr);
				}				
			}

			if (namesize == 0 || sidsize == 0) {
				ret = -ERROR_INVALID_PARAMETER;
				ERROR_INFO("[%s] not valid current",pplines[i]);
				goto fail;
			}

			pname = (char*) malloc((size_t)(namesize + 1));
			psid = (char*) malloc((size_t)(sidsize + 1));
			if (pname == NULL || psid == NULL) {
				GETERRNO(ret);
				goto fail;
			}
		} else {
			if (cnt >= retsize || pretinfo == NULL) {
				if (retsize == 0) {
					retsize = 4;
				} else {
					retsize <<= 1;
				}

				ptmp = (puser_info_t) malloc(sizeof(*ptmp) * retsize);
				if (ptmp == NULL) {
					GETERRNO(ret);
					goto fail;
				}
				memset(ptmp,0,sizeof(*ptmp) * retsize);
				if (cnt > 0) {
					memcpy(ptmp,pretinfo, sizeof(*ptmp)* cnt);
				}

				if (pretinfo != NULL && pretinfo != *ppuser) {
					free(pretinfo);
				}
				pretinfo = ptmp;
				ptmp = NULL;
			}

			memset(pname,0,(size_t)(namesize + 1));
			memset(psid,0,(size_t)sidsize + 1);

			memcpy(pname,&(pplines[i][nameoff]),(size_t)namesize);
			memcpy(psid,&(pplines[i][sidoff]),(size_t)sidsize);

			pcurptr = pname + namesize;
			/*to erase ' ' */
			while (pcurptr != pname) {
				if (*pcurptr != ' ' && *pcurptr != 0x0) {
					break;
				}
				*pcurptr = 0;
				pcurptr -= 1;
			}

			pcurptr = psid + sidsize;
			/*to erase ' ' */
			while (pcurptr != psid) {
				if (*pcurptr != ' ' && *pcurptr != 0x0) {
					break;
				}
				*pcurptr = 0;
				pcurptr -= 1;
			}

			if (strlen(pname) > 0 && strlen(psid) > 0) {
				strncpy_s(pretinfo[cnt].m_name,sizeof(pretinfo[cnt].m_name)-1,pname,sizeof(pretinfo[cnt].m_name));
				strncpy_s(pretinfo[cnt].m_sid,sizeof(pretinfo[cnt].m_sid)-1,psid,sizeof(pretinfo[cnt].m_sid));
				cnt += 1;				
			}
		}
	}

	if (*ppuser && *ppuser != pretinfo) {
		free(*ppuser);
	}

	*ppuser = pretinfo;
	*psize = retsize;


	if (pname) {
		free(pname);
	}
	pname = NULL;
	if (psid) {
		free(psid);
	}
	psid = NULL;

	split_lines(NULL,&pplines,&linesize);
	run_cmd_event_output_single(NULL,NULL,0,&pout,&outsize,&perr,&errsize,&exitcode,0,NULL);
	snprintf_safe(&cmd,&cmdsize,NULL);

	return cnt;
fail:
	if (pretinfo != NULL && pretinfo != *ppuser) {
		free(pretinfo);
	}
	pretinfo = NULL;
	retsize = 0;

	if (pname) {
		free(pname);
	}
	pname = NULL;
	if (psid) {
		free(psid);
	}
	psid = NULL;

	split_lines(NULL,&pplines,&linesize);
	run_cmd_event_output_single(NULL,NULL,0,&pout,&outsize,&perr,&errsize,&exitcode,0,NULL);
	snprintf_safe(&cmd,&cmdsize,NULL);
	SETERRNO(ret);
	return ret;
}