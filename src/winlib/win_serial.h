#ifndef __WIN_SERIAL_H_744FE42CD4A1F558BA43170D0A3657EC__
#define __WIN_SERIAL_H_744FE42CD4A1F558BA43170D0A3657EC__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#define  SERIAL_SET_SPEED                 1   /*void* is int* */
#define  SERIAL_FBINARY_VALUE             2   /*void* for int for value 1 bit*/
#define  SERIAL_FPARITY_VALUE             3   /*void* for int for value 1 bit*/
#define  SERIAL_OUTCTXFLOW_VALUE          4   /*void* for int for value 1 bit*/
#define  SERIAL_OUTDSRFLOW_VALUE          5   /*void* for int for value 1 bit*/
#define  SERIAL_DTRCTRL_VALUE             6   /*void* for int for value 2 bit*/
#define  SERIAL_DSRSENSITY_VALUE          7   /*void* for int for value 1 bit*/
#define  SERIAL_TXCONONXOFF_VALUE         8   /*void* for int for value 1 bit*/
#define  SERIAL_OUTX_VALUE                9   /*void* for int for value 1 bit*/
#define  SERIAL_INX_VALUE                 10  /*void* for int for value 1 bit*/
#define  SERIAL_FERRORCHAR_VALUE          11  /*void* for int for value 1 bit*/
#define  SERIAL_NULL_VALUE                12  /*void* for int for value 1 bit*/
#define  SERIAL_RTSCTRL_VALUE             13  /*void* for int for value 2 bit*/
#define  SERIAL_ABORTONERROR_VALUE        14  /*void* for int for value 1 bit*/
#define  SERIAL_DUMMY2_VALUE              15  /*void* for int for value 17 bit*/
#define  SERIAL_RESERVED_VALUE            16  /*void* for word*/
#define  SERIAL_XONLIMIT_VALUE            17  /*void* for word*/
#define  SERIAL_XOFFLIMIT_VALUE           18  /*void* for word*/
#define  SERIAL_BYTESIZE_VALUE            19  /*void* for byte*/
#define  SERIAL_PARITY_VALUE              20  /*void* for byte*/
#define  SERIAL_STOPBITS_VALUE            21  /*void* for byte*/
#define  SERIAL_XONCHAR_VALUE             22  /*void* for byte*/
#define  SERIAL_XOFFCHAR_VALUE            23  /*void* for byte*/
#define  SERIAL_ERRORCHAR_VALUE           24  /*void* for byte*/
#define  SERIAL_EOFCHAR_VALUE             25  /*void* for byte*/
#define  SERIAL_EVTCHAR_VALUE             26  /*void* for byte*/
#define  SERIAL_RESERVED1_VALUE           27  /*void* for word*/
#define  SERIAL_SET_RAW                   28  /*void* not use*/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WINLIB_API void* open_serial(const char* name);
WINLIB_API void close_serial(void** ppcom);
WINLIB_API int prepare_config_serial(void* pcom1, int flag, void* pval);
WINLIB_API int commit_config_serial(void* pcom1);
WINLIB_API int read_serial(void* pcom1, void* pbuf, int bufsize);
WINLIB_API int write_serial(void* pcom1, void* pbuf,int bufsize);
WINLIB_API HANDLE get_serial_read_handle(void* pcom1);
WINLIB_API HANDLE get_serial_write_handle(void* pcom1);
WINLIB_API int complete_serial_read(void* pcom1);
WINLIB_API int complete_serial_write(void* pcom1);
WINLIB_API int get_serial_config_direct(void* pcom1,void** ppbuf,int* psize);


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_SERIAL_H_744FE42CD4A1F558BA43170D0A3657EC__ */
