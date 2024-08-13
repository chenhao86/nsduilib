#ifndef __WIN_HDL_H_964187895EDF422DFFD4F0F41367C657__
#define __WIN_HDL_H_964187895EDF422DFFD4F0F41367C657__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <win_types.h>


typedef struct __handle_info {
	int m_pid;
	int m_reserv1;
	HANDLE m_hdl;
	char m_typename[32];
	char m_name[256];
} handle_info_t,*phandle_info_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WINLIB_API int get_handle_infos(int freed, phandle_info_t* pphdls,int *psize);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_HDL_H_964187895EDF422DFFD4F0F41367C657__ */
