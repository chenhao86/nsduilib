#ifndef __WIN_HWINFO_H_F3D56C34D985D7525428A4E503638064__
#define __WIN_HWINFO_H_F3D56C34D985D7525428A4E503638064__


#include <win_types.h>


#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


typedef struct __hw_prop {
	char* m_propguid;
	uint8_t* m_propbuf;
	ULONG m_propbuflen;
	ULONG m_propbufsize;
	int m_propguidsize;
	int m_propguididx;
} hw_prop_t,*phw_prop_t;

typedef struct __hw_info {
	phw_prop_t* m_proparr;
	int m_propsize;
	int m_proplen;
} hw_info_t,*phw_info_t;

typedef struct __hw_meminfo {
	char m_sn[64];
	char m_partnumber[64];
	char m_manufacturer[64];
	uint64_t m_size;
	uint32_t m_speed;
	uint32_t m_reserv1;
} hw_meminfo_t, *phw_meminfo_t;

typedef struct __hw_cpuinfo {
	char m_name[256];
	uint32_t m_numthreads;
	uint32_t m_numcores;
	uint8_t m_processorid[64];
	uint32_t m_idlen;
	uint32_t m_reserv1;
} hw_cpuinfo_t, *phw_cpuinfo_t;

typedef struct __hw_audioinfo {
	char m_name[256];
	char m_path[256];
	uint32_t m_vendorid;
	uint32_t m_prodid;
} hw_audioinfo_t,*phw_audioinfo_t;


#define  GUID_NULL_PTR ((LPGUID)0x1)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WINLIB_API int get_hw_infos(LPGUID pguid, DWORD flags,phw_info_t** pppinfos, int *psize);
WINLIB_API int get_hw_prop(phw_info_t pinfo, char* propguid, int propidx, uint8_t** ppbuf, int *psize);
WINLIB_API int get_guid_str(LPGUID pguid, char** ppstr, int *psize);
WINLIB_API int guid_from_str2(LPGUID pguid, char* pstr);
WINLIB_API int get_hw_mem_info(int freed,phw_meminfo_t* ppmems, int *psize);
WINLIB_API int get_hw_cpu_info(int freed,phw_cpuinfo_t* ppcpus, int *psize);
WINLIB_API int get_hw_audio_info(int freed,phw_audioinfo_t* ppaudios, int *psize);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_HWINFO_H_F3D56C34D985D7525428A4E503638064__ */
