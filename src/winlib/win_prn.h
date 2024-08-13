#ifndef __WIN_PRN_H_1CDAFB12887C49A069DC126837570DDD__
#define __WIN_PRN_H_1CDAFB12887C49A069DC126837570DDD__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <win_types.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

typedef struct __printer_list {
	char m_name[256];
	char m_type[32];
	char m_ip[256];
	char m_sharename[256];
} printer_list_t,*pprinter_list_t;

WINLIB_API int get_printer_list(int freed,HANDLE hexitevt,pprinter_list_t* ppret, int *psize);
WINLIB_API int add_share_printer(HANDLE hexitevt,char* name,char* remoteip,char* user,char* password);
WINLIB_API int del_share_printer(HANDLE hexitevt,char* name,char* remoteip);
WINLIB_API int save_printer_exportfile(HANDLE hexitevt,char* exportfile);
WINLIB_API int restore_printer_exportfile(HANDLE hexitevt,char* exportfile);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_PRN_H_1CDAFB12887C49A069DC126837570DDD__ */
