#ifndef __WIN_NET_INTER_H__
#define __WIN_NET_INTER_H__

#include <win_types.h>

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#define ADAPTER_NAME_SIZE  256
#define ADAPTER_IP_SIZE    64

#define ETHER_NET           1
#define IP4_NET             2
#define IP6_NET             4

typedef struct _net_inter_info {
	char m_adaptername[ADAPTER_NAME_SIZE];
	char m_adapternickname[ADAPTER_NAME_SIZE];
	char m_adapterip4[ADAPTER_IP_SIZE];
	char m_adapterip6[ADAPTER_IP_SIZE];
	char m_adaptermask4[ADAPTER_IP_SIZE];
	char m_adaptermask6[ADAPTER_IP_SIZE];
	char m_adaptergw[ADAPTER_IP_SIZE];
	char m_adapterdns[ADAPTER_IP_SIZE];
	char m_adaptermac[ADAPTER_IP_SIZE];
	int  m_mtu;
	uint32_t m_type;
}net_inter_info_t ,*pnet_inter_info_t;

#ifdef __cplusplus
extern "C" {
#endif

WINLIB_API int get_all_adapter_info(int freed,char* pfilter,pnet_inter_info_t* ppinfos,int *pinfonum);
WINLIB_API int set_adapter_info(pnet_inter_info_t pinfo);
WINLIB_API int get_adapter_servicenames(int freed, char*** pppsvcnames,int *psize);
WINLIB_API int get_arp_request(const char* srcip,const char* destip,void** ppmacaddr);
WINLIB_API int get_domain_ipaddr(const char* pdomain,char*** pppips,int *psize);

#ifdef __cplusplus
};
#endif


#endif /*__WIN_NET_INTER_H__*/