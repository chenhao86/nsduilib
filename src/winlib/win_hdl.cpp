#include <win_hdl.h>
#include <win_output_debug.h>
#include <win_ntapi.h>
#include <win_uniansi.h>
#include <win_strop.h>
#include <win_time.h>

#pragma warning(push)
#pragma warning(disable:4530)
#pragma warning(disable:4514)
#pragma warning(disable:4577)

#include <vector>
#pragma warning(pop)


#pragma warning(push)
#if defined(_MSC_VER)
#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif
#endif


#define MAX_QUERY_BUF_SIZE   (2UL << 20)

typedef struct __info_variables {
	int m_typesize;
	int m_namesize;
	char* m_ptypename;
	char* m_pname;
	void* m_ptypeinfo;
	void* m_pnameinfo;
	HANDLE m_duphdl;
	HANDLE m_prochdl;
	PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX m_table;
	phandle_info_t m_phdlinfo;
	HMODULE m_ntmod;
	int m_lastpid;
	ULONG m_tinfosize;
	ULONG m_ninfosize;
	ULONG m_reserv1;
} info_variables_t, *pinfo_variables_t;

typedef struct __input_handls_t {
	CRITICAL_SECTION m_cs;
	std::vector<PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX>* m_pvec;
} input_handles_t, *pinput_handles_t;

typedef struct __output_infos_t {
	CRITICAL_SECTION m_cs;
	std::vector<phandle_info_t>* m_pvec;
} output_infos_t, *poutput_infos_t;

typedef struct __thread_vars {
	HANDLE m_notievt;
	HANDLE m_exitevt;
	HANDLE m_thrhdl;
	pinfo_variables_t m_pvars;
	pinput_handles_t m_pinput;
	poutput_infos_t m_poutput;
	int m_exited;
	int m_exitcode;
} thread_vars_t, *pthread_vars_t;

void free_info_variables(pinfo_variables_t* ppvars)
{
	if (ppvars && *ppvars) {
		pinfo_variables_t pvars = *ppvars;
		if (pvars->m_ptypename) {
			free(pvars->m_ptypename);
		}
		pvars->m_ptypename = NULL;
		pvars->m_typesize = 0;

		if (pvars->m_pname) {
			free(pvars->m_pname);
		}
		pvars->m_pname = NULL;
		pvars->m_namesize = 0;

		if (pvars->m_duphdl != NULL) {
			DEBUG_INFO("close duphdl [%p]", pvars->m_duphdl);
			CloseHandle(pvars->m_duphdl);
		}
		pvars->m_duphdl = NULL;

		if (pvars->m_prochdl != NULL) {
			DEBUG_INFO("close prochdl [%p]", pvars->m_prochdl);
			CloseHandle(pvars->m_prochdl);
		}
		pvars->m_prochdl = NULL;

		if (pvars->m_table != NULL) {
			free(pvars->m_table);
		}
		pvars->m_table = NULL;

		if (pvars->m_phdlinfo != NULL) {
			free(pvars->m_phdlinfo);
		}
		pvars->m_phdlinfo = NULL;

		/*not set module*/
		pvars->m_ntmod = NULL;

		pvars->m_lastpid = -1;

		if (pvars->m_ptypeinfo) {
			free(pvars->m_ptypeinfo);
		}
		pvars->m_ptypeinfo = NULL;
		pvars->m_tinfosize = 0;

		if (pvars->m_pnameinfo) {
			free(pvars->m_pnameinfo);
		}
		pvars->m_pnameinfo = NULL;
		pvars->m_ninfosize = 0;

		free(pvars);
		*ppvars = NULL;
	}
	return ;
}


pinfo_variables_t alloc_info_variables(void)
{
	pinfo_variables_t pvars = NULL;
	int ret;
	pvars = (pinfo_variables_t)malloc(sizeof(*pvars));
	if (pvars == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	memset(pvars, 0, sizeof(*pvars));
	pvars->m_lastpid = -1;

	pvars->m_tinfosize = 4;
	pvars->m_ninfosize = 4;

	return pvars;
fail:
	free_info_variables(&pvars);
	SETERRNO(ret);
	return NULL;
}

void free_input_handles(pinput_handles_t* pphdls)
{
	if (pphdls && *pphdls) {
		pinput_handles_t phdls = *pphdls;
		if (phdls->m_pvec) {
			while (phdls->m_pvec->size() > 0) {
				PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX phdl = phdls->m_pvec->at(0);
				phdls->m_pvec->erase(phdls->m_pvec->begin());
				free(phdl);
				phdl = NULL;
			}

			delete phdls->m_pvec;
			phdls->m_pvec = NULL;
		}
		free(phdls);
		*pphdls = NULL;
	}
	return;
}


int peek_input_handles(pinput_handles_t phdls, PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pinput)
{
	int ret = 0;
	EnterCriticalSection(&(phdls->m_cs));
	if (phdls->m_pvec->size() > 0) {
		PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pget = phdls->m_pvec->at(0);
		memcpy(pinput, pget, sizeof(*pget));
		ret = 1;
	}
	LeaveCriticalSection(&(phdls->m_cs));
	return ret;
}

int remove_input_handles(pinput_handles_t phdls, int errmode)
{
	int ret = 0;
	PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pget = NULL;
	EnterCriticalSection(&(phdls->m_cs));
	if (phdls->m_pvec->size() > 0) {
		pget = phdls->m_pvec->at(0);
		phdls->m_pvec->erase(phdls->m_pvec->begin());
		ret = (int)phdls->m_pvec->size();
	}
	LeaveCriticalSection(&(phdls->m_cs));
	if (pget != NULL) {
		if (errmode != 0) {
			DEBUG_INFO("remove [%d].[0x%x] handle ", (int)pget->UniqueProcessId, pget->HandleValue);
		}
		free(pget);
	}
	pget = NULL;
	return ret;
}

int push_input_handles(pinput_handles_t phdls, PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX ptable)
{
	int ret = 0;
	PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pinsert = NULL;
	pinsert = (PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX)malloc(sizeof(*pinsert));
	if (pinsert == NULL) {
		GETERRNO(ret);
		return ret;
	}

	memcpy(pinsert, ptable, sizeof(*pinsert));

	EnterCriticalSection(&(phdls->m_cs));
	phdls->m_pvec->push_back(pinsert);
	ret = (int)phdls->m_pvec->size();
	LeaveCriticalSection(&(phdls->m_cs));
	return ret;
}

pinput_handles_t alloc_input_handles(void)
{
	pinput_handles_t phdls = NULL;
	int ret;

	phdls = (pinput_handles_t)malloc(sizeof(*phdls));
	if (phdls == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	memset(phdls, 0, sizeof(*phdls));
	InitializeCriticalSection(&(phdls->m_cs));
	phdls->m_pvec = new std::vector<PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX>();
	return phdls;
fail:
	free_input_handles(&phdls);
	SETERRNO(ret);
	return NULL;
}

void free_output_info(poutput_infos_t *ppinfos)
{
	if (ppinfos && *ppinfos) {
		poutput_infos_t pinfos = *ppinfos;
		if (pinfos->m_pvec != NULL) {
			while (pinfos->m_pvec->size() > 0) {
				phandle_info_t pinfo = pinfos->m_pvec->at(0);
				pinfos->m_pvec->erase(pinfos->m_pvec->begin());
				free(pinfo);
				pinfo = NULL;
			}
			delete pinfos->m_pvec;
			pinfos->m_pvec = NULL;
		}
		free(pinfos);
		*ppinfos = NULL;
	}
	return;
}

poutput_infos_t alloc_output_infos(void)
{
	poutput_infos_t pinfos = NULL;
	int ret;

	pinfos = (poutput_infos_t)malloc(sizeof(*pinfos));
	if (pinfos == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	memset(pinfos, 0, sizeof(*pinfos));
	InitializeCriticalSection(&(pinfos->m_cs));
	pinfos->m_pvec = new std::vector<phandle_info_t>();
	return pinfos;
fail:
	free_output_info(&pinfos);
	SETERRNO(ret);
	return NULL;
}

int push_output_info(poutput_infos_t pinfos, phandle_info_t *ppinfo)
{
	int ret = 0;
	EnterCriticalSection(&(pinfos->m_cs));
	pinfos->m_pvec->push_back(*ppinfo);
	*ppinfo = NULL;
	ret = (int)pinfos->m_pvec->size();
	LeaveCriticalSection(&(pinfos->m_cs));
	return ret;
}

phandle_info_t get_output_info(poutput_infos_t pinfos)
{
	phandle_info_t poutput = NULL;
	EnterCriticalSection(&(pinfos->m_cs));
	if (pinfos->m_pvec->size() > 0) {
		poutput = pinfos->m_pvec->at(0);
		pinfos->m_pvec->erase(pinfos->m_pvec->begin());
	}
	LeaveCriticalSection(&(pinfos->m_cs));
	return poutput;
}

DWORD WINAPI get_handle_info_thread(void* args)
{
	pthread_vars_t pthrvars = (pthread_vars_t) args;
	int ret;
	pinfo_variables_t pvars = pthrvars->m_pvars;
	pinput_handles_t pinput = pthrvars->m_pinput;
	poutput_infos_t poutput = pthrvars->m_poutput;
	PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX ptable;
	int curpid = (int)GetCurrentProcessId();
	int curthrid = (int)GetCurrentThreadId();
	int lasterrpid = -1;
	int idx = 0;
	NTSTATUS status;
	NtDuplicateObject_fn_t pNtDuplicateObject = NULL;
	NtQueryObject_fn_t pNtQueryObject = NULL;
	ULONG uret;
	POBJECT_TYPE_INFORMATION ptinfo;
	POBJECT_NAME_INFORMATION pninfo;
	phandle_info_t phdlinfo;

	if (pvars->m_tinfosize == 0) {
		pvars->m_tinfosize = 4;
	}

	if (pvars->m_ninfosize == 0) {
		pvars->m_ninfosize = 4;
	}

	if (pvars->m_table == NULL) {
		pvars->m_table = (PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX)malloc(sizeof(*(pvars->m_table)));
		if (pvars->m_table == NULL) {
			GETERRNO(ret);
			goto out;
		}
	}
	ptable = pvars->m_table;

	ASSERT_IF(pvars->m_ntmod != NULL);

	pNtQueryObject = reinterpret_cast<NtQueryObject_fn_t>(reinterpret_cast<void*>(GetProcAddress(pvars->m_ntmod, "NtQueryObject")));
	if (pNtQueryObject == NULL) {
		GETERRNO(ret);
		ERROR_INFO("[%d].[%d] GetProcAddress NtQueryObject error[%d]", curpid, curthrid, ret);
		goto out;
	}

	pNtDuplicateObject = reinterpret_cast<NtDuplicateObject_fn_t>(reinterpret_cast<void*>(GetProcAddress(pvars->m_ntmod, "NtDuplicateObject")));
	if (pNtDuplicateObject == NULL) {
		GETERRNO(ret);
		ERROR_INFO("[%d].[%d] GetProcAddress NtDuplicateObject error[%d]", curpid, curthrid, ret);
		goto out;
	}


	while (1) {
		ret = peek_input_handles(pinput, ptable);
		if (ret == 0) {
			/*all is ok ,so we should exit*/
			break;
		}

		if ((int)ptable->UniqueProcessId == curpid) {
			DEBUG_INFO("[%d].[%d] skip same process", curpid, curthrid);
			goto next_cycle;
		}

		if (lasterrpid >= 0 && lasterrpid == (int) ptable->UniqueProcessId) {
			goto next_cycle;
		}

		/*ok this is the */
		if (pvars->m_lastpid < 0 || pvars->m_lastpid != (int)ptable->UniqueProcessId) {
			if (pvars->m_prochdl != NULL) {
				CloseHandle(pvars->m_prochdl);
			}
			pvars->m_prochdl = NULL;
			pvars->m_prochdl = OpenProcess(PROCESS_DUP_HANDLE, FALSE, (DWORD)ptable->UniqueProcessId);
			if (pvars->m_prochdl == NULL) {
				if (lasterrpid != (int)ptable->UniqueProcessId) {
					GETERRNO(ret);
					ERROR_INFO("[%d].[%d] can not open [%d] process error[%d]", curpid, curthrid, (int)(ptable->UniqueProcessId), ret);
					lasterrpid = (int) ptable->UniqueProcessId;
				}
				goto next_cycle;
			}
			pvars->m_lastpid = (int) ptable->UniqueProcessId;
		}


		if (pvars->m_duphdl != NULL) {
			CloseHandle(pvars->m_duphdl);
		}
		pvars->m_duphdl = NULL;
		status = pNtDuplicateObject(pvars->m_prochdl, (HANDLE)ptable->HandleValue, NtCurrentProcess(), &(pvars->m_duphdl), 0, 0, DUPLICATE_SAME_ACCESS);
		if (!NT_SUCCESS(status)) {
			GETERRNO(ret);
			pvars->m_duphdl = NULL;
			ERROR_INFO("[%d].[%d]can not open [%d] handle [0x%x] error[0x%lx] [%d]", curpid, curthrid, pvars->m_lastpid, ptable->HandleValue, status, ret);
			goto next_cycle;
		}

		/**/
get_type_again:
		if (pvars->m_ptypeinfo) {
			free(pvars->m_ptypeinfo);
		}
		pvars->m_ptypeinfo = NULL;
		pvars->m_ptypeinfo = malloc(pvars->m_tinfosize);
		if (pvars->m_ptypeinfo == NULL) {
			GETERRNO(ret);
			goto out;
		}
		memset(pvars->m_ptypeinfo, 0, pvars->m_tinfosize);

		status = pNtQueryObject(pvars->m_duphdl, ObjectTypeInformation, pvars->m_ptypeinfo, pvars->m_tinfosize, &uret);
		if (!NT_SUCCESS(status)) {
			if (status == STATUS_INFO_LENGTH_MISMATCH) {
				if (pvars->m_tinfosize < uret) {
					pvars->m_tinfosize = uret;
				} else {
					pvars->m_tinfosize <<= 1;
					if (pvars->m_tinfosize == 0) {
						pvars->m_tinfosize = 4;
					}
				}
				WARN_INFO("[%d].[%d] query [%d].[0x%x] duphdl [0x%x] expand size [0x%lx]", curpid, curthrid, pvars->m_lastpid, ptable->HandleValue, pvars->m_duphdl, pvars->m_tinfosize);
				goto get_type_again;
			}
			GETERRNO(ret);
			ERROR_INFO("[%d].[%d] query [%d].[0x%p] duphdl [0x%x] error[0x%x] [%d]", curpid, curthrid, pvars->m_lastpid, ptable->HandleValue, pvars->m_duphdl, status, ret);
			goto out;
		}

		ptinfo = (POBJECT_TYPE_INFORMATION) pvars->m_ptypeinfo;

		ret = UnicodeToAnsi(ptinfo->TypeName.Buffer, &(pvars->m_ptypename), &(pvars->m_typesize));
		if (ret < 0) {
			GETERRNO(ret);
			goto out;
		}

		if (str_nocase_cmp(pvars->m_ptypename, "file") != 0 &&
		        str_nocase_cmp(pvars->m_ptypename, "directory") != 0) {
			WARN_INFO("[%d].[%d] [%d].[0x%x] duphdl [0x%x] type [%s]", curpid, curthrid, pvars->m_lastpid, ptable->HandleValue, pvars->m_duphdl, pvars->m_ptypename);
			goto next_cycle;
		}

		/*now we should give the name*/
get_name_again:
		if (pvars->m_pnameinfo) {
			free(pvars->m_pnameinfo);
		}
		pvars->m_pnameinfo = NULL;
		pvars->m_pnameinfo = malloc(pvars->m_ninfosize);
		if (pvars->m_pnameinfo == NULL) {
			GETERRNO(ret);
			goto out;
		}
		memset(pvars->m_pnameinfo, 0, pvars->m_ninfosize);

		status = pNtQueryObject(pvars->m_duphdl, ObjectNameInformation, pvars->m_pnameinfo, pvars->m_ninfosize, &uret);
		if (!NT_SUCCESS(status)) {
			if (status == STATUS_INFO_LENGTH_MISMATCH) {
				if (pvars->m_ninfosize < uret) {
					pvars->m_ninfosize = uret;
				} else {
					pvars->m_ninfosize <<= 1;
					if (pvars->m_ninfosize == 0) {
						pvars->m_ninfosize = 4;
					}
				}
				WARN_INFO("[%d].[%d] query [%d].[0x%x] duphdl [0x%x] name expand size [0x%lx]", curpid, curthrid, pvars->m_lastpid, ptable->HandleValue, pvars->m_duphdl, pvars->m_ninfosize);
				goto get_name_again;
			}
			GETERRNO(ret);
			ERROR_INFO("[%d].[%d] query [%d].[0x%p] duphdl [0x%x] name error[0x%x] [%d]", curpid, curthrid, pvars->m_lastpid, ptable->HandleValue, pvars->m_duphdl, status, ret);
			goto next_cycle;
		}

		pninfo = (POBJECT_NAME_INFORMATION) pvars->m_pnameinfo;

		ret = UnicodeToAnsi(pninfo->Name.Buffer, &(pvars->m_pname), &(pvars->m_namesize));
		if (ret < 0) {
			GETERRNO(ret);
			goto out;
		}

		if (pvars->m_phdlinfo == NULL) {
			pvars->m_phdlinfo = (phandle_info_t)malloc(sizeof(*(pvars->m_phdlinfo)));
			if (pvars->m_phdlinfo == NULL) {
				GETERRNO(ret);
				goto out;
			}
		}
		memset(pvars->m_phdlinfo, 0 , sizeof(*(pvars->m_phdlinfo)));


		phdlinfo = pvars->m_phdlinfo;
		/**/
		phdlinfo->m_pid = (int)ptable->UniqueProcessId;
		phdlinfo->m_hdl = (HANDLE)ptable->HandleValue;
		if (pvars->m_ptypename != NULL) {
			strncpy_s(phdlinfo->m_typename, sizeof(phdlinfo->m_typename) - 1, pvars->m_ptypename, sizeof(phdlinfo->m_typename));	
		}		
		if (pvars->m_pname != NULL) {
			strncpy_s(phdlinfo->m_name, sizeof(phdlinfo->m_name) - 1, pvars->m_pname, sizeof(phdlinfo->m_name));
		}		
		push_output_info(poutput, &(pvars->m_phdlinfo));

next_cycle:
		/*we handle next one*/
		remove_input_handles(pinput,0);
		idx ++;
		SetEvent(pthrvars->m_notievt);
	}

	ret = 0;
out:
	pthrvars->m_exitcode = ret;
	SETERRNO(ret);
	pthrvars->m_exited = 1;
	return (DWORD)ret;
}

void free_thread_vars(pthread_vars_t* ppthrvars)
{
	BOOL bret;
	int ret;
	DWORD dret;
	if (ppthrvars && *ppthrvars) {
		pthread_vars_t pthrvars = *ppthrvars;
		if (pthrvars->m_exited == 0 && pthrvars->m_thrhdl != NULL) {
			int cnt = 0;
			while (cnt < 5) {
				SetEvent(pthrvars->m_exitevt);
				dret = WaitForSingleObject(pthrvars->m_thrhdl, 10);
				if (dret == WAIT_OBJECT_0) {
					if (pthrvars->m_exited > 0) {
						break;
					}
				}
				cnt ++;
			}

			if (pthrvars->m_exited == 0) {
				bret = TerminateThread(pthrvars->m_thrhdl, 20);
				if (!bret) {
					GETERRNO(ret);
					ERROR_INFO("can not terminate thread error[%d]", ret);
				}
			}
		}
		pthrvars->m_exited = 1;
		if (pthrvars->m_thrhdl != NULL) {
			bret = CloseHandle(pthrvars->m_thrhdl);
			if (!bret) {
				GETERRNO(ret);
				ERROR_INFO("can not close thread [%d]", ret);
			}
			pthrvars->m_thrhdl = NULL;
		}

		if (pthrvars->m_exitevt) {
			CloseHandle(pthrvars->m_exitevt);
		}
		pthrvars->m_exitevt = NULL;

		if (pthrvars->m_notievt) {
			CloseHandle(pthrvars->m_notievt);
		}
		pthrvars->m_notievt = NULL;

		free_input_handles(&(pthrvars->m_pinput));
		free_output_info(&(pthrvars->m_poutput));
		free_info_variables(&(pthrvars->m_pvars));
		pthrvars->m_exited = 1;
		pthrvars->m_exitcode = 0;
		free(pthrvars);
		*ppthrvars = NULL;
	}
}

pthread_vars_t alloc_thread_vars(void)
{
	pthread_vars_t pthrvars = NULL;
	int ret;

	pthrvars = (pthread_vars_t) malloc(sizeof(*pthrvars));
	if (pthrvars == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	memset(pthrvars, 0, sizeof(*pthrvars));
	pthrvars->m_exited = 1;

	pthrvars->m_exitevt = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (pthrvars->m_exitevt == NULL) {
		GETERRNO(ret);
		ERROR_INFO("cannot init exitevt error[%d]", ret);
		goto fail;
	}

	pthrvars->m_notievt = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (pthrvars->m_notievt == NULL) {
		GETERRNO(ret);
		ERROR_INFO("cannot init notievt error[%d]", ret);
		goto fail;
	}

	pthrvars->m_pinput = alloc_input_handles();
	if (pthrvars->m_pinput == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	pthrvars->m_poutput = alloc_output_infos();
	if (pthrvars->m_poutput == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	pthrvars->m_pvars = alloc_info_variables();
	if (pthrvars->m_pvars == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	return pthrvars;
fail:
	free_thread_vars(&pthrvars);
	SETERRNO(ret);
	return NULL;
}

int restart_background_thread(pthread_vars_t pthrvars, HMODULE hmod)
{
	DWORD thrid;
	BOOL bret;
	int ret;
	if (pthrvars->m_thrhdl != NULL) {
		/*now it is exited*/
		bret = TerminateThread(pthrvars->m_thrhdl, 20);
		if (!bret) {
			GETERRNO(ret);
			ERROR_INFO("terminate thread error[%d]", ret);
			goto fail;
		}
		CloseHandle(pthrvars->m_thrhdl);
		pthrvars->m_thrhdl = NULL;
		pthrvars->m_exited = 1;

		/*now we should remove the first*/
		ret = remove_input_handles(pthrvars->m_pinput,1);
		if (ret == 0) {
			/*nothing to handle*/
			return 0;
		}
	}

	pthrvars->m_exited = 1;
	free_info_variables(&(pthrvars->m_pvars));
	pthrvars->m_pvars = alloc_info_variables();
	if (pthrvars->m_pvars == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	pthrvars->m_pvars->m_ntmod = hmod;

	pthrvars->m_exited = 0;
	pthrvars->m_thrhdl = CreateThread(NULL, 0, get_handle_info_thread, pthrvars, 0, &thrid);
	if (pthrvars->m_thrhdl == NULL) {
		GETERRNO(ret);
		pthrvars->m_exited = 1;
		ERROR_INFO("create thread error[%d]", ret);
		goto fail;
	}

	return 1;
fail:
	SETERRNO(ret);
	return ret;
}


int get_handle_infos(int freed, phandle_info_t* pphdls, int *psize)
{
	int retsize = 0;
	phandle_info_t prethdl = NULL;
	phandle_info_t ptmphdl = NULL;
	int retlen = 0;
	int ret;
	NTSTATUS status;
	ULONG uret;
	ULONG bufsize = 32;
	PVOID pbuffer = NULL;
	unsigned int i;
	PSYSTEM_HANDLE_INFORMATION_EX pinfoex = NULL;
	NtQuerySystemInformation_fn_t pNtQuerySystemInformation = NULL;
	HANDLE waithdls[2];
	DWORD waitnum;
	phandle_info_t pinfo = NULL;
	HMODULE hmod = NULL;
	pthread_vars_t pthrvars = NULL;
	DWORD dret;

	if (freed > 0) {
		if (pphdls && *pphdls) {
			free(*pphdls);
			*pphdls = NULL;
		}

		if (psize) {
			*psize = 0;
		}
		return 0;
	}

	if (pphdls == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	prethdl = *pphdls;
	retsize = *psize;

	hmod = LoadLibraryA("ntdll.dll");
	if (hmod == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	pNtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformation_fn_t>(reinterpret_cast<void*>(GetProcAddress(hmod, "NtQuerySystemInformation")));
	if (pNtQuerySystemInformation == NULL) {
		GETERRNO(ret);
		ERROR_INFO("no NtQuerySystemInformation error[%d]", ret);
		goto fail;
	}

get_info_again:
	if (pbuffer) {
		free(pbuffer);
	}
	pbuffer = NULL;
	pbuffer = malloc(bufsize);
	if (pbuffer == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	status = pNtQuerySystemInformation(SystemExtendedHandleInformation, pbuffer, bufsize, &uret);
	if (!NT_SUCCESS(status)) {
		if (status == STATUS_INFO_LENGTH_MISMATCH) {
			if (bufsize < uret) {
				bufsize = uret;
			} else {
				bufsize <<= 1;
			}
			goto get_info_again;
		}
		GETERRNO(ret);
		ERROR_INFO("can not get SystemExtendedHandleInformation error[%ld] [%d]", status, ret);
		goto fail;
	}

	pthrvars = alloc_thread_vars();
	if (pthrvars == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	/*now to input */
	pinfoex = (PSYSTEM_HANDLE_INFORMATION_EX) pbuffer;
	DEBUG_INFO("handles [%d]", pinfoex->NumberOfHandles);
	for (i = 0 ; i < pinfoex->NumberOfHandles; i ++) {
		ret = push_input_handles(pthrvars->m_pinput, &(pinfoex->Handles[i]));
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
	}

	ret = restart_background_thread(pthrvars, hmod);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	} else if (ret == 0) {
		goto succ;
	}


	while (1) {
		waitnum = 0;
		waithdls[waitnum] = pthrvars->m_notievt;
		waitnum ++;
		waithdls[waitnum] = pthrvars->m_thrhdl;
		waitnum ++;
		dret = WaitForMultipleObjects(waitnum, waithdls, FALSE, 200);
		if (dret == WAIT_OBJECT_0) {
			ResetEvent(pthrvars->m_notievt);
			while (1) {
				pinfo = get_output_info(pthrvars->m_poutput);
				if (pinfo == NULL) {
					break;
				}
				if (retlen >= retsize) {
					if (retsize == 0) {
						retsize = 4;
					} else {
						retsize <<= 1;
					}
					ptmphdl = (phandle_info_t)malloc(retsize * sizeof(*ptmphdl));
					if (ptmphdl == NULL) {
						GETERRNO(ret);
						goto fail;
					}
					memset(ptmphdl, 0, sizeof(*ptmphdl) * retsize);
					if (retlen > 0) {
						memcpy(ptmphdl, prethdl, sizeof(*ptmphdl) * retlen);
					}
					if (prethdl && prethdl != *pphdls) {
						free(prethdl);
					}
					prethdl = ptmphdl;
					ptmphdl = NULL;
				}
				DEBUG_INFO("[%d] [%s][%s]", retlen, pinfo->m_typename, pinfo->m_name);
				memcpy(&(prethdl[retlen]), pinfo, sizeof(*pinfo));
				retlen ++;
				free(pinfo);
				pinfo = NULL;
			}
		} else if (dret == (WAIT_OBJECT_0 + 1)) {
			DEBUG_INFO("EXITED");
			break;
		} else if (dret == WAIT_TIMEOUT) {
			DEBUG_INFO("WAIT_TIMEOUT");
			ret = restart_background_thread(pthrvars, hmod);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			} else if (ret == 0) {
				break;
			}
		} else {
			GETERRNO(ret);
			ERROR_INFO("can not wait error [%ld] [%d]", dret, ret);
			goto fail;
		}
	}

	while (pthrvars->m_exited == 0) {
		sleep_mill(1);
	}

	DEBUG_INFO("exitcode [%d]", pthrvars->m_exitcode);
	if (pthrvars->m_exitcode != 0) {
		ret = pthrvars->m_exitcode;
		goto fail;
	}

succ:
	while (1) {
		pinfo = get_output_info(pthrvars->m_poutput);
		if (pinfo == NULL) {
			break;
		}
		if (retlen >= retsize) {
			if (retsize == 0) {
				retsize = 4;
			} else {
				retsize <<= 1;
			}
			ptmphdl = (phandle_info_t)malloc(retsize * sizeof(*ptmphdl));
			if (ptmphdl == NULL) {
				GETERRNO(ret);
				goto fail;
			}
			memset(ptmphdl, 0, sizeof(*ptmphdl) * retsize);
			if (retlen > 0) {
				memcpy(ptmphdl, prethdl, sizeof(*ptmphdl) * retlen);
			}
			if (prethdl && prethdl != *pphdls) {
				free(prethdl);
			}
			prethdl = ptmphdl;
			ptmphdl = NULL;
		}

		memcpy(&(prethdl[retlen]), pinfo, sizeof(*pinfo));
		retlen ++;
		free(pinfo);
		pinfo = NULL;
	}

	if (*pphdls && *pphdls != prethdl) {
		free(*pphdls);
	}
	*pphdls = prethdl;
	*psize = retsize;

	if (ptmphdl) {
		free(ptmphdl);
	}
	ptmphdl = NULL;


	if (pinfo) {
		free(pinfo);
	}
	pinfo = NULL;

	free_thread_vars(&pthrvars);

	if (pbuffer) {
		free(pbuffer);
	}
	pbuffer = NULL;


	if (hmod != NULL) {
		FreeLibrary(hmod);
	}
	hmod = NULL;

	return retlen;
fail:

	if (prethdl && prethdl != *pphdls) {
		free(prethdl);
	}
	prethdl = NULL;
	retsize = 0;

	if (ptmphdl) {
		free(ptmphdl);
	}
	ptmphdl = NULL;

	if (pinfo) {
		free(pinfo);
	}
	pinfo = NULL;

	free_thread_vars(&pthrvars);

	if (pbuffer) {
		free(pbuffer);
	}
	pbuffer = NULL;


	if (hmod != NULL) {
		FreeLibrary(hmod);
	}
	hmod = NULL;
	SETERRNO(ret);
	return ret;
}

#pragma warning(pop)