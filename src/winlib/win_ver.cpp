#include <win_ver.h>
#include <win_err.h>
#include <win_output_debug.h>
#include <win_regop.h>

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

#define  VERSION_REG_PATH   "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define  CURRENT_VERSION    "CurrentVersion"
#define  INSTALLATION_TYPE  "InstallationType"

int __get_version(int freed, char** ppversion,int *pversize)
{
	void* pregop=NULL;
	int ret;
	int vlen=0;

	if (freed) {
		query_hklm_string(NULL,NULL,ppversion,pversize);
		return 0;
	}

	pregop = open_hklm(VERSION_REG_PATH,ACCESS_KEY_READ);
	if (pregop == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	ret = query_hklm_string(pregop,CURRENT_VERSION,ppversion,pversize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	vlen = ret;
	close_hklm(&pregop);

	return vlen;

fail:
	query_hklm_string(NULL,NULL,ppversion,pversize);
	close_hklm(&pregop);
	SETERRNO(ret);
	return ret;
}

int __get_type(int freed,char** pptype,int *ptypesize)
{
	void* pregop=NULL;
	int ret;
	int vlen=0;

	if (freed) {
		query_hklm_string(NULL,NULL,pptype,ptypesize);
		return 0;
	}

	pregop = open_hklm(VERSION_REG_PATH,ACCESS_KEY_READ);
	if (pregop == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	ret = query_hklm_string(pregop,INSTALLATION_TYPE,pptype,ptypesize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	vlen = ret;
	close_hklm(&pregop);

	return vlen;

fail:
	query_hklm_string(NULL,NULL,pptype,ptypesize);
	close_hklm(&pregop);
	SETERRNO(ret);
	return ret;
}

int is_win7()
{
	int isvalid = 0;
	char* ptype=NULL;
	int typesize=0;
	char* pversion=NULL;
	int versize=0;
	int ret;

	ret = __get_version(0,&pversion,&versize);
	if (ret < 0) {
		goto fail;
	}

	ret = __get_type(0,&ptype,&typesize);
	if (ret < 0) {
		goto fail;
	}

	if (strcmp(pversion,"6.1") == 0 && 
		_stricmp(ptype,"Client") == 0) {
		isvalid = 1;
	}

	__get_version(1,&pversion,&versize);
	__get_type(1, &ptype,&typesize);
	return isvalid;
fail:
	__get_version(1,&pversion,&versize);
	__get_type(1, &ptype,&typesize);
	return 0;
}

int is_win10()
{
	int isvalid = 0;
	char* ptype=NULL;
	int typesize=0;
	char* pversion=NULL;
	int versize=0;
	int ret;

	ret = __get_version(0,&pversion,&versize);
	if (ret < 0) {
		goto fail;
	}

	ret = __get_type(0,&ptype,&typesize);
	if (ret < 0) {
		goto fail;
	}

	if (strcmp(pversion,"6.3") == 0 && 
		_stricmp(ptype,"Client") == 0) {
		isvalid = 1;
	}

	__get_version(1,&pversion,&versize);
	__get_type(1, &ptype,&typesize);
	return isvalid;
fail:
	__get_version(1,&pversion,&versize);
	__get_type(1, &ptype,&typesize);
	return 0;
}

int is_winserver_2019(void)
{
	int isvalid = 0;
	char* ptype=NULL;
	int typesize=0;
	char* pversion=NULL;
	int versize=0;
	int ret;

	ret = __get_version(0,&pversion,&versize);
	if (ret < 0) {
		goto fail;
	}

	ret = __get_type(0,&ptype,&typesize);
	if (ret < 0) {
		goto fail;
	}

	if (strcmp(pversion,"6.3") == 0 && 
		_stricmp(ptype,"Server") == 0) {
		isvalid = 1;
	}

	__get_version(1,&pversion,&versize);
	__get_type(1, &ptype,&typesize);
	return isvalid;
fail:
	__get_version(1,&pversion,&versize);
	__get_type(1, &ptype,&typesize);
	return 0;
}