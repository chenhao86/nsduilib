#pragma warning(disable:4514)


#include <win_prn.h>
#include <win_proc.h>
#include <win_strop.h>
#include <win_err.h>
#include <win_fileop.h>
#include <win_strop.h>
#include <win_envop.h>
#include <win_regex.h>

#if _MSC_VER >= 1910
#pragma warning(push)
/*disable Spectre warnings*/
#pragma warning(disable:5045)
#endif

typedef struct __find_path {
	char* m_search;
	char* m_findpath;
	int m_findsize;
	int m_res1;
} find_path_t,*pfind_path_t;

int __find_path(char* basedir,char* curdir,char* curpat,void* arg)
{
	pfind_path_t pfind = (pfind_path_t) arg;
	int ret;
	REFERENCE_ARG(basedir);
	if (_stricmp(pfind->m_search,curpat) == 0) {
		ret = snprintf_safe(&(pfind->m_findpath),&(pfind->m_findsize),"%s\\%s",curdir,curpat);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		return 0;
	}
	return 1;
fail:
	SETERRNO(ret);
	return ret;
}

int __find_vbs(char* partname,char** pppath,int *pathsize)
{
	find_path_t findpath={0};
	int ret;
	char* psysdir=NULL;
	int syssize=0;	
	int retlen=0;
	char* basedir=NULL;
	int basesize=0;
	if (partname == NULL) {
		snprintf_safe(pppath,pathsize,NULL);
		return 0;
	}

	ret = get_env_variable("windir",&psysdir,&syssize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = snprintf_safe(&basedir,&basesize,"%s\\System32\\Printing_Admin_Scripts",psysdir);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	findpath.m_search = _strdup(partname);
	if (findpath.m_search == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	ret = enumerate_directory(basedir,__find_path,&findpath);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	if (findpath.m_findpath == NULL) {
		ret = -ERROR_NOT_FOUND;
		ERROR_INFO("not find [%s] in [%s]", partname,basedir);
		goto fail;
	}

	ret = snprintf_safe(pppath,pathsize,"%s",findpath.m_findpath);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	retlen = ret;

	snprintf_safe(&basedir,&basesize,NULL);
	get_env_variable(NULL,&psysdir,&syssize);

	if (findpath.m_search) {
		free(findpath.m_search);
	}
	findpath.m_search = NULL;
	snprintf_safe(&(findpath.m_findpath),&(findpath.m_findsize),NULL);
	return retlen;
fail:
	snprintf_safe(&basedir,&basesize,NULL);
	get_env_variable(NULL,&psysdir,&syssize);

	if (findpath.m_search) {
		free(findpath.m_search);
	}
	findpath.m_search = NULL;
	snprintf_safe(&(findpath.m_findpath),&(findpath.m_findsize),NULL);
	snprintf_safe(pppath,pathsize,NULL);
	SETERRNO(ret);
	return ret;
}


int get_printer_list(int freed,HANDLE hexitevt,pprinter_list_t* ppret, int *psize)
{
	char* poutlines=NULL;
	int outsize=0;
	char** pplines=NULL;
	int linesize=0;
	int linelen=0;
	int ret;
	int exitcode=0;
	pprinter_list_t pretlist=NULL;
	pprinter_list_t ptmplist=NULL;
	int retsize=0;
	int retlen=0;
	int i;
	int startline=0;
	char* pcurname=NULL;
	char* pcurserver=NULL;
	char* pcurshare=NULL;
	char* pcurlocal=NULL;
	const char* pwmiccmd = "wmic.exe printer list full";

	if (freed) {
		if (ppret && *ppret) {
			free(*ppret);
			*ppret = NULL;
		}
		if (psize) {
			*psize=0;
		}
		return 0;
	}

	if (ppret == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}
	pretlist = *ppret;
	retsize = *psize;

	ret = run_cmd_event_output_single(hexitevt,NULL,0,&poutlines,&outsize,NULL,0,&exitcode,0,(char*)pwmiccmd);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO("can not run [%s] error[%d]", pwmiccmd,ret);
		goto fail;
	}

	if (exitcode != 0) {
		ret = -exitcode;
		if (ret > 0) {
			ret = -ret;
		}
		ERROR_INFO("run [%s] exitcode[%d]", pwmiccmd,exitcode);
		goto fail;
	}

	ret = split_lines(poutlines,&pplines,&linesize);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO("split [%s] error[%d]",poutlines,ret);
		goto fail;
	}

	linelen = ret;
	for (i=0;i<linelen;i++) {
		if (startline) {
			if (strcmp(pplines[i],"") == 0) {
				if (pcurname && pcurserver && pcurlocal && pcurshare) {
					DEBUG_INFO("curname [%s]",pcurname);
					DEBUG_INFO("curserver [%s]",pcurserver);
					DEBUG_INFO("curshare [%s]",pcurshare);
					DEBUG_INFO("curlocal [%s]", pcurlocal);
					if (pretlist == NULL||retlen < retsize) {
						if (retsize == 0) {
							retsize = 4;
						} else {
							retsize <<= 1;
						}
						ptmplist = (pprinter_list_t)malloc(sizeof(*ptmplist) * retsize);
						if (ptmplist == NULL) {
							GETERRNO(ret);
							goto fail;
						}
						memset(ptmplist,0,sizeof(*ptmplist)*retsize);
						if (retlen > 0) {
							memcpy(ptmplist,pretlist,retlen * sizeof(*ptmplist));
						}
						if (pretlist && pretlist != *ppret) {
							free(pretlist);
						}
						pretlist = ptmplist;
						ptmplist = NULL;
					}
					strncpy_s(pretlist[retlen].m_sharename,sizeof(pretlist[retlen].m_sharename),pcurshare,sizeof(pretlist[retlen].m_sharename));
					strncpy_s(pretlist[retlen].m_name, sizeof(pretlist[retlen].m_name), pcurname, sizeof(pretlist[retlen].m_name));
					if (_strnicmp(pcurlocal,"true",4) == 0) {
						strncpy_s(pretlist[retlen].m_type,sizeof(pretlist[retlen].m_type),"local",sizeof(pretlist[retlen].m_type));
					} else {
						strncpy_s(pretlist[retlen].m_type,sizeof(pretlist[retlen].m_type),"network",sizeof(pretlist[retlen].m_type));
					}

					if (strcmp(pretlist[retlen].m_type,"network") == 0) {
						/*skip the first \\ characters*/
						strncpy_s(pretlist[retlen].m_ip, sizeof(pretlist[retlen].m_ip),(pcurserver + 2),sizeof(pretlist[retlen].m_ip));
					}
					retlen ++;
				}
				pcurname = NULL;
				pcurserver = NULL;
				pcurshare = NULL;
				pcurlocal = NULL;
			}
		} else {
			if (strcmp(pplines[i],"") != 0) {
				startline = 1;
			}
		}
		if (_strnicmp(pplines[i],"name=",5) == 0) {
			pcurname = (pplines[i] + 5);
		} else if (_strnicmp(pplines[i],"sharename=",10)==0) {
			pcurshare=(pplines[i] + 10);
		} else if (_strnicmp(pplines[i],"servername=",11)==0) {
			pcurserver = (pplines[i] + 11);
		} else if (_strnicmp(pplines[i],"local=",6) == 0) {
			pcurlocal=(pplines[i] + 6);
		}
	}

	if (*ppret && *ppret != pretlist) {
		free(*ppret);
	}
	*ppret = pretlist;
	pretlist = NULL;

	split_lines(NULL,&pplines,&linesize);
	linelen = 0;
	run_cmd_event_output_single(NULL,NULL,0,&poutlines,&outsize,NULL,0,&exitcode,0,NULL);

	return retlen;
fail:
	if (ptmplist) {
		free(ptmplist);
	}
	ptmplist = NULL;
	if (pretlist && pretlist != *ppret) {
		free(pretlist);
	}
	pretlist = NULL;
	retsize = 0;
	retlen = 0;
	split_lines(NULL,&pplines,&linesize);
	linelen = 0;
	run_cmd_event_output_single(NULL,NULL,0,&poutlines,&outsize,NULL,0,&exitcode,0,NULL);
	SETERRNO(ret);
	return ret;
}


int add_share_printer(HANDLE hexitevt,char* name,char* remoteip,char* user,char* password)
{
	int ret;
	int added=1;
	char* batscript=NULL;
	int batsize=0;
	char* cmpname=NULL;
	int cmpsize=0;
	pprinter_list_t pplist=NULL;
	int prnsize=0,prnlen=0;
	int fidx=0;
	int i;
	int exitcode=0;
	char* vbsfile=NULL;
	int vbssize=0;

	ret = snprintf_safe(&cmpname,&cmpsize,"\\\\%s\\%s",remoteip,name);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}


	ret = get_printer_list(0,hexitevt,&pplist,&prnsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	prnlen = ret;


	fidx= -1;
	for (i=0;i<prnlen;i++) {
		if (_stricmp(cmpname,pplist[i].m_name) ==0) {
			added=0;
			DEBUG_INFO("already added [%s]",cmpname);
			goto succ;
		}
	}

	ret = __find_vbs("prnmngr.vbs",&vbsfile,&vbssize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = snprintf_safe(&batscript,&batsize,"net use \"\\\\%s\\ipc$\" /usr:\"%s\" \"%s\" && cscript.exe %s -ac -p \"\\\\%s\\%s\"", 
			remoteip,user,password, vbsfile,remoteip,name);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = run_cmd_event_output_single(hexitevt,NULL,0,NULL,0,NULL,0,&exitcode,0,batscript);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}


	/*we do not check for the return value or */
	ret = get_printer_list(0,hexitevt,&pplist,&prnsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	prnlen = ret;
	fidx = -1;
	for (i= 0;i<prnlen;i++) {
		DEBUG_INFO("[%d].m_name[%s] cmpname[%s]", i, pplist[i].m_name,cmpname);
		if (_stricmp(pplist[i].m_name,cmpname) == 0) {
			fidx = i;
			DEBUG_INFO("match [%d]",i);
			break;
		}
	}

	if (fidx < 0) {
		ret = -ERROR_CANNOT_MAKE;
		ERROR_INFO("can not make \\\\%s\\%s on user[%s] password[%s]", remoteip,name,user,password);
		goto fail;
	}

succ:
	get_printer_list(1,NULL,&pplist,&prnsize);
	snprintf_safe(&cmpname,&cmpsize,NULL);
	snprintf_safe(&batscript,&batsize,NULL);
	__find_vbs(NULL,&vbsfile,&vbssize);
	return added;
fail:
	get_printer_list(1,NULL,&pplist,&prnsize);
	snprintf_safe(&cmpname,&cmpsize,NULL);
	snprintf_safe(&batscript,&batsize,NULL);
	__find_vbs(NULL,&vbsfile,&vbssize);
	SETERRNO(ret);
	return ret;
}

int del_share_printer(HANDLE hexitevt,char* name,char* remoteip)
{
	int ret;
	int prnsize=0,prnlen=0;
	pprinter_list_t pprn=NULL;
	int fidx=-1;
	int removed=0;
	char* cmpname=NULL;
	int cmpsize=0;
	char* cmd=NULL;
	int cmdsize=0;
	int exitcode=0;
	int i;
	char* vbsfile=NULL;
	int vbssize=0;

	ret = snprintf_safe(&cmpname,&cmpsize,"\\\\%s\\%s",remoteip,name);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	DEBUG_INFO("cmpname [%s]", cmpname);

	ret = get_printer_list(0,hexitevt,&pprn,&prnsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	prnlen = ret;
	fidx = -1;
	for (i=0;i<prnlen;i++) {
		DEBUG_INFO("[%d].m_name [%s] cmpname[%s]",i,pprn[i].m_name,cmpname);
		if (_stricmp(pprn[i].m_name,cmpname) == 0) {
			fidx = i;
			DEBUG_INFO("match [%d]",i);
			break;
		}
	}

	if (fidx < 0) {
		goto succ;
	}

	ret = __find_vbs("prnmngr.vbs",&vbsfile,&vbssize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = snprintf_safe(&cmd,&cmdsize,"cscript.exe %s -d -p \"\\\\%s\\%s\"",vbsfile,remoteip,name);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = run_cmd_event_output_single(hexitevt,NULL,0,NULL,0,NULL,0,&exitcode,0,cmd);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = get_printer_list(0,hexitevt,&pprn,&prnsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	prnlen = ret;

	fidx = -1;
	for(i=0;i<prnlen;i++) {
		DEBUG_INFO("[%d].m_name [%s] cmpname[%s]",i,pprn[i].m_name,cmpname);
		if (_stricmp(pprn[i].m_name,cmpname) == 0) {
			fidx = i;
			DEBUG_INFO("match [%d]",i);
			break;
		}
	}

	if (fidx >= 0) {
		ret = -ERROR_FILE_EXISTS;
		ERROR_INFO("can not remove [%s]", cmpname);
		goto fail;
	}
	removed = 1;
succ:
	snprintf_safe(&cmd,&cmdsize,NULL);
	get_printer_list(1,NULL,&pprn,&prnsize);
	snprintf_safe(&cmpname,&cmpsize,NULL);
	__find_vbs(NULL,&vbsfile,&vbssize);
	return removed;
fail:
	snprintf_safe(&cmd,&cmdsize,NULL);
	get_printer_list(1,NULL,&pprn,&prnsize);
	snprintf_safe(&cmpname,&cmpsize,NULL);
	__find_vbs(NULL,&vbsfile,&vbssize);
	SETERRNO(ret);
	return ret;
}

int __printbrm_call(HANDLE hexitevt,const char* cmd)
{
	int exitcode=0;
	int ret;
	char* output=NULL;
	int outsize=0;
	char** pplines=NULL;
	int linesize=0,linelen=0;
	int i;
	void* chpregex=NULL;
	void* pregex=NULL;
	int* pstartpos=NULL,*pendpos=NULL;
	int possize=0;
	/*chinese ^发生以下错误:*/
	const char* cherroroccur = "^\xb7\xa2\xc9\xfa\xd2\xd4\xcf\xc2\xb4\xed\xce\xf3:";

	ret = run_cmd_event_output_single(hexitevt,NULL,0,&output,&outsize,NULL,0,&exitcode,0,(char*)cmd);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO(" ");
		goto fail;
	}

	if (exitcode != 0) {
		ret = -exitcode;
		if (ret > 0) {
			ret = -ret;
		}
		ERROR_INFO("run [%s] exitcode[%d]",cmd,exitcode);
		goto fail;
	}

	ret = split_lines(output,&pplines,&linesize);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO("output ++++++++++++\n%s\n----------------\nerror[%d]", output,ret);
		goto fail;
	}
	linelen = ret;

	ret = regex_compile("^the following error occurred:",REGEX_IGNORE_CASE,&pregex);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO(" ");
		goto fail;
	}

	ret = regex_compile(cherroroccur,REGEX_IGNORE_CASE,&chpregex);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO(" ");
		goto fail;
	}


	for (i=0;i<linelen;i++) {
		DEBUG_INFO("[%d][%s]",i,pplines[i]);
		ret = regex_exec(pregex,pplines[i],&pstartpos,&pendpos,&possize);
		if (ret < 0) {
			GETERRNO(ret);
			ERROR_INFO(" ");
			goto fail;
		} else if (ret > 0){
			ret = -ERROR_INTERNAL_ERROR;
			ERROR_INFO("[%d] [%s] matches error code", i,pplines[i]);
			goto fail;
		}

		ret = regex_exec(chpregex,pplines[i],&pstartpos,&pendpos,&possize);
		if (ret < 0) {
			GETERRNO(ret);
			ERROR_INFO(" ");
			goto fail;
		} else if (ret > 0) {
			ret = -ERROR_INVALID_PARAMETER;
			ERROR_INFO("[%d] [%s] matches error code chinese", i, pplines[i]);
			goto fail;
		}
	}

	regex_exec(NULL,NULL,&pstartpos,&pendpos,&possize);
	regex_compile(NULL,0,&chpregex);
	regex_compile(NULL,0,&pregex);
	split_lines(NULL,&pplines,&linesize);
	run_cmd_event_output_single(NULL,NULL,0,&output,&outsize,NULL,0,&exitcode,0,NULL);
	return 0;
fail:
	regex_exec(NULL,NULL,&pstartpos,&pendpos,&possize);
	regex_compile(NULL,0,&chpregex);
	regex_compile(NULL,0,&pregex);
	split_lines(NULL,&pplines,&linesize);
	run_cmd_event_output_single(NULL,NULL,0,&output,&outsize,NULL,0,&exitcode,0,NULL);
	SETERRNO(ret);
	return ret;	
}


int save_printer_exportfile(HANDLE hexitevt,char* exportfile)
{
	int cmdsize=0;
	char* cmd=NULL;
	char* sysdir=NULL;
	int syssize=0;
	int ret;

	ret = get_env_variable("windir",&sysdir,&syssize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = snprintf_safe(&cmd,&cmdsize,"%s\\System32\\Spool\\Tools\\PrintBRM.exe -b -f %s",
		sysdir,exportfile);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret= __printbrm_call(hexitevt,cmd);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	snprintf_safe(&cmd,&cmdsize,NULL);
	get_env_variable(NULL,&sysdir,&syssize);
	return 0;
fail:
	snprintf_safe(&cmd,&cmdsize,NULL);
	get_env_variable(NULL,&sysdir,&syssize);
	SETERRNO(ret);
	return ret;
}

int restore_printer_exportfile(HANDLE hexitevt,char* exportfile)
{
	int cmdsize=0;
	char* cmd=NULL;
	char* sysdir=NULL;
	int syssize=0;
	int ret;

	ret = get_env_variable("windir",&sysdir,&syssize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = snprintf_safe(&cmd,&cmdsize,"%s\\System32\\Spool\\Tools\\PrintBRM.exe -r -f %s",
		sysdir,exportfile);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret= __printbrm_call(hexitevt,cmd);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}


	snprintf_safe(&cmd,&cmdsize,NULL);
	get_env_variable(NULL,&sysdir,&syssize);
	return 0;
fail:
	snprintf_safe(&cmd,&cmdsize,NULL);
	get_env_variable(NULL,&sysdir,&syssize);
	SETERRNO(ret);
	return ret;
}


#if _MSC_VER >= 1910
#pragma warning(pop)
#endif 