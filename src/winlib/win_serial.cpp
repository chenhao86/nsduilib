#include <win_types.h>
#include <win_serial.h>
#include <win_output_debug.h>
#include <win_uniansi.h>

#pragma warning(push)

#if defined(_MSC_VER)
#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif
#endif


#define  SERIAL_DATA_MAGIC   0x710129d3

#define  FLUSH_BUFFER_NONE           0
#define  FLUSH_BUFFER_STARTING       1
#define  FLUSH_BUFFER_COPYING        2
#define  FLUSH_BUFFER_FINISHED       3

#define  FLUSH_BUFFER_SIZE           8192

#define  DISABLED_FLUSHED            1

typedef struct __win_serial_priv {
	uint32_t m_magic;
	uint32_t m_reserv1;
	char* m_name;
	HANDLE m_hfile;
	HANDLE m_hrdevt;
	HANDLE m_hwrevt;
	uint8_t* m_prdptr;
	int m_rdleft;
	int m_reserv2;
	uint8_t* m_pwrptr;
	int m_wrleft;
	int m_reserv3;
	OVERLAPPED m_rdov;
	OVERLAPPED m_wrov;
	int m_inrd;
	int m_inwr;
	uint64_t m_rdtotal;
	uint64_t m_wrtotal;
	DCB m_dcb;
	DCB m_cacheddcb;
	int m_cached;
	int m_flshstate;
	char* m_flshbuf;
	int m_flshsize;
	int m_flshlen;
	int m_flshrlen;
	int m_reserv4;
} win_serial_priv_t, *pwin_serial_priv_t;

void __free_serial(pwin_serial_priv_t* ppcom)
{
	BOOL bret;
	int ret;
	if (ppcom && *ppcom) {
		pwin_serial_priv_t pcom = *ppcom;
		if (pcom->m_magic != SERIAL_DATA_MAGIC) {
			ERROR_INFO("not magic com");
		}

		if (pcom->m_inrd > 0) {
			ASSERT_IF(pcom->m_hfile != NULL && pcom->m_hfile != INVALID_HANDLE_VALUE);
			bret = CancelIoEx(pcom->m_hfile, &(pcom->m_rdov));
			if (!bret) {
				GETERRNO(ret);
				ERROR_INFO("cancel [%s] read event error[%d]", pcom->m_name, ret);
			}
		}
		pcom->m_inrd = 0;
		memset(&(pcom->m_rdov), 0 , sizeof(pcom->m_rdov));

		if (pcom->m_inwr > 0) {
			ASSERT_IF(pcom->m_hfile != NULL && pcom->m_hfile != INVALID_HANDLE_VALUE);
			bret = CancelIoEx(pcom->m_hfile, &(pcom->m_wrov));
			if (!bret) {
				GETERRNO(ret);
				ERROR_INFO("cancel [%s] write event error[%d]", pcom->m_name, ret);
			}
		}
		pcom->m_inwr = 0;
		memset(&(pcom->m_wrov), 0, sizeof(pcom->m_wrov));

		if (pcom->m_hrdevt != NULL) {
			CloseHandle(pcom->m_hrdevt);
		}
		pcom->m_hrdevt = NULL;

		if (pcom->m_hwrevt != NULL) {
			CloseHandle(pcom->m_hwrevt);
		}
		pcom->m_hwrevt = NULL;

		if (pcom->m_hfile != NULL && pcom->m_hfile != INVALID_HANDLE_VALUE) {
			CloseHandle(pcom->m_hfile);
		}
		pcom->m_hfile = NULL;

		if (pcom->m_flshbuf != NULL) {
			free(pcom->m_flshbuf);
		}
		pcom->m_flshbuf = NULL;
		pcom->m_flshstate = FLUSH_BUFFER_NONE;
		pcom->m_flshsize = 0;
		pcom->m_flshlen = 0;
		pcom->m_flshrlen = 0;

		if (pcom->m_name != NULL) {
			free(pcom->m_name);
		}
		pcom->m_name = NULL;


		pcom->m_pwrptr = NULL;
		pcom->m_wrleft = 0;
		pcom->m_prdptr = NULL;
		pcom->m_rdleft = 0;

		pcom->m_rdtotal = 0;
		pcom->m_wrtotal = 0;

		memset(&(pcom->m_dcb), 0, sizeof(pcom->m_dcb));
		memset(&(pcom->m_cacheddcb), 0, sizeof(pcom->m_cacheddcb));
		pcom->m_cached = 0;

		free(pcom);
		*ppcom = NULL;
	}
}

void close_serial(void** ppcom)
{
	__free_serial((pwin_serial_priv_t*)ppcom);
}


void* open_serial(const char* name)
{
	pwin_serial_priv_t pcom = NULL;
	int ret;
	TCHAR* ptname = NULL;
	int tnamesize = 0;
	BOOL bret;

	if (name == NULL) {
		ret =  -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	pcom = (pwin_serial_priv_t)malloc(sizeof(*pcom));
	if (pcom == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	memset(pcom, 0, sizeof(*pcom));
	pcom->m_magic = SERIAL_DATA_MAGIC;
	pcom->m_name = NULL;
	pcom->m_hfile = NULL;
	pcom->m_hrdevt = NULL;
	pcom->m_hwrevt = NULL;

	pcom->m_prdptr = NULL;
	pcom->m_rdleft = 0;
	pcom->m_pwrptr = NULL;
	pcom->m_wrleft = 0;
	pcom->m_flshbuf = NULL;
	pcom->m_flshsize = FLUSH_BUFFER_SIZE;
	pcom->m_flshstate = FLUSH_BUFFER_NONE;
	pcom->m_flshlen = 0;
	pcom->m_flshrlen = 0;

	memset(&(pcom->m_rdov), 0, sizeof(pcom->m_rdov));
	memset(&(pcom->m_wrov), 0, sizeof(pcom->m_wrov));

	pcom->m_inrd = 0;
	pcom->m_inwr = 0;

	memset(&(pcom->m_dcb), 0, sizeof(pcom->m_dcb));
	memset(&(pcom->m_cacheddcb), 0, sizeof(pcom->m_cacheddcb));
	pcom->m_cached = 0;


	/*now to open file*/
	pcom->m_name = _strdup(name);
	if (pcom->m_name == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	ret = AnsiToTchar(name, &ptname, &tnamesize);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	pcom->m_hfile = CreateFile(ptname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (pcom->m_hfile == NULL || pcom->m_hfile == INVALID_HANDLE_VALUE) {
		pcom->m_hfile = NULL;
		GETERRNO(ret);
		ERROR_INFO("open [%s] error[%d]", pcom->m_name, ret);
		goto fail;
	}

	/*now to make event ok*/
	pcom->m_hrdevt = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (pcom->m_hrdevt == NULL) {
		GETERRNO(ret);
		ERROR_INFO("create [%s] rdevt error[%d]", pcom->m_name, ret);
		goto fail;
	}
	pcom->m_rdov.hEvent = pcom->m_hrdevt;

	pcom->m_hwrevt = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (pcom->m_hwrevt == NULL) {
		GETERRNO(ret);
		ERROR_INFO("create [%s] wrevt error[%d]", pcom->m_name, ret);
		goto fail;
	}
	pcom->m_wrov.hEvent = pcom->m_hwrevt;

	memset(&(pcom->m_dcb), 0, sizeof(pcom->m_dcb));
	pcom->m_dcb.DCBlength = sizeof(pcom->m_dcb.DCBlength);


	bret = GetCommState(pcom->m_hfile, &(pcom->m_dcb));
	if (!bret) {
		GETERRNO(ret);
		ERROR_INFO("can not GetCommState [%s] error[%d]", pcom->m_name, ret);
		goto fail;
	}

	ASSERT_IF(pcom->m_flshbuf == NULL);
	pcom->m_flshbuf = (char*)malloc((size_t)pcom->m_flshsize);
	if (pcom->m_flshbuf == NULL) {
		GETERRNO(ret);
		goto fail;
	}


	DEBUG_INFO("open [%s] succ", pcom->m_name);
	AnsiToTchar(NULL, &ptname, &tnamesize);
	return pcom;
fail:
	AnsiToTchar(NULL, &ptname, &tnamesize);
	__free_serial(&pcom);
	SETERRNO(ret);
	return NULL;
}

#define SERIAL_SET_VALUE_MASK(member,mask)                                                        \
do{                                                                                               \
	pival = (int*) pval;                                                                          \
	ival = *pival;                                                                                \
	ival = ival & (mask);                                                                         \
	pcom->m_cacheddcb.member = (DWORD)ival;                                                       \
}while(0)

int prepare_config_serial(void* pcom1, int flag, void* pval)
{
	pwin_serial_priv_t pcom = (pwin_serial_priv_t)pcom1;
	int ret;

	int ival, *pival;
	if (pcom == NULL || pcom->m_magic != SERIAL_DATA_MAGIC || pcom->m_hfile == NULL || pval == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	if (pcom->m_cached == 0) {
		memcpy(&(pcom->m_cacheddcb), &(pcom->m_dcb), sizeof(pcom->m_dcb));
		pcom->m_cached = 1;
	}

	switch (flag) {
	case SERIAL_SET_SPEED:
		pival = (int*) pval;
		ival = *pival;
		switch (ival) {
		case 110:
			pcom->m_cacheddcb.BaudRate = CBR_110;
			break;
		case 300:
			pcom->m_cacheddcb.BaudRate = CBR_300;
			break;
		case 600:
			pcom->m_cacheddcb.BaudRate = CBR_600;
			break;
		case 1200:
			pcom->m_cacheddcb.BaudRate = CBR_1200;
			break;
		case 2400:
			pcom->m_cacheddcb.BaudRate = CBR_2400;
			break;
		case 4800:
			pcom->m_cacheddcb.BaudRate = CBR_4800;
			break;
		case 9600:
			pcom->m_cacheddcb.BaudRate = CBR_9600;
			break;
		case 14400:
			pcom->m_cacheddcb.BaudRate = CBR_14400;
			break;
		case 19200:
			pcom->m_cacheddcb.BaudRate = CBR_19200;
			break;
		case 38400:
			pcom->m_cacheddcb.BaudRate = CBR_38400;
			break;
		case 57600:
			pcom->m_cacheddcb.BaudRate = CBR_57600;
			break;
		case 115200:
			pcom->m_cacheddcb.BaudRate = CBR_115200;
			break;
		case 128000:
			pcom->m_cacheddcb.BaudRate = CBR_128000;
			break;
		case 256000:
			pcom->m_cacheddcb.BaudRate = CBR_256000;
			break;
		default:
			ret = -ERROR_INVALID_PARAMETER;
			goto fail;
		}
		break;
	case SERIAL_FBINARY_VALUE:
		SERIAL_SET_VALUE_MASK(fBinary, 0x1);
		break;
	case SERIAL_FPARITY_VALUE:
		SERIAL_SET_VALUE_MASK(fParity, 0x1);
		break;
	case SERIAL_OUTCTXFLOW_VALUE:
		SERIAL_SET_VALUE_MASK(fOutxCtsFlow, 0x1);
		break;
	case SERIAL_OUTDSRFLOW_VALUE:
		SERIAL_SET_VALUE_MASK(fOutxDsrFlow, 0x1);
		break;
	case SERIAL_DTRCTRL_VALUE:
		SERIAL_SET_VALUE_MASK(fDtrControl, 0x3);
		break;
	case SERIAL_DSRSENSITY_VALUE:
		SERIAL_SET_VALUE_MASK(fDsrSensitivity, 0x1);
		break;
	case SERIAL_TXCONONXOFF_VALUE:
		SERIAL_SET_VALUE_MASK(fTXContinueOnXoff, 0x1);
		break;
	case SERIAL_OUTX_VALUE:
		SERIAL_SET_VALUE_MASK(fOutX, 0x1);
		break;
	case SERIAL_INX_VALUE:
		SERIAL_SET_VALUE_MASK(fInX, 0x1);
		break;
	case SERIAL_FERRORCHAR_VALUE:
		SERIAL_SET_VALUE_MASK(fErrorChar, 0x1);
		break;
	case SERIAL_NULL_VALUE:
		SERIAL_SET_VALUE_MASK(fNull, 0x1);
		break;
	case SERIAL_RTSCTRL_VALUE:
		SERIAL_SET_VALUE_MASK(fRtsControl, 0x3);
		break;
	case SERIAL_ABORTONERROR_VALUE:
		SERIAL_SET_VALUE_MASK(fAbortOnError, 0x1);
		break;
	case SERIAL_DUMMY2_VALUE:
		SERIAL_SET_VALUE_MASK(fDummy2, ((1 << 17) - 1));
		break;
	case SERIAL_RESERVED_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.wReserved = (WORD) ival;
		break;
	case SERIAL_XONLIMIT_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.XonLim = (WORD) ival;
		break;
	case SERIAL_XOFFLIMIT_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.XoffLim = (WORD) ival;
		break;
	case SERIAL_BYTESIZE_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.ByteSize = (BYTE) ival;
		break;
	case SERIAL_PARITY_VALUE:
		pival = (int*)pval;
		ival = *pival;
		switch (ival) {
		case EVENPARITY:
		case MARKPARITY:
		case NOPARITY:
		case ODDPARITY:
		case SPACEPARITY:
			break;
		default:
			ret = -ERROR_INVALID_PARAMETER;
			goto fail;
		}
		pcom->m_cacheddcb.Parity = (BYTE) ival;
		break;
	case SERIAL_STOPBITS_VALUE:
		pival = (int*)pval;
		ival = *pival;
		switch (ival) {
		case ONESTOPBIT:
		case ONE5STOPBITS:
		case TWOSTOPBITS:
			break;
		default:
			ret = -ERROR_INVALID_PARAMETER;
			goto fail;
		}
		pcom->m_cacheddcb.StopBits = (BYTE) ival;
		break;
	case SERIAL_XONCHAR_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.XonChar = (char) ival;
		break;
	case SERIAL_XOFFCHAR_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.XoffChar = (char) ival;
		break;
	case SERIAL_ERRORCHAR_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.ErrorChar = (char) ival;
		break;
	case SERIAL_EOFCHAR_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.EofChar = (char) ival;
		break;
	case SERIAL_EVTCHAR_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.EvtChar = (char) ival;
		break;
	case SERIAL_RESERVED1_VALUE:
		pival = (int*)pval;
		ival = *pival;
		pcom->m_cacheddcb.wReserved1 = (WORD) ival;
		break;
	case SERIAL_SET_RAW:
		pcom->m_cacheddcb.fBinary = 1;
		pcom->m_cacheddcb.fParity = 0;
		pcom->m_cacheddcb.fOutxCtsFlow = 0;
		pcom->m_cacheddcb.fOutxDsrFlow = 0;
		pcom->m_cacheddcb.fDtrControl = DTR_CONTROL_DISABLE;
		pcom->m_cacheddcb.fDsrSensitivity = 0;
		pcom->m_cacheddcb.fTXContinueOnXoff = 1;
		pcom->m_cacheddcb.fOutX = 0;
		pcom->m_cacheddcb.fInX = 0;
		pcom->m_cacheddcb.fErrorChar = 0;
		pcom->m_cacheddcb.fNull = 0;
		pcom->m_cacheddcb.fRtsControl = RTS_CONTROL_DISABLE;
		pcom->m_cacheddcb.fAbortOnError = 0;
		pcom->m_cacheddcb.fDummy2 = 0;
		pcom->m_cacheddcb.wReserved = 0;
		pcom->m_cacheddcb.XonLim = 0x800;
		pcom->m_cacheddcb.XoffLim = 0x200;
		pcom->m_cacheddcb.ByteSize = 8;
		pcom->m_cacheddcb.Parity = NOPARITY;
		pcom->m_cacheddcb.StopBits = ONESTOPBIT;
		pcom->m_cacheddcb.XonChar = 0x11;
		pcom->m_cacheddcb.XoffChar = 0x13;
		pcom->m_cacheddcb.ErrorChar = 0x0;
		pcom->m_cacheddcb.EofChar = 0x0;
		pcom->m_cacheddcb.EvtChar = 0x0;
		pcom->m_cacheddcb.wReserved1 = 0x0;
		break;
	default:
		ret = -ERROR_NOT_SUPPORTED;
		goto fail;
	}
	return 0;
fail:
	SETERRNO(ret);
	return ret;
}

int commit_config_serial(void* pcom1)
{
	int commited = 0;
	pwin_serial_priv_t pcom = (pwin_serial_priv_t)pcom1;
	int ret;
	BOOL bret;
	if (pcom == NULL || pcom->m_magic != SERIAL_DATA_MAGIC || pcom->m_hfile == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	if (pcom->m_cached > 0) {
		bret = SetCommState(pcom->m_hfile, &(pcom->m_cacheddcb));
		if (!bret) {
			GETERRNO(ret);
			ERROR_INFO("can not set [%s] dcb error[%d]", pcom->m_name, ret);
			goto fail;
		}
		memcpy(&(pcom->m_dcb), &(pcom->m_cacheddcb), sizeof(pcom->m_dcb));
		pcom->m_cached = 0;
		commited = 1;
	}

	return commited ;
fail:
	SETERRNO(ret);
	return ret;
}

int __inner_read_serial(pwin_serial_priv_t pcom)
{
	int completed = 0;
	int ret;
	DWORD cbret;
	BOOL bret;

	ASSERT_IF(pcom->m_flshstate == FLUSH_BUFFER_FINISHED || DISABLED_FLUSHED > 0);
	while (pcom->m_rdleft > 0) {
		bret = ReadFile(pcom->m_hfile, pcom->m_prdptr, (DWORD) pcom->m_rdleft, &cbret, &(pcom->m_rdov));
		if (!bret) {
			GETERRNO(ret);
			if (ret == -ERROR_IO_PENDING ) {
				DEBUG_INFO("read pending on [%d] rdleft", pcom->m_rdleft);
				return 0;
			} else if (ret == -ERROR_MORE_DATA) {
				pcom->m_rdleft -= cbret;
				pcom->m_prdptr += cbret;
				pcom->m_rdtotal += cbret;
				if (pcom->m_rdleft == 0) {
					pcom->m_prdptr = NULL;
					completed = 1;
					pcom->m_inrd = 0;
				}
				return completed;
			}
			ERROR_INFO("can not read [%s] on [%d] error[%d]", pcom->m_name, pcom->m_rdleft, ret);
			SETERRNO(ret);
			return ret;
		}

		DEBUG_INFO("cbret [%d]", cbret);
		pcom->m_rdleft -= cbret;
		pcom->m_prdptr += cbret;
		pcom->m_rdtotal += cbret;
	}

	if (pcom->m_rdleft == 0) {
		completed = 1;
		pcom->m_prdptr = NULL;
		pcom->m_inrd = 0;
	}
	return completed;
}

int _read_flushing(pwin_serial_priv_t pcom)
{
	int completed = 0;
	DWORD cbret = 0;
	BOOL bret;
	int curlen = 0;
	int leftlen;
	int ret;
	if (pcom->m_flshstate == FLUSH_BUFFER_NONE) {
		pcom->m_flshstate = FLUSH_BUFFER_STARTING;
		while (1) {
			cbret = 0;
			bret = ReadFile(pcom->m_hfile, pcom->m_flshbuf, (DWORD)pcom->m_flshsize, &cbret, &(pcom->m_rdov));
			if (!bret) {
				GETERRNO(ret);
				if (ret == -ERROR_IO_PENDING ) {
					DEBUG_INFO("cbret %ld ret[%d]", cbret, ret);
					break;
				} else if (ret != -ERROR_MORE_DATA) {
					ERROR_INFO("flush buffer [%s] error[%d]" , pcom->m_name, ret);
					goto fail;
				}
			}
			curlen = (int)cbret;
			if (curlen > 0x200) {
				curlen = 0x200;
				DEBUG_BUFFER_FMT(pcom->m_flshbuf, curlen, "flush start [%d]", cbret);
				DEBUG_BUFFER_FMT(&(pcom->m_flshbuf[(cbret - curlen)]), curlen, "flush end [%d]", cbret);
			} else {
				DEBUG_BUFFER_FMT(pcom->m_flshbuf, curlen, "flush total");
			}
		}
	} else if (pcom->m_flshstate == FLUSH_BUFFER_STARTING ||
	           pcom->m_flshstate == FLUSH_BUFFER_COPYING) {
		if (pcom->m_flshstate == FLUSH_BUFFER_STARTING) {
		read_again:
			ASSERT_IF(pcom->m_flshlen < pcom->m_flshsize);
			curlen = pcom->m_flshsize - pcom->m_flshlen;
			if (curlen > 0) {
				cbret = 0;
				bret = ReadFile(pcom->m_hfile, &(pcom->m_flshbuf[pcom->m_flshlen]), (DWORD)curlen, &cbret, &(pcom->m_rdov));
				if (!bret) {
					GETERRNO(ret);
					if (ret != -ERROR_IO_PENDING && ret != -ERROR_MORE_DATA) {
						ERROR_INFO("flush buffer [%s] error[%d]" , pcom->m_name, ret);
						goto fail;
					}
				}
				DEBUG_INFO("cbret [%ld:0x%lx]",cbret,cbret);
				curlen = (int)cbret;
				if (curlen > 0x200) {
					curlen = 0x200;
					DEBUG_BUFFER_FMT(pcom->m_flshbuf, curlen, "flush start [%d]", cbret);
					DEBUG_BUFFER_FMT(&(pcom->m_flshbuf[(cbret - curlen)]), curlen, "flush end [%d]", cbret);
				} else {
					DEBUG_BUFFER_FMT(pcom->m_flshbuf, curlen, "flush total");
				}
				pcom->m_flshlen += cbret;
				if (pcom->m_flshrlen == pcom->m_flshsize) {
					pcom->m_flshstate = FLUSH_BUFFER_COPYING;
				}
				if (cbret > 0 && pcom->m_flshstate == FLUSH_BUFFER_STARTING){
					goto read_again;
				}
			}
		}

		leftlen = pcom->m_flshlen - pcom->m_flshrlen;
		if (leftlen > pcom->m_rdleft) {
			leftlen = pcom->m_rdleft;
		}
		if (leftlen > 0) {
			memcpy(pcom->m_prdptr, &(pcom->m_flshbuf[pcom->m_flshrlen]), (size_t)leftlen);
			pcom->m_flshrlen += leftlen;
			pcom->m_prdptr += leftlen;
			pcom->m_rdleft -= leftlen;
		}

		if (pcom->m_rdleft == 0) {
			completed = 1;
			pcom->m_prdptr = NULL;
		}

		if (pcom->m_flshstate == FLUSH_BUFFER_COPYING)  {
			ASSERT_IF(pcom->m_flshlen == pcom->m_flshsize);
			if (pcom->m_flshrlen == pcom->m_flshlen) {
				pcom->m_flshstate = FLUSH_BUFFER_FINISHED;
			}
		}

		if (pcom->m_flshstate == FLUSH_BUFFER_FINISHED) {
			/*we finished ,so we should give this ok*/
			if (pcom->m_rdleft > 0) {
				ret = __inner_read_serial(pcom);
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}
				completed = ret;
			} else {
				/*that is read all over*/
				ASSERT_IF(pcom->m_prdptr == NULL);
				pcom->m_inrd = 0;
			}
		}
	} else {
		ret = -ERROR_INTERNAL_ERROR;
		ERROR_INFO("inner state [%d]", pcom->m_flshstate);
		goto fail;
	}

	return completed;
fail:
	SETERRNO(ret);
	return ret;
}

int read_serial(void* pcom1, void* pbuf, int bufsize)
{
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	int completed = 0;
	int ret;
	if (pcom == NULL || pcom->m_magic != SERIAL_DATA_MAGIC ||
	        pcom->m_hfile == NULL || pcom->m_prdptr != NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}


	pcom->m_prdptr = (uint8_t*)pbuf;
	pcom->m_rdleft = bufsize;
	pcom->m_inrd = 1;

	if (pcom->m_flshstate == FLUSH_BUFFER_FINISHED || DISABLED_FLUSHED > 0) {
		ret = __inner_read_serial(pcom);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		if (pcom->m_inrd == 0) {
			completed = 1;
		}
	} else {
		ret = _read_flushing(pcom);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		completed = ret;
	}

	return completed;
fail:
	SETERRNO(ret);
	return ret;
}

int __inner_write_serial(pwin_serial_priv_t pcom)
{
	int completed = 0;
	int ret;
	DWORD cbret;
	BOOL bret;

	while (pcom->m_wrleft > 0) {
		bret = WriteFile(pcom->m_hfile, pcom->m_pwrptr, (DWORD) pcom->m_wrleft, &cbret, &(pcom->m_wrov));
		if (!bret) {
			GETERRNO(ret);
			DEBUG_INFO("write [%s][%d:0x%x] error[%d]", pcom->m_name, pcom->m_wrleft,pcom->m_wrleft,ret);
			if (ret == -ERROR_IO_PENDING ) {
				DEBUG_INFO("write pending on [%d] wrleft", pcom->m_wrleft);
				return 0;
			} else if (ret == -ERROR_MORE_DATA) {
				pcom->m_wrleft -= cbret;
				pcom->m_pwrptr += cbret;
				pcom->m_wrtotal += cbret;
				if (pcom->m_wrleft == 0) {
					pcom->m_pwrptr = NULL;
					completed = 1;
					pcom->m_inwr = 0;
				}
				DEBUG_INFO("completed %d", completed);
				return completed;
			}
			ERROR_INFO("can not write [%s] on [%d] error[%d]", pcom->m_name, pcom->m_wrleft, ret);
			SETERRNO(ret);
			return ret;
		}

		DEBUG_INFO("cbret [%d]", cbret);
		pcom->m_wrleft -= cbret;
		pcom->m_pwrptr += cbret;
		pcom->m_wrtotal += cbret;
	}

	if (pcom->m_wrleft == 0) {
		completed = 1;
		pcom->m_pwrptr = NULL;
		pcom->m_inwr = 0;
	}
	return completed;
}


int write_serial(void* pcom1, void* pbuf, int bufsize)
{
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	int completed = 0;
	int ret;
	if (pcom == NULL || pcom->m_magic != SERIAL_DATA_MAGIC ||
	        pcom->m_inwr > 0) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	pcom->m_pwrptr = (uint8_t*)pbuf;
	pcom->m_wrleft = bufsize;
	pcom->m_inwr = 1;
	ret = __inner_write_serial(pcom);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	if (pcom->m_inwr == 0) {
		completed = 1;
	}
	return completed;
fail:
	SETERRNO(ret);
	return ret;
}

HANDLE get_serial_read_handle(void* pcom1)
{
	HANDLE hret = NULL;
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	if (pcom != NULL && pcom->m_magic == SERIAL_DATA_MAGIC && pcom->m_inrd > 0) {
		hret = pcom->m_hrdevt;
	}
	return hret;
}

HANDLE get_serial_write_handle(void* pcom1)
{
	HANDLE hret = NULL;
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	if (pcom != NULL && pcom->m_magic == SERIAL_DATA_MAGIC && pcom->m_inwr > 0) {
		hret = pcom->m_hrdevt;
	}
	return hret;
}

int _complete_flush(pwin_serial_priv_t pcom)
{
	int ret;
	int completed = 0;
	DWORD cbread;
	BOOL bret;
	int leftlen;
	if (pcom->m_flshstate == FLUSH_BUFFER_NONE) {
		ERROR_INFO("not valid state [FLUSH_BUFFER_NONE]");
		ret = -ERROR_INTERNAL_ERROR;
		goto fail;
	}  else if (pcom->m_flshstate == FLUSH_BUFFER_STARTING) {
		cbread = 0;
		bret = GetOverlappedResult(pcom->m_hfile, &(pcom->m_rdov), &cbread, FALSE);
		if (!bret) {
			GETERRNO(ret);
			if (ret != -ERROR_IO_PENDING && ret != -ERROR_MORE_DATA) {
				ERROR_INFO("get rdov [%s] error[%d]", pcom->m_name, ret);
				goto fail;
			}
		}
		DEBUG_INFO("cbread [%ld:0x%lx]",cbread,cbread);
		pcom->m_flshlen += cbread;
		if (pcom->m_flshlen == pcom->m_flshsize) {
			pcom->m_flshstate = FLUSH_BUFFER_COPYING;
		} else {
			/*we need to read*/
			ret = _read_flushing(pcom);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
		}
		if (pcom->m_rdleft > 0) {
			leftlen = pcom->m_flshlen - pcom->m_flshrlen;
			if (leftlen > pcom->m_rdleft) {
				leftlen = pcom->m_rdleft;
			}

			if (leftlen > 0) {
				memcpy(pcom->m_prdptr, &(pcom->m_flshbuf[pcom->m_flshrlen]), (size_t)leftlen);
				pcom->m_rdleft -= leftlen;
				pcom->m_prdptr += leftlen;
				pcom->m_flshrlen += leftlen;
			}
		}
		if (pcom->m_rdleft == 0) {
			pcom->m_prdptr = NULL;
			completed = 1;
		}

		if (pcom->m_flshstate == FLUSH_BUFFER_COPYING) {
			if (pcom->m_flshrlen == pcom->m_flshlen) {
				pcom->m_flshstate = FLUSH_BUFFER_FINISHED;
				if (pcom->m_rdleft > 0) {
					ret = __inner_read_serial(pcom);
					if (ret < 0) {
						GETERRNO(ret);
						goto fail;
					}
					completed = ret;
				} else {
					/*nothing to handle */
					ASSERT_IF(pcom->m_prdptr == NULL);
					ASSERT_IF(completed == 1);
					pcom->m_inrd = 0;
				}
			}
		} 
	} else if (pcom->m_flshstate == FLUSH_BUFFER_COPYING) {
		if (pcom->m_rdleft > 0) {
			leftlen = pcom->m_flshlen - pcom->m_flshrlen;
			if (leftlen > pcom->m_rdleft) {
				leftlen = pcom->m_rdleft;
			}
			if (leftlen > 0) {
				memcpy(pcom->m_prdptr, &(pcom->m_flshbuf[pcom->m_flshrlen]), (size_t)leftlen);
				pcom->m_rdleft -= leftlen;
				pcom->m_prdptr += leftlen;
				pcom->m_flshrlen += leftlen;
			}
		}

		if (pcom->m_rdleft == 0) {
			pcom->m_prdptr = NULL;
			completed = 1;
		}

		if (pcom->m_flshlen == pcom->m_flshrlen) {
			pcom->m_flshstate = FLUSH_BUFFER_FINISHED;
		}

		if (pcom->m_flshstate == FLUSH_BUFFER_FINISHED) {
			if (pcom->m_rdleft > 0) {
				/*read more data*/
				ret = __inner_read_serial(pcom);
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}
				completed = ret;
			} else {
				/*nothing to handle */
				ASSERT_IF(pcom->m_prdptr == NULL);
				ASSERT_IF(completed == 1);
				pcom->m_inrd = 0;
			}
		}
	}

	return completed;
fail:
	SETERRNO(ret);
	return ret;
}

int complete_serial_read(void* pcom1)
{
	int completed = 0;
	int ret;
	BOOL bret;
	DWORD cbread;
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	if (pcom == NULL || pcom->m_magic != SERIAL_DATA_MAGIC || pcom->m_hfile == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	if (pcom->m_inrd == 0) {
		completed = 1;
	} else if (pcom->m_flshstate != FLUSH_BUFFER_FINISHED && DISABLED_FLUSHED == 0) {
		ret = _complete_flush(pcom);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		completed = ret;
	} else {
		cbread = 0;
		bret = GetOverlappedResult(pcom->m_hfile, &(pcom->m_rdov), &cbread, FALSE);
		if (!bret) {
			GETERRNO(ret);
			if (ret != -ERROR_IO_PENDING && ret != -ERROR_MORE_DATA) {
				ERROR_INFO("get rdov [%s] error[%d]", pcom->m_name, ret);
				goto fail;
			}
		}
		DEBUG_INFO("cbread [%d]", cbread);
		pcom->m_rdleft -= cbread;
		pcom->m_prdptr += cbread;
		if (pcom->m_rdleft == 0) {
			pcom->m_prdptr = NULL;
			pcom->m_inrd = 0;
			completed = 1;
		} else {
			/*now to start again read*/
			ret = __inner_read_serial(pcom);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
			completed = ret;
		}
	}
	return completed;
fail:
	SETERRNO(ret);
	return ret;
}

int complete_serial_write(void* pcom1)
{
	int completed = 0;
	int ret;
	BOOL bret;
	DWORD cbwrite;
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	if (pcom == NULL || pcom->m_magic != SERIAL_DATA_MAGIC || pcom->m_hfile == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	if (pcom->m_inwr == 0) {
		completed = 1;
	} else {
		cbwrite = 0;
		bret = GetOverlappedResult(pcom->m_hfile, &(pcom->m_wrov), &cbwrite, FALSE);
		if (!bret) {
			GETERRNO(ret);
			//DEBUG_INFO("ret [%d]", ret);
			if (ret != -ERROR_IO_PENDING && ret != -ERROR_MORE_DATA && ret != -ERROR_IO_INCOMPLETE) {
				ERROR_INFO("get rdov [%s] error[%d]", pcom->m_name, ret);
				goto fail;
			}

			if (ret == -ERROR_IO_INCOMPLETE) {
				/*io not complete*/
				return 0;
			}
		}
		DEBUG_INFO("cbwrite [%d:0x%x]",cbwrite,cbwrite);
		pcom->m_wrleft -= cbwrite;
		pcom->m_pwrptr += cbwrite;
		if (pcom->m_wrleft == 0) {
			pcom->m_pwrptr = NULL;
			pcom->m_inwr = 0;
			completed = 1;
		} else {
			ret = __inner_write_serial(pcom);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
			completed = ret;
		}
	}
	return completed;
fail:
	SETERRNO(ret);
	return ret;
}


int get_serial_config_direct(void* pcom1, void** ppbuf, int* psize)
{
	pwin_serial_priv_t pcom = (pwin_serial_priv_t) pcom1;
	int ret;
	int retlen = 0;
	void* pretbuf = NULL;
	int retsize = 0;

	if (pcom == NULL) {
		if (ppbuf && *ppbuf) {
			free(*ppbuf);
			*ppbuf = NULL;
		}
		if (psize) {
			*psize = 0;
		}
		return 0;
	}

	if (pcom->m_magic != SERIAL_DATA_MAGIC || pcom->m_hfile == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	if (ppbuf == NULL || psize == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	pretbuf = *ppbuf;
	retsize = *psize;

	if (retsize < sizeof(pcom->m_dcb) || pretbuf == NULL) {
		if (retsize < sizeof(pcom->m_dcb)) {
			retsize = sizeof(pcom->m_dcb);
		}
		pretbuf = malloc((size_t)retsize);
		if (pretbuf == NULL) {
			GETERRNO(ret);
			goto fail;
		}
	}
	memset(pretbuf, 0, (size_t)retsize);
	retlen = sizeof(pcom->m_dcb);
	memcpy(pretbuf, &(pcom->m_dcb), (size_t)retlen);

	if (*ppbuf && *ppbuf != pretbuf) {
		free(*ppbuf);
	}
	*ppbuf = pretbuf;
	*psize = retsize;
	return retlen;
fail:
	if (pretbuf && pretbuf != *ppbuf) {
		free(pretbuf);
	}
	pretbuf = NULL;
	retsize = 0;
	SETERRNO(ret);
	return ret;
}

#pragma warning(pop)