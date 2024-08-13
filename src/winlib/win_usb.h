#ifndef __WIN_USB_H_60D69BAC37D7039065EAF9A0286A7FA5__
#define __WIN_USB_H_60D69BAC37D7039065EAF9A0286A7FA5__

#include <win_types.h>

#define  USB_ROOT_DEV          0x1
#define  USB_HUB_DEV           0x2
#define  USB_BASE_DEV          0x3

typedef struct __usb_device {
	uint32_t m_vid;
	uint32_t m_pid;
	uint8_t  m_path[256];
	char m_description[256];
} usb_device_t, *pusb_device_t;

typedef struct __usb_hub {
	uint32_t m_vid;
	uint32_t m_pid;
	uint8_t  m_path[256];
	char m_description[256];
} usb_hub_t,*pusb_hub_t;

typedef struct __usb_root {
	uint32_t m_vendorid;
	uint32_t m_prodid;
	uint8_t m_path[256];
	char m_description[256];
} usb_root_t, *pusb_root_t;

typedef struct __usb_dev {
	int m_type;
	union {
		usb_device_t m_basedev;
		usb_root_t m_root;
		usb_hub_t m_hub;
	} u;
} usb_dev_t ,*pusb_dev_t;

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


WINLIB_API int list_usb_devices(int freed, pusb_dev_t* ppur, int *psize);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_USB_H_60D69BAC37D7039065EAF9A0286A7FA5__ */
