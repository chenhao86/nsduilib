#ifndef __WIN_DBG_H_CB2C50B5564C62D409571AABA1425EC8__
#define __WIN_DBG_H_CB2C50B5564C62D409571AABA1425EC8__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#define WIN_DBG_FLAGS_CHILDREN                   0x1
#define WIN_DBG_FLAGS_HEAP                       0x2

#define WIN_DBG_FLAGS_FREE                       0x0
#define WIN_DBG_OUTPUT_OUT                       0x1

#define SYMINFO_NULL             0x1
#define SYMINFO_EXE              0x2
#define SYMINFO_COMPILAND        0x4
#define SYMINFO_COMPILANDDETAILS 0x8
#define SYMINFO_COMPILANDENV     0x10
#define SYMINFO_FUNCTION         0x20
#define SYMINFO_BLOCK            0x40
#define SYMINFO_DATA             0x80
#define SYMINFO_ANNOTATION       0x100
#define SYMINFO_LABEL            0x200
#define SYMINFO_PUBLICSYMBOL     0x400
#define SYMINFO_UDT              0x800
#define SYMINFO_ENUM             0x1000
#define SYMINFO_FUNCTIONTYPE     0x2000
#define SYMINFO_POINTERTYPE      0x4000
#define SYMINFO_ARRAYTYPE        0x8000
#define SYMINFO_BASETYPE         0x10000
#define SYMINFO_TYPEDEF          0x20000
#define SYMINFO_BASECLASS        0x40000
#define SYMINFO_FRIEND           0x80000
#define SYMINFO_FUNCTIONARGTYPE  0x100000
#define SYMINFO_FUNCDEBUGSTART   0x200000
#define SYMINFO_FUNCDEBUGEND     0x400000
#define SYMINFO_USINGNAMESPACE   0x800000
#define SYMINFO_VTABLESHAPE      0x1000000
#define SYMINFO_VTABLE           0x2000000
#define SYMINFO_CUSTOM           0x4000000
#define SYMINFO_THUNK            0x8000000
#define SYMINFO_CUSTOMTYPE       0x10000000
#define SYMINFO_MANAGEDTYPE      0x20000000
#define SYMINFO_DIMENSION        0x40000000
#define SYMINFO_UNKNOWN          0x80000000


typedef struct __sym_info {
	uint64_t m_idx;
	uint64_t m_type;
	uint64_t m_address;
	uint8_t m_name[256];
}sym_info_t,*psym_info_t;

typedef struct __debug_symbol_info{
	int m_size;
	int m_num;
	int m_needsize;
	int m_err;
	sym_info_t m_syminfo[1];
}debug_symbol_info_t,*pdebug_symbol_info_t;


#ifdef  _M_X64

WINLIB_API int windbg_create_client(char* option, void** ppclient);
WINLIB_API int windbg_start_process_single(void* pclient, char* cmd, int flags);
WINLIB_API int windbg_stop_process(void* pclient);
WINLIB_API int windbg_go(void* pclient);
WINLIB_API int windbg_exec(void* pclient, const char* cmd);
WINLIB_API int windbg_get_out(void* pclient,int flags, char** ppout, int *psize);
WINLIB_API int windbg_interrupt(void* pclient);
WINLIB_API int enum_symbol_pdb(const char* pdbfile,const char* searchmask,addr_t loadaddr, pdebug_symbol_info_t psyminfo,int maxsize,uint64_t* pretval);

#endif /*  _M_X64*/


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_DBG_H_CB2C50B5564C62D409571AABA1425EC8__ */
