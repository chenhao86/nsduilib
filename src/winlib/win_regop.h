#ifndef __WIN_REGOP_H_3846E88D0AF44B2AABE7076223742390__
#define __WIN_REGOP_H_3846E88D0AF44B2AABE7076223742390__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <win_types.h>

#define ACCESS_KEY_READ       0x1
#define ACCESS_KEY_WRITE      0x2
#define ACCESS_KEY_ALL        0x4

#define HKLM_KEY              "HKEY_LOCAL_MACHINE"
#define HKCLASS_KEY           "HKEY_CLASSES_ROOT"
#define HKCFG_KEY             "HKEY_CURRENT_CONFIG"
#define HKCU_KEY              "HKEY_CURRENT_USER"
#define HKUSERS_KEY           "HKEY_USERS"
#define HKPERM_DATA_KEY       "HKEY_PERFORMANCE_DATA"
#define HKPERM_NL_KEY         "HKEY_PERFORMANCE_NLSTEXT"


WINLIB_API void* open_hklm(const char* psubkey,int accessmode);
WINLIB_API int query_hklm_string(void* pregop,const char* path,char** ppretval,int *pretsize);
WINLIB_API int query_hklm_binary(void* pregop,const char* path,void** ppdata,int *pdatasize);
WINLIB_API int query_hklm_dword(void* pregop,const char* path,uint32_t* pvalue);
WINLIB_API int set_hklm_binary(void* pregop, const char* path, void* pdata, int size);
WINLIB_API int set_hklm_string(void* pregop, const char* path, char* valstr);
WINLIB_API int set_hklm_sz(void* pregop1, const char* path, char* valstr);
WINLIB_API int set_hklm_dword(void* pregop1, const char* path, uint32_t value);
WINLIB_API int enum_hklm_keys(void* pregop1, char*** pppitems, int* psize);
WINLIB_API int enum_hklm_values(void* pregop1, char*** pppitems, int* psize);
WINLIB_API int delete_hklm_value(void* pregop1, const char* path);
WINLIB_API int delete_reg_value(void* pregop1, const char* path);
WINLIB_API void close_hklm(void** ppregop);
WINLIB_API void* open_reg_key(const char* pkeyname,const char* psubkey, int accessmode);
WINLIB_API int delete_reg_key(void* pregop1, const char* psubkey);
WINLIB_API void* create_reg_key(const char* pkeyname,const char* psubkey,int accessmode);
WINLIB_API void close_reg_key(void** ppregop);
WINLIB_API int set_reg_sz(void* pregop1, const char* path, char* valstr);
WINLIB_API int exist_reg_key(const char* pkeyname,const char* psubkey);
WINLIB_API int save_hive(char* file,char* keyname,char* subkey);
WINLIB_API int load_hive(char* file,char* keyname, char* subkey);
WINLIB_API int unload_hive(char* keyname, char* subkey);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_REGOP_H_3846E88D0AF44B2AABE7076223742390__ */
