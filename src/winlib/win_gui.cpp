
#include <win_gui.h>
#include <win_err.h>
#include <win_uniansi.h>

#pragma warning(push)

#if defined(_MSC_VER)
#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif
#endif


int enum_display_devices(int freed, pdisplay_name_t* ppdevices, int *psize)
{
	pdisplay_name_t pdevs = NULL;
	pdisplay_name_t ptmps = NULL;
	int retsize = 0;
	int retlen = 0;
	int ret;
	BOOL bret;
	PDISPLAY_DEVICEA pdisp = NULL;
	if (freed) {
		if (ppdevices && *ppdevices) {
			free(*ppdevices);
			*ppdevices = NULL;
		}
		if (psize) {
			*psize = 0;
		}
		return 0;
	}

	if (ppdevices == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}
	retsize = *psize;
	pdevs = *ppdevices;

	pdisp = (PDISPLAY_DEVICEA)malloc(sizeof(*pdisp));
	if (pdisp == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	for (;;) {
		memset(pdisp, 0, sizeof(*pdisp));
		pdisp->cb = sizeof(*pdisp);
		SETERRNO(0);
		bret = EnumDisplayDevicesA(NULL, (DWORD)retlen, pdisp, EDD_GET_DEVICE_INTERFACE_NAME);
		if (!bret) {
			GETERRNO_DIRECT(ret);
			if (ret == 0) {
				break;
			}
			ERROR_INFO("can not get [%d] error[%d]", retlen, ret);
			goto fail;
		}

		if (retlen == retsize || pdevs == NULL) {
			if (retsize == 0) {
				retsize = 4;
			} else {
				retsize <<= 1;
			}

			ptmps = (pdisplay_name_t) malloc(retsize * sizeof(*ptmps));
			if (ptmps == NULL) {
				GETERRNO(ret);
				goto fail;
			}

			memset(ptmps, 0, sizeof(*ptmps) * retsize);
			if (retlen > 0) {
				memcpy(ptmps, pdevs, retlen * sizeof(*ptmps));
			}

			if (pdevs != NULL && pdevs != *ppdevices) {
				free(pdevs);
			}
			pdevs = ptmps;
			ptmps = NULL;
		}

		DEBUG_INFO("[%d].DeviceName =[%s]", retlen, pdisp->DeviceName);
		DEBUG_INFO("[%d].DeviceString =[%s]", retlen, pdisp->DeviceString);
		DEBUG_INFO("[%d].DeviceID =[%s]", retlen, pdisp->DeviceID);
		DEBUG_INFO("[%d].DeviceKey =[%s]", retlen, pdisp->DeviceKey);
		DEBUG_INFO("[%d].StateFlags =[0x%x]", retlen, pdisp->StateFlags);
		strncpy_s(pdevs[retlen].m_name, sizeof(pdisp->DeviceName), pdisp->DeviceName, sizeof(pdisp->DeviceName));
		strncpy_s(pdevs[retlen].m_id, sizeof(pdisp->DeviceID), pdisp->DeviceID, sizeof(pdisp->DeviceID));
		strncpy_s(pdevs[retlen].m_key, sizeof(pdisp->DeviceKey), pdisp->DeviceKey, sizeof(pdisp->DeviceKey));
		strncpy_s(pdevs[retlen].m_devstr, sizeof(pdisp->DeviceString), pdisp->DeviceString, sizeof(pdisp->DeviceString));
		pdevs[retlen].m_state = (int)pdisp->StateFlags;
		retlen ++;
	}

	if (pdisp) {
		free(pdisp);
	}
	pdisp = NULL;

	if (*ppdevices && *ppdevices != pdevs) {
		free(*ppdevices);
	}
	*ppdevices = pdevs;
	*psize = retsize;

	return retlen;
	fail:
	if (pdisp) {
		free(pdisp);
	}
	pdisp = NULL;


	if (pdevs != NULL && pdevs != *ppdevices) {
		free(pdevs);
	}
	pdevs = NULL;
	SETERRNO(ret);
	return ret;
}

int enum_display_mode(char* devname, pdisplay_mode_t* ppmode, int *psize)
{
	int ret;
	PDEVMODEA pmode = NULL;
	pdisplay_mode_t pretmode = NULL;
	pdisplay_mode_t ptmps = NULL;
	int retsize = 0;
	int retlen = 0;
	BOOL bret;
	if (devname == NULL) {
		if (ppmode && *ppmode) {
			free(*ppmode);
			*ppmode = NULL;
		}
		if (psize) {
			*psize = 0;
		}
		return 0;
	}

	if (ppmode == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	pretmode = *ppmode;
	retsize = *psize;

	pmode = (PDEVMODEA) malloc(sizeof(*pmode));
	if (pmode == NULL) {
		GETERRNO(ret);
		goto fail;
	}


	for (;;) {
		memset(pmode, 0, sizeof(*pmode));
		pmode->dmSize = sizeof(*pmode);
		SETERRNO(0);
		bret = EnumDisplaySettingsExA(devname, (DWORD)retlen, pmode, 0);
		if (!bret) {
			GETERRNO_DIRECT(ret);
			if (ret == 0) {
				break;
			}
			ERROR_INFO("[%s].[%d] error[%d]", devname, retlen, ret);
			goto fail;
		}

		if (retlen == retsize || pretmode == NULL) {
			if (retsize == 0) {
				retsize = 4;
			} else {
				retsize <<= 1;
			}

			ptmps = (pdisplay_mode_t) malloc(retsize * sizeof(*ptmps));
			if (ptmps == NULL) {
				GETERRNO(ret);
				goto fail;
			}

			memset(ptmps, 0, retsize * sizeof(*ptmps));
			if (retlen > 0) {
				memcpy(ptmps, pretmode, retlen * sizeof(*ptmps));
			}
			if (pretmode && pretmode != *ppmode) {
				free(pretmode);
			}
			pretmode = ptmps;
			ptmps = NULL;
		}

		// DEBUG_INFO("[%s].[%d].dmDeviceName=[%s]", devname,retlen, pmode->dmDeviceName);
		// DEBUG_INFO("[%s].[%d].dmFormName=[%s]", devname,retlen, pmode->dmFormName);
		// DEBUG_INFO("[%s].[%d].dmSpecVersion=[0x%x]", devname,retlen, pmode->dmSpecVersion);
		// DEBUG_INFO("[%s].[%d].dmDriverVersion=[0x%x]", devname,retlen, pmode->dmDriverVersion);
		// DEBUG_INFO("[%s].[%d].dmSize=[0x%x]", devname,retlen, pmode->dmSize);
		// DEBUG_INFO("[%s].[%d].dmDriverExtra=[0x%x]", devname,retlen, pmode->dmDriverExtra);
		// DEBUG_INFO("[%s].[%d].dmFields=[0x%x]", devname,retlen, pmode->dmFields);
		// DEBUG_INFO("[%s].[%d].dmDisplayFrequency=[0x%x]", devname,retlen, pmode->dmDisplayFrequency);
		// DEBUG_INFO("[%s].[%d].dmPelsWidth=[0x%x]", devname,retlen, pmode->dmPelsWidth);
		// DEBUG_INFO("[%s].[%d].dmPelsHeight=[0x%x]", devname,retlen, pmode->dmPelsHeight);
		// DEBUG_INFO("[%s].[%d].dmBitsPerPel=[0x%x]", devname,retlen, pmode->dmBitsPerPel);
		// DEBUG_INFO("[%s].[%d].dmLogPixels=[0x%x]", devname,retlen, pmode->dmLogPixels);

		pretmode[retlen].m_width = (int)pmode->dmPelsWidth;
		pretmode[retlen].m_height = (int)pmode->dmPelsHeight;
		pretmode[retlen].m_refresh = (int)pmode->dmDisplayFrequency;
		// DEBUG_BUFFER_FMT(pmode->dmDeviceName,sizeof(pmode->dmDeviceName),"[%s].[%d].dmDeviceName", devname,retlen);
		DEBUG_BUFFER_FMT(pmode, sizeof(*pmode), "[%d] mode", retlen);
		strncpy_s(pretmode[retlen].m_name, sizeof(pretmode[retlen].m_name) - 1, devname, sizeof(pretmode[retlen].m_name));
		strncpy_s(pretmode[retlen].m_devname, sizeof(pretmode[retlen].m_devname), (char*)pmode->dmDeviceName, sizeof(pretmode[retlen].m_devname));
		retlen ++;
	}

	if (pmode) {
		free(pmode);
	}
	pmode = NULL;

	if (*ppmode && *ppmode != pretmode) {
		free(*ppmode);
	}
	*ppmode = pretmode;
	*psize = retsize;

	return retlen;
	fail:
	if (pmode) {
		free(pmode);
	}
	pmode = NULL;
	if (pretmode && pretmode != *ppmode) {
		free(pretmode);
	}
	pretmode = NULL;
	SETERRNO(ret);
	return ret;
}

int set_display_mode(pdisplay_mode_t pmode, DWORD flags)
{
	PDEVMODEA pdevmode = NULL;
	LONG lret;
	int ret;

	pdevmode = (PDEVMODEA) malloc(sizeof(*pdevmode));
	if (pdevmode == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	memset(pdevmode, 0, sizeof(*pdevmode));
	pdevmode->dmSize = sizeof(*pdevmode);
	strncpy_s((char*)pdevmode->dmDeviceName, sizeof(pdevmode->dmDeviceName), pmode->m_devname, sizeof(pdevmode->dmDeviceName));
	pdevmode->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
	pdevmode->dmPelsWidth = (DWORD) pmode->m_width;
	pdevmode->dmPelsHeight = (DWORD) pmode->m_height;
	pdevmode->dmDisplayFrequency = (DWORD) pmode->m_refresh;

	lret = ChangeDisplaySettingsA(pdevmode, flags);
	if (lret != DISP_CHANGE_SUCCESSFUL) {
		GETERRNO(ret);
		ERROR_INFO("DISP_CHANGE_BADDUALVIEW [%d]", DISP_CHANGE_BADDUALVIEW);
		ERROR_INFO("DISP_CHANGE_SUCCESSFUL [%d]", DISP_CHANGE_SUCCESSFUL);
		ERROR_INFO("DISP_CHANGE_BADFLAGS [%d]", DISP_CHANGE_BADFLAGS);
		ERROR_INFO("DISP_CHANGE_BADMODE [%d]", DISP_CHANGE_BADMODE);
		ERROR_INFO("DISP_CHANGE_BADPARAM [%d]", DISP_CHANGE_BADPARAM);
		ERROR_INFO("DISP_CHANGE_FAILED [%d]", DISP_CHANGE_FAILED);
		ERROR_INFO("DISP_CHANGE_NOTUPDATED [%d]", DISP_CHANGE_NOTUPDATED);
		ERROR_INFO("DISP_CHANGE_RESTART [%d]", DISP_CHANGE_RESTART);
		ERROR_INFO("can not set [%s] flags[0x%x] error[%d] lret[%d]", pmode->m_name, flags, ret, lret);
		goto fail;
	}


	if (pdevmode) {
		free(pdevmode);
	}
	pdevmode = NULL;
	return 0;
	fail:
	if (pdevmode) {
		free(pdevmode);
	}
	pdevmode = NULL;
	SETERRNO(ret);
	return ret;
}

int get_display_info(int freed, pdisplay_info_t *ppinfo, int *psize)
{
	int retlen = 0;
	int ret;
	pdisplay_info_t pretinfo=NULL;
	int retsize=0;
	UINT32 flags = QDC_ONLY_ACTIVE_PATHS,numpaths=0,nummodes=0,getpaths=0,getmodes=0;
	UINT32 i;
	LONG lret;
	char *friedlyname=NULL;
	int friendsize=0;
	int cpylen=0;
	DISPLAYCONFIG_PATH_INFO* pinfos=NULL;
	DISPLAYCONFIG_MODE_INFO* pmodes=NULL;
	DISPLAYCONFIG_TARGET_DEVICE_NAME* pdevname=NULL;
	if (freed) {
		if (ppinfo && *ppinfo)  {
			free(*ppinfo);
			*ppinfo = NULL;
		}
		if (psize) {
			*psize=0;
		}
		return 0;
	}

	if (ppinfo == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	pretinfo = *ppinfo;
	retsize = *psize;

	lret = GetDisplayConfigBufferSizes(flags,&numpaths,&nummodes);
	if (lret != ERROR_SUCCESS) {
		GETERRNO(ret);
		ERROR_INFO("GetDisplayConfigBufferSizes error[%d]",ret);
		goto fail;
	}

	if (numpaths > 0) {
		pinfos = (DISPLAYCONFIG_PATH_INFO*)malloc(sizeof(*pinfos) * numpaths);
		if (pinfos == NULL) {
			GETERRNO(ret);
			goto fail;
		}
		memset(pinfos,0, sizeof(*pinfos) * numpaths);
	}

	if (nummodes > 0) {
		pmodes =(DISPLAYCONFIG_MODE_INFO*) malloc(sizeof(*pmodes) * nummodes);
		if (pmodes == NULL) {
			GETERRNO(ret);
			goto fail;
		}
		memset(pmodes, 0, sizeof(*pmodes) * nummodes);
	}

	getpaths = numpaths;
	getmodes = nummodes;
	lret = QueryDisplayConfig(flags,&getpaths,pinfos,&getmodes,pmodes,NULL);
	if (lret != ERROR_SUCCESS) {
		GETERRNO(ret);
		ERROR_INFO("QueryDisplayConfig error[%d] lret [%ld]", ret,lret);
		goto fail;
	}

	if (getpaths != numpaths || getmodes != nummodes) {
		ret = -ERROR_REPLY_MESSAGE_MISMATCH;
		ERROR_INFO("getpaths [%ld] != numpaths[%ld] || getmodes[%ld] != nummodes [%ld]",
				getpaths,numpaths,getmodes,nummodes);
		goto fail;
	}

	if (retsize < (int)getpaths) {
		retsize = (int)getpaths;
		pretinfo = (pdisplay_info_t)malloc(sizeof(*pretinfo) * retsize);
		if (pretinfo == NULL) {
			GETERRNO(ret);
			goto fail;
		}
	}

	if (pretinfo != NULL) {
		memset(pretinfo,0,retsize * sizeof(*pretinfo));
	}

	for(i=0;i<numpaths;i++) {
		pretinfo[i].m_targetid = pinfos[i].targetInfo.id;
		pretinfo[i].m_sourceid = pinfos[i].sourceInfo.id;
		memcpy(&pretinfo[i].m_targetluid,&pinfos[i].targetInfo.adapterId,sizeof(pretinfo[i].m_targetluid));

		if (pdevname == NULL) {
			pdevname = (DISPLAYCONFIG_TARGET_DEVICE_NAME*) malloc(sizeof(*pdevname));
		}
		if (pdevname !=NULL) {
			memset(pdevname,0,sizeof(*pdevname));
			pdevname->header.size = sizeof(*pdevname);
			pdevname->header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			memcpy(&(pdevname->header.adapterId),&(pretinfo[i].m_targetluid),sizeof(pdevname->header.adapterId));
			pdevname->header.id = pretinfo[i].m_targetid;
			lret = DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)pdevname);
			if (lret == ERROR_SUCCESS) {
				DEBUG_BUFFER_FMT(pdevname->monitorFriendlyDeviceName,sizeof(pdevname->monitorFriendlyDeviceName),"[%ld]friedlyname",i);
				DEBUG_BUFFER_FMT(pdevname->monitorDevicePath,sizeof(pdevname->monitorDevicePath),"[%ld]devicepath",i);
				ret = UnicodeToAnsi(pdevname->monitorFriendlyDeviceName,&friedlyname,&friendsize);
				if (ret >= 0) {
					cpylen = ret;
					if (cpylen >= sizeof(pretinfo[i].m_devname)) {
						cpylen = sizeof(pretinfo[i].m_devname) - 1;
					}
					DEBUG_INFO("[%ld]cpylen [%d]", i,cpylen);
					if (cpylen > 0) {
						memcpy(pretinfo[i].m_devname, friedlyname,(size_t)cpylen);	
					}					
				} 

				ret = UnicodeToAnsi(pdevname->monitorDevicePath,&friedlyname,&friendsize);
				if (ret >= 0) {
					cpylen = ret;
					if (cpylen >= sizeof(pretinfo[i].m_devpath)) {
						cpylen = sizeof(pretinfo[i].m_devpath) - 1;
					}
					DEBUG_INFO("[%ld] path cpylen [%d]",i, cpylen);
					if (cpylen > 0) {
						memcpy(pretinfo[i].m_devpath,friedlyname,(size_t)cpylen);
					}
				}
			} else {
				ERROR_INFO("[%ld]DisplayConfigGetDeviceInfo for srcid [%ld] targetid [%ld] error [%ld]",i,pretinfo[i].m_sourceid,pretinfo[i].m_targetid);
			}

		}

		retlen ++;
	}

	UnicodeToAnsi(NULL,&friedlyname,&friendsize);

	if (pdevname) {
		free(pdevname);
	}
	pdevname = NULL;
	if (pinfos) {
		free(pinfos);
	}
	pinfos = NULL;
	if (pmodes) {
		free(pmodes);
	}
	pmodes = NULL;


	if (*ppinfo && *ppinfo != pretinfo) {
		free(*ppinfo);
	}
	*ppinfo = pretinfo;
	*psize = retsize;

	return retlen;
fail:
	UnicodeToAnsi(NULL,&friedlyname,&friendsize);

	if (pdevname) {
		free(pdevname);
	}
	pdevname = NULL;

	if (pinfos) {
		free(pinfos);
	}
	pinfos = NULL;
	if (pmodes) {
		free(pmodes);
	}
	pmodes = NULL;

	if (pretinfo && pretinfo != *ppinfo) {
		free(pretinfo);
	}
	pretinfo = NULL;
	retsize = 0;
	retlen = 0;
	SETERRNO(ret);
	return ret;
}


typedef struct _DISPLAYCONFIG_SOURCE_DPI_SCALE_GET
{
	DISPLAYCONFIG_DEVICE_INFO_HEADER            header;
    /*
        * @brief min value of DPI scaling is always 100, minScaleRel gives no. of steps down from recommended scaling
        * eg. if minScaleRel is -3 => 100 is 3 steps down from recommended scaling => recommended scaling is 175%
        */
	int32_t minScaleRel;

        /*
        * @brief currently applied DPI scaling value wrt the recommended value. eg. if recommended value is 175%,
        * => if curScaleRel == 0 the current scaling is 175%, if curScaleRel == -1, then current scale is 150%
        */
	int32_t curScaleRel;

        /*
        * @brief maximum supported DPI scaling wrt recommended value
        */
	int32_t maxScaleRel;
}DISPLAYCONFIG_SOURCE_DPI_SCALE_GET,*PDISPLAYCONFIG_SOURCE_DPI_SCALE_GET;

    /*
    * struct DISPLAYCONFIG_SOURCE_DPI_SCALE_SET
    * @brief set DPI scaling value of a source
    * Note that DPI scaling is a property of the source, and not of target.
    */
 typedef struct _DISPLAYCONFIG_SOURCE_DPI_SCALE_SET
    {
        DISPLAYCONFIG_DEVICE_INFO_HEADER            header;
        /*
        * @brief The value we want to set. The value should be relative to the recommended DPI scaling value of source.
        * eg. if scaleRel == 1, and recommended value is 175% => we are trying to set 200% scaling for the source
        */
        int32_t scaleRel;
    }DISPLAYCONFIG_SOURCE_DPI_SCALE_SET,*PDISPLAYCONFIG_SOURCE_DPI_SCALE_SET;


#define  DISPLAYCONFIG_DEVICE_INFO_GET_DPI_SCALE   -3
#define  DISPLAYCONFIG_DEVICE_INFO_SET_DPI_SCALE   -4

static const UINT32 DpiVals[] = { 100,125,150,175,200,225,250,300,350, 400, 450, 500 };

#define ARRAY_COUNT(cv) (sizeof(cv) / sizeof(cv[0]))

int get_display_rescale(pdisplay_info_t pinfo, uint32_t* pscale,uint32_t** ppavails,int* psize)
{
	DISPLAYCONFIG_SOURCE_DPI_SCALE_GET* scaleinfo=NULL;
	int ret;
	LONG lret;
	int needscales = 0;
	uint32_t* pretscales= NULL;
	int retsize=0;
	int retlen=0;
	UINT32 i,curi;
	int32_t tmp32;
	if (pinfo == NULL ) {
		if (ppavails && *ppavails) {
			free(*ppavails);
			*ppavails= NULL;
		}

		if (psize) {
			*psize = 0;
		}

		return 0;
	}

	if (pscale == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	if (ppavails != NULL) {
		if (psize == NULL) {
			ret = -ERROR_INVALID_PARAMETER;
			SETERRNO(ret);
			return ret;
		}
		needscales = 1;
		pretscales = *ppavails;
		retsize = *psize;
	}

	scaleinfo = (DISPLAYCONFIG_SOURCE_DPI_SCALE_GET*)malloc(sizeof(*scaleinfo));
	if (scaleinfo == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	memset(scaleinfo,0,sizeof(*scaleinfo));
	scaleinfo->header.size = sizeof(*scaleinfo);
	scaleinfo->header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE)DISPLAYCONFIG_DEVICE_INFO_GET_DPI_SCALE;
	scaleinfo->header.adapterId = pinfo->m_targetluid;
	scaleinfo->header.id = pinfo->m_sourceid;


	lret = DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)scaleinfo);
	if (lret != ERROR_SUCCESS) {
		GETERRNO(ret);
		ERROR_INFO("DisplayConfigGetDeviceInfo scale error[%ld]",lret);
		goto fail;
	}

	if (scaleinfo->minScaleRel < 0) {
		scaleinfo->minScaleRel = 0;
	}

	if (scaleinfo->maxScaleRel >= ARRAY_COUNT(DpiVals)) {
		scaleinfo->maxScaleRel = ARRAY_COUNT(DpiVals) - 1;
	}

	if (scaleinfo->maxScaleRel < scaleinfo->minScaleRel) {
		tmp32 = scaleinfo->maxScaleRel;
		scaleinfo->maxScaleRel = scaleinfo->minScaleRel;
		scaleinfo->minScaleRel = tmp32;
	}


	if (scaleinfo->curScaleRel < scaleinfo->minScaleRel) {
		scaleinfo->curScaleRel = scaleinfo->minScaleRel;
	}
	if (scaleinfo->curScaleRel > scaleinfo->maxScaleRel) {
		scaleinfo->curScaleRel = scaleinfo->maxScaleRel;
	}

	if (ARRAY_COUNT(DpiVals) > (scaleinfo->minScaleRel + scaleinfo->curScaleRel)) {
		*pscale = DpiVals[scaleinfo->minScaleRel +scaleinfo->curScaleRel];	
	} else {
		ret = -ERROR_PARAMETER_QUOTA_EXCEEDED;
		ERROR_INFO("minScaleRel [%ld] curScaleRel[%ld]", scaleinfo->minScaleRel,scaleinfo->curScaleRel);
		goto fail;
	}

	if (needscales) {
		retlen = 0;
		for(i=(UINT32)scaleinfo->minScaleRel;i<ARRAY_COUNT(DpiVals) && i <= (UINT32)scaleinfo->maxScaleRel;i++ ) {
			retlen ++;
		}

		if (retsize < retlen || pretscales == NULL) {
			retsize = retlen;
			pretscales = (uint32_t*)malloc(sizeof(*pretscales) * retlen);
			if (pretscales == NULL) {
				GETERRNO(ret);
				goto fail;
			}
		}

		if (pretscales) {
			memset(pretscales,0,retlen* sizeof(*pretscales));
		}

		for(i=(UINT32)scaleinfo->minScaleRel,curi = 0;i < ARRAY_COUNT(DpiVals) && i <= (UINT32)scaleinfo->maxScaleRel;i++,curi++) {
			pretscales[curi] = DpiVals[i];
		}
	}

	
	if (scaleinfo) {
		free(scaleinfo);
	}
	scaleinfo = NULL;
	if (needscales) {
		if (*ppavails && *ppavails != pretscales) {
			free(*ppavails);
		}
		*ppavails = pretscales;
		*psize = retsize;
	}


	return retlen;
fail:
	if (needscales) {
		if (pretscales && pretscales != *ppavails) {
			free(pretscales);
		}
	}
	pretscales = NULL;
	retsize = 0;

	if (scaleinfo) {
		free(scaleinfo);
	}
	scaleinfo = NULL;
	SETERRNO(ret);
	return ret;
}

int set_display_rescale(pdisplay_info_t pinfo, uint32_t scale)
{
	uint32_t getscale = 0;
	int curidx=-1,setidx=-1;
	int i;
	int ret;
	int relativeval = -1;
	LONG lret;
	uint32_t *availscales=NULL;
	int availsize=0,availlen=0;
	DISPLAYCONFIG_SOURCE_DPI_SCALE_SET* psetinfo=NULL;
	ret = get_display_rescale(pinfo,&getscale,&availscales,&availsize);
	if (ret < 0) {
		GETERRNO(ret);
		SETERRNO(ret);
		return ret;
	}
	availlen = ret;

	/*now check get value*/
	for(i=0;i<availlen;i++) {
		if (getscale == availscales[i]) {
			curidx = i;
		}
		if (scale == availscales[i]) {
			setidx = i;
		}
	}

	if (setidx < 0 || curidx < 0) {
		ret = -ERROR_INVALID_PARAMETER;
		ERROR_INFO("getscale [%d] or scale [%d] not valid",getscale,scale);
		goto fail;
	}

	if (setidx == curidx) {
		goto succ;
	}

	psetinfo = (DISPLAYCONFIG_SOURCE_DPI_SCALE_SET*)malloc(sizeof(*psetinfo));
	if (psetinfo == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	relativeval = setidx - curidx;
	memset(psetinfo,0,sizeof(*psetinfo));
	psetinfo->header.size = sizeof(*psetinfo);
	psetinfo->header.adapterId = pinfo->m_targetluid;
	psetinfo->header.id = pinfo->m_sourceid;
	psetinfo->header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE)DISPLAYCONFIG_DEVICE_INFO_SET_DPI_SCALE;
	psetinfo->scaleRel = (int32_t) relativeval;

	lret = DisplayConfigSetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)psetinfo);
	if (lret != ERROR_SUCCESS) {
		GETERRNO(ret);
		ERROR_INFO("DisplayConfigSetDeviceInfo scale [%d] error[%ld]", scale,lret);
		goto fail;
	}

succ:
	get_display_rescale(NULL,NULL,&availscales,&availsize);
	if (psetinfo) {
		free(psetinfo);
	}
	psetinfo = NULL;


	return 0;
fail:
	get_display_rescale(NULL,NULL,&availscales,&availsize);
	if (psetinfo) {
		free(psetinfo);
	}
	psetinfo = NULL;
	SETERRNO(ret);
	return ret;
}

#pragma warning(pop)