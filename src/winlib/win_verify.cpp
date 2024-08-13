#pragma warning(disable:4668)
#pragma warning(disable:4820)

#include <win_verify.h>
#include <win_err.h>
#include <win_output_debug.h>
#include <Wintrust.h>
#include <wincrypt.h>
#include <Softpub.h>
#include <win_uniansi.h>

#pragma warning(default:4820)
#pragma warning(default:4668)

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#pragma comment(lib,"Wintrust.lib")

int verify_windows_pe(const char* fname)
{
	int ret;
	wchar_t* pwfname=NULL;
	int wfnamesize=0;
	LONG lstatus;
	WINTRUST_FILE_INFO fdata;
	GUID policyguid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_DATA wdata;

	memset(&fdata,0,sizeof(fdata));
	memset(&wdata,0,sizeof(wdata));

	ret = AnsiToUnicode((char*)fname,&pwfname,&wfnamesize);
	if (ret < 0) {
		GETERRNO(ret);
		ERROR_INFO(" ");
		goto fail;
	}

	fdata.cbStruct = sizeof(fdata);
	fdata.pcwszFilePath  = pwfname;
	fdata.hFile = NULL;
	fdata.pgKnownSubject = NULL;

	wdata.cbStruct = sizeof(wdata);
	wdata.pPolicyCallbackData  = NULL;
	wdata.pSIPClientData  = NULL;
	wdata.dwUIChoice  = WTD_UI_NONE;
	wdata.fdwRevocationChecks  = WTD_REVOKE_NONE;
	wdata.dwUnionChoice  = WTD_CHOICE_FILE;
	wdata.dwStateAction  = WTD_STATEACTION_VERIFY;
	wdata.hWVTStateData  = NULL;
	wdata.pwszURLReference  = NULL;
	wdata.dwUIContext  = NULL;
	wdata.pFile = &fdata;

	lstatus = WinVerifyTrust(NULL,&policyguid,&wdata);
	if (lstatus != ERROR_SUCCESS) {
		GETERRNO(ret);
		ERROR_INFO("check [%s] lstatus[%ld] error[%d]",fname,lstatus,ret);
		goto fail;
	}

	AnsiToUnicode(NULL,&pwfname,&wfnamesize);
	return 0;
fail:
	ERROR_INFO("ret [%d]",ret);
	AnsiToUnicode(NULL,&pwfname,&wfnamesize);
	SETERRNO(ret);
	return ret;
}