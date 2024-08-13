#include <win_usb.h>
#include <win_err.h>
#include <win_uniansi.h>
#include <win_hwinfo.h>

#pragma warning(push)
#pragma warning(disable:4820)
#pragma warning(disable:4514)
#include <cfgmgr32.h>
#pragma warning(disable:4668)
#include <setupapi.h>
#pragma warning(pop)

#include <initguid.h>

#pragma warning(push)
#if defined(_MSC_VER)
#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif
#endif

// This is the GUID for the USB device class
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
            0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
// ({A5DCBF10-6530-11D2-901F-00C04FB951ED})


// This is the GUID for the USB hub class
DEFINE_GUID(GUID_DEVINTERFACE_USB_HUB,
            0xF18A0E88L, 0xC30C, 0x11D0, 0x88, 0x15, 0x00, 0xA0, 0xC9, 0x06, 0xBE, 0xD8);
// ({F18A0E88-C30C-11D0-8815-00A0C906BED8})

// This is the GUID for the USB controller class
DEFINE_GUID(GUID_DEVINTERFACE_USB_HOST_CONTROLLER,
            0x3ABF6F2DL, 0x71C4, 0x462a, 0x8a, 0x92, 0x1e, 0x68, 0x61, 0xe6, 0xaf, 0x27);
// ({3ABF6F2D-71C4-462A-8A92-1E6861E6AF27})


#define  DESCRIPTION_PROP_GUID  "{a45c254e-df1c-4efd-8020-67d146a850e0}"
#define  DESCRIPTION_PROP_IDX   2

#define  HARDWAREID_PROP_GUID   "{a45c254e-df1c-4efd-8020-67d146a850e0}"
#define  HARDWAREID_PROP_IDX    3


#pragma comment(lib,"Cfgmgr32.lib")
#pragma comment(lib,"SetupAPI.lib")

#define PARSE_VALUES(valnum,descr)                                                                \
do{                                                                                               \
	while(*cpptr != 0 && *cpptr != '&') {                                                         \
		valnum <<= 4;                                                                             \
		if (*cpptr >= '0' && *cpptr <= '9') {                                                     \
			valnum += *cpptr - '0';                                                               \
		} else if (*cpptr >= 'a' && *cpptr <= 'f') {                                              \
			valnum += *cpptr - 'a' + 10;                                                          \
		} else if (*cpptr >= 'A' && *cpptr <= 'F') {                                              \
			valnum += *cpptr - 'A' + 10;                                                          \
		} else {                                                                                  \
			ret = -ERROR_INVALID_PARAMETER;                                                       \
			ERROR_INFO("[%s] not valid for %s", propansi,descr);                                  \
			goto fail;                                                                            \
		}                                                                                         \
		cpptr ++;                                                                                 \
	}                                                                                             \
}while(0)

int _fill_usb_root(pusb_dev_t pcurdev, phw_info_t pinfo)
{
	uint8_t* propbuf = NULL;
	int propsize = 0;
	int proplen = 0;
	wchar_t* pwptr = NULL;
	int ret;
	char* propansi = NULL;
	int ansisize = 0;
	int ansilen = 0;
	char* cpptr = NULL;
	int setvid = -1;
	int setpid = -1;
	pusb_root_t proot = &(pcurdev->u.m_root);
	pcurdev->m_type = USB_ROOT_DEV;

	ret = get_hw_prop(pinfo, DESCRIPTION_PROP_GUID, DESCRIPTION_PROP_IDX, &propbuf, &propsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	proplen = ret;

	pwptr = (wchar_t*) propbuf;
	ret = UnicodeToAnsi(pwptr, &propansi, &ansisize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ansilen = ret;
	if (ansilen >= sizeof(proot->m_description)) {
		ansilen = sizeof(proot->m_description) - 1;
	}
	memcpy(proot->m_description, propansi, (size_t)ansilen);
	proot->m_description[ansilen] = 0x0;

	ret = get_hw_prop(pinfo, HARDWAREID_PROP_GUID, HARDWAREID_PROP_IDX, &propbuf, &propsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	proplen = ret;
	pwptr = (wchar_t*) propbuf;
	ret = UnicodeToAnsi(pwptr, &propansi, &ansisize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ansilen = ret;

	cpptr = propansi;
	setvid = -1;
	setpid = -1;
	while (*cpptr != 0x0) {
		if (setvid < 0) {
			if (_strnicmp(cpptr, "ven_", 4) == 0) {
				cpptr += 4;
				setvid = 0;
				PARSE_VALUES(setvid, "setvid");
			}
		}

		if (setpid < 0) {
			if (_strnicmp(cpptr, "dev_", 4) == 0) {
				cpptr += 4;
				setpid = 0;
				PARSE_VALUES(setpid, "setpid");
			}
		}

		if (setvid >= 0 && setpid >= 0) {
			break;
		}
		cpptr ++;
	}

	if (setvid < 0 || setpid < 0) {
		ret = -ERROR_INVALID_PARAMETER;
		ERROR_INFO("[%s] not valid for HARDWAREID_PROP_GUID", propansi);
		goto fail;
	}

	proot->m_vendorid = (uint32_t)setvid;
	proot->m_prodid = (uint32_t)setpid;

	if (ansilen >= sizeof(proot->m_path)) {
		ansilen = sizeof(proot->m_path) - 1;
	}
	memcpy(proot->m_path, propansi, (size_t)ansilen);
	proot->m_path[ansilen] = 0x0;

	UnicodeToAnsi(NULL, &propansi, &ansisize);
	get_hw_prop(NULL, NULL, 0, &propbuf, &propsize);


	return 0;
fail:
	UnicodeToAnsi(NULL, &propansi, &ansisize);
	get_hw_prop(NULL, NULL, 0, &propbuf, &propsize);
	SETERRNO(ret);
	return ret;
}

int _fill_usb_hub(pusb_dev_t pcurdev, phw_info_t pinfo)
{
	uint8_t* propbuf = NULL;
	int propsize = 0;
	int proplen = 0;
	wchar_t* pwptr = NULL;
	int ret;
	char* propansi = NULL;
	int ansisize = 0;
	int ansilen = 0;
	char* cpptr = NULL;
	int setvid = -1;
	int setpid = -1;
	pusb_hub_t phub = &(pcurdev->u.m_hub);
	pcurdev->m_type = USB_HUB_DEV;

	ret = get_hw_prop(pinfo, DESCRIPTION_PROP_GUID, DESCRIPTION_PROP_IDX, &propbuf, &propsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	proplen = ret;

	pwptr = (wchar_t*) propbuf;
	ret = UnicodeToAnsi(pwptr, &propansi, &ansisize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ansilen = ret;
	if (ansilen >= sizeof(phub->m_description)) {
		ansilen = sizeof(phub->m_description) - 1;
	}
	memcpy(phub->m_description, propansi, (size_t)ansilen);
	phub->m_description[ansilen] = 0x0;

	ret = get_hw_prop(pinfo, HARDWAREID_PROP_GUID, HARDWAREID_PROP_IDX, &propbuf, &propsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	proplen = ret;
	pwptr = (wchar_t*) propbuf;
	ret = UnicodeToAnsi(pwptr, &propansi, &ansisize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ansilen = ret;

	cpptr = propansi;
	setvid = -1;
	setpid = -1;
	while (*cpptr != 0x0) {
		if (setvid < 0) {
			if (_strnicmp(cpptr, "vid_", 4) == 0) {
				cpptr += 4;
				setvid = 0;
				PARSE_VALUES(setvid, "setvid");
			}
		}

		if (setpid < 0) {
			if (_strnicmp(cpptr, "pid_", 4) == 0) {
				cpptr += 4;
				setpid = 0;
				PARSE_VALUES(setpid, "setpid");
			}
		}

		if (setvid >= 0 && setpid >= 0) {
			break;
		}
		cpptr ++;
	}

	if (setvid < 0 || setpid < 0) {
		cpptr = propansi;
		setvid = -1;
		setpid = -1;
		while (*cpptr != 0x0) {
			if (setvid < 0) {
				if (_strnicmp(cpptr, "vid", 3) == 0) {
					cpptr += 3;
					setvid = 0;
					PARSE_VALUES(setvid, "setvid");
				}
			}

			if (setpid < 0) {
				if (_strnicmp(cpptr, "pid", 3) == 0) {
					cpptr += 3;
					setpid = 0;
					PARSE_VALUES(setpid, "setpid");
				}
			}

			if (setvid >= 0 && setpid >= 0) {
				break;
			}
			cpptr ++;
		}
	}

	if (setvid < 0 || setpid < 0) {
		ret = -ERROR_INVALID_PARAMETER;
		ERROR_INFO("[%s] not valid for HARDWAREID_PROP_GUID", propansi);
		goto fail;
	}

	phub->m_vid = (uint32_t)setvid;
	phub->m_pid = (uint32_t)setpid;

	if (ansilen >= sizeof(phub->m_path)) {
		ansilen = sizeof(phub->m_path) - 1;
	}
	memcpy(phub->m_path, propansi, (size_t)ansilen);
	phub->m_path[ansilen] = 0x0;

	UnicodeToAnsi(NULL, &propansi, &ansisize);
	get_hw_prop(NULL, NULL, 0, &propbuf, &propsize);


	return 0;
fail:
	UnicodeToAnsi(NULL, &propansi, &ansisize);
	get_hw_prop(NULL, NULL, 0, &propbuf, &propsize);
	SETERRNO(ret);
	return ret;
}

int _fill_usb_device(pusb_dev_t pcurdev, phw_info_t pinfo)
{
	uint8_t* propbuf = NULL;
	int propsize = 0;
	int proplen = 0;
	wchar_t* pwptr = NULL;
	int ret;
	char* propansi = NULL;
	int ansisize = 0;
	int ansilen = 0;
	char* cpptr = NULL;
	int setvid = -1;
	int setpid = -1;
	pusb_device_t pdev = &(pcurdev->u.m_basedev);
	pcurdev->m_type = USB_BASE_DEV;

	ret = get_hw_prop(pinfo, DESCRIPTION_PROP_GUID, DESCRIPTION_PROP_IDX, &propbuf, &propsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	proplen = ret;

	pwptr = (wchar_t*) propbuf;
	ret = UnicodeToAnsi(pwptr, &propansi, &ansisize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ansilen = ret;
	if (ansilen >= sizeof(pdev->m_description)) {
		ansilen = sizeof(pdev->m_description) - 1;
	}
	memcpy(pdev->m_description, propansi, (size_t)ansilen);
	pdev->m_description[ansilen] = 0x0;

	ret = get_hw_prop(pinfo, HARDWAREID_PROP_GUID, HARDWAREID_PROP_IDX, &propbuf, &propsize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	proplen = ret;
	pwptr = (wchar_t*) propbuf;
	ret = UnicodeToAnsi(pwptr, &propansi, &ansisize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ansilen = ret;

	cpptr = propansi;
	setvid = -1;
	setpid = -1;
	while (*cpptr != 0x0) {
		if (setvid < 0) {
			if (_strnicmp(cpptr, "vid_", 4) == 0) {
				cpptr += 4;
				setvid = 0;
				PARSE_VALUES(setvid, "setvid");
			}
		}

		if (setpid < 0) {
			if (_strnicmp(cpptr, "pid_", 4) == 0) {
				cpptr += 4;
				setpid = 0;
				PARSE_VALUES(setpid, "setpid");
			}
		}

		if (setvid >= 0 && setpid >= 0) {
			break;
		}
		cpptr ++;
	}

	if (setvid < 0 || setpid < 0) {
		ret = -ERROR_INVALID_PARAMETER;
		ERROR_INFO("[%s] not valid for HARDWAREID_PROP_GUID", propansi);
		goto fail;
	}

	pdev->m_vid = (uint32_t)setvid;
	pdev->m_pid = (uint32_t)setpid;

	if (ansilen >= sizeof(pdev->m_path)) {
		ansilen = sizeof(pdev->m_path) - 1;
	}
	memcpy(pdev->m_path, propansi, (size_t)ansilen);
	pdev->m_path[ansilen] = 0x0;

	UnicodeToAnsi(NULL, &propansi, &ansisize);
	get_hw_prop(NULL, NULL, 0, &propbuf, &propsize);


	return 0;
fail:
	UnicodeToAnsi(NULL, &propansi, &ansisize);
	get_hw_prop(NULL, NULL, 0, &propbuf, &propsize);
	SETERRNO(ret);
	return ret;
}


#define  EXPAND_USB_DEV()                                                                         \
do{                                                                                               \
	if (retlen >= retsize) {                                                                      \
		if (retsize == 0) {                                                                       \
			retsize = 4;                                                                          \
		} else {                                                                                  \
			retsize <<= 1;                                                                        \
		}                                                                                         \
		ASSERT_IF(ptmp == NULL);                                                                  \
		ptmp = (pusb_dev_t) malloc(sizeof(*ptmp) * retsize);                                      \
		if (ptmp == NULL) {                                                                       \
			GETERRNO(ret);                                                                        \
			goto fail;                                                                            \
		}                                                                                         \
		memset(ptmp, 0 ,sizeof(*ptmp) * retsize);                                                 \
		if (retlen > 0) {                                                                         \
			memcpy(ptmp, pretdev, sizeof(*ptmp) * retlen);                                        \
		}                                                                                         \
		if (pretdev && pretdev != *ppur) {                                                        \
			free(pretdev);                                                                        \
		}                                                                                         \
		pretdev = ptmp;                                                                           \
		ptmp = NULL;                                                                              \
	}                                                                                             \
}while(0)


int list_usb_devices(int freed, pusb_dev_t* ppur, int *psize)
{
	int retlen = 0;
	int ret;
	phw_info_t* ppinfos = NULL;
	phw_info_t pcurinfo = NULL;
	int infosize = 0;
	int infolen = 0;
	int i;
	pusb_dev_t  pretdev = NULL;
	int retsize = 0;
	pusb_dev_t pcurdev;
	pusb_dev_t ptmp = NULL;
	if (freed) {
		if (ppur && *ppur) {
			free(*ppur);
			*ppur = NULL;
		}
		if (psize) {
			*psize = 0;
		}
		return 0;
	}

	if (ppur == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	pretdev = *ppur;
	retsize = *psize;
	retlen = 0;

	ret = get_hw_infos((LPGUID)&GUID_DEVINTERFACE_USB_HOST_CONTROLLER, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT, &ppinfos, &infosize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	infolen = ret;
	for (i = 0; i < infolen; i++) {
		pcurinfo = ppinfos[i];
		EXPAND_USB_DEV();
		pcurdev = &(pretdev[retlen]);
		ret = _fill_usb_root(pcurdev, pcurinfo);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		retlen ++;
	}


	ret = get_hw_infos((LPGUID)&GUID_DEVINTERFACE_USB_HUB, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT, &ppinfos, &infosize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	infolen = ret;
	for (i = 0; i < infolen; i++) {
		pcurinfo = ppinfos[i];
		EXPAND_USB_DEV();
		pcurdev = &(pretdev[retlen]);
		ret = _fill_usb_hub(pcurdev, pcurinfo);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		retlen ++;
	}

	ret = get_hw_infos((LPGUID)&GUID_DEVINTERFACE_USB_DEVICE, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT, &ppinfos, &infosize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	infolen = ret;
	for (i = 0; i < infolen; i++) {
		pcurinfo = ppinfos[i];
		EXPAND_USB_DEV();
		pcurdev = &(pretdev[retlen]);
		ret = _fill_usb_device(pcurdev, pcurinfo);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		retlen ++;
	}



	get_hw_infos(NULL, 0, &ppinfos, &infosize);
	infolen = 0;

	if (*ppur && *ppur != pretdev) {
		free(*ppur);
	}
	*ppur = pretdev;
	*psize = retsize;

	return retlen;
fail:
	if (pretdev && pretdev != *ppur) {
		free(pretdev);
	}
	pretdev = NULL;
	get_hw_infos(NULL, 0, &ppinfos, &infosize);
	infolen = 0;
	SETERRNO(ret);
	return ret;
}
#pragma warning(pop)