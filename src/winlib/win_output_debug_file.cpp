#include "win_output_debug_cfg.h"

class DebugOutIO
{
public:
	virtual ~DebugOutIO() {};
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr) = 0;
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen) = 0;
	virtual int write_buffer(char* pbuf, int buflen) = 0;
	virtual void flush() = 0;
	virtual int set_cfg(OutfileCfg* pcfg) = 0;
	virtual int set_level(int level) = 0;
};

class DebugOutBuffer : public DebugOutIO
{
public:
	DebugOutBuffer();
	virtual ~DebugOutBuffer();
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr);
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen);
	virtual int write_buffer(char* pbuf, int buflen);
	virtual void flush();
	virtual int set_cfg(OutfileCfg* pcfg);
	virtual int set_level(int level);	
protected:
	int m_level;
	int m_fmtflag;
};


int str_append_vsnprintf_safe(char** ppbuf, int *pbufsize, const char* fmt, va_list ap)
{
	char* pRetBuf = *ppbuf;
	char* pTmpBuf = NULL;
	char* pcurptr = NULL;
	size_t tmpsize;
	size_t retsize = (size_t)(*pbufsize);
	int nret, ret;
	size_t leftsize = retsize;
	size_t cntsize = 0;
	va_list origap;

	if (fmt == NULL) {
		if (*ppbuf) {
			free(*ppbuf);
		}
		*ppbuf = NULL;
		*pbufsize = 0;
		return 0;
	}

	if (pRetBuf == NULL || retsize < 32) {
		if (retsize < 32) {
			retsize = 32;
		}
		pRetBuf = (char*)malloc(retsize);
		if (pRetBuf == NULL) {
			GETERRNO(ret);
			goto fail;
		}
	}

	if (*ppbuf) {
		cntsize = strlen(*ppbuf);
	}

	if (cntsize > 0  ) {
		if (pRetBuf != *ppbuf) {
			memcpy(pRetBuf, *ppbuf, cntsize);
		}
		pRetBuf[cntsize] = 0x0;
		pcurptr = &(pRetBuf[cntsize]);
		leftsize = retsize - cntsize;
	} else {
		pcurptr = pRetBuf;
		leftsize = retsize;
	}

	va_copy(origap, ap);
try_again:
	va_copy(ap, origap);
	ret = vsnprintf(pcurptr, leftsize - 1, fmt, ap);
	if (ret == -1 || ret >= (int)(leftsize - 1)) {
		tmpsize = retsize << 1;
		pTmpBuf = (char*)malloc(tmpsize);
		if (pTmpBuf == NULL) {
			GETERRNO(ret);
			goto fail;
		}

		if (cntsize > 0 && *ppbuf != NULL) {
			memcpy(pTmpBuf, *ppbuf, cntsize);
			pTmpBuf[cntsize] = 0x0;
			pcurptr = &(pTmpBuf[cntsize]);
			leftsize = tmpsize - cntsize;
		} else {
			pTmpBuf[0] = 0;
			pcurptr = pTmpBuf;
			leftsize = tmpsize;
		}

		if (pRetBuf && pRetBuf != *ppbuf) {
			free(pRetBuf);
		}
		pRetBuf = NULL;
		pRetBuf = pTmpBuf;
		pTmpBuf = NULL;
		retsize = tmpsize;
		tmpsize = 0;
		goto try_again;
	}

	nret = ret + 1;
	nret += (int)cntsize;

	if (*ppbuf && *ppbuf != pRetBuf) {
		free(*ppbuf);
	}
	*ppbuf = pRetBuf;
	*pbufsize = (int)retsize;
	return nret;
fail:
	if (pRetBuf && pRetBuf != *ppbuf) {
		free(pRetBuf);
	}
	pRetBuf = NULL;
	SETERRNO(-ret);
	return ret;
}

int str_append_snprintf_safe(char**ppbuf, int*pbufsize, const char* fmt, ...)
{
	va_list ap;
	if (fmt == NULL) {
		if (*ppbuf) {
			free(*ppbuf);
		}
		*ppbuf = NULL;
		*pbufsize = 0;
		return 0;
	}
	va_start(ap, fmt);
	return str_append_vsnprintf_safe(ppbuf, pbufsize, fmt, ap);
}


int format_out_string(int fmtflag, int addline, char** ppoutstr, int* psize, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	int ret;
	int retlen = 0;
	int cnt = 0;
	if (fmtflag < 0) {
		return str_append_snprintf_safe(ppoutstr, psize, NULL);
	}

	if (fmtflag & WINLIB_OUTPUT_LOCATION) {
		ret = str_append_snprintf_safe(ppoutstr, psize, "%s", locstr);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		cnt ++;
	}

	if (fmtflag & WINLIB_OUTPUT_TIMESTAMP) {
		if (cnt > 0) {
			ret = str_append_snprintf_safe(ppoutstr, psize, " ");
		}
		ret = str_append_snprintf_safe(ppoutstr, psize, "%s", timestr);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		cnt ++;
	}

	if (fmtflag & WINLIB_OUTPUT_LEVEL) {
		if (cnt > 0) {
			ret = str_append_snprintf_safe(ppoutstr, psize, " ");
		}
		ret = str_append_snprintf_safe(ppoutstr, psize, "%s", tagstr);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		cnt ++;
	}

	if (fmtflag & WINLIB_OUTPUT_MSG) {
		if (cnt > 0) {
			ret = str_append_snprintf_safe(ppoutstr, psize, " ");
		}
		ret = str_append_snprintf_safe(ppoutstr, psize, "%s", msgstr);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}
		cnt ++;
	}

	if (cnt > 0) {
		if (addline > 0) {
			ret = str_append_snprintf_safe(ppoutstr, psize, "\n");
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
		}

		retlen = (int)strlen(*ppoutstr);
	}

	return retlen;

fail:
	str_append_snprintf_safe(ppoutstr, psize, NULL);
	SETERRNO(ret);
	return ret;
}

int DebugOutBuffer::write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	int retlen = 0;
	int ret;
	char* outstr = NULL;
	int outsize = 0;
	if (this->m_level >=  level) {
		ret = format_out_string(this->m_fmtflag, 1, &outstr, &outsize, locstr, timestr, tagstr, msgstr);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}

		retlen = ret;
		if (retlen > 0) {
			ret = this->write_buffer(outstr, retlen);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
		}
		format_out_string(-1, 0, &outstr, &outsize, NULL, NULL, NULL, NULL);
	}
	this->flush();
	return retlen;
fail:
	format_out_string(-1, 0, &outstr, &outsize, NULL, NULL, NULL, NULL);
	SETERRNO(ret);
	return ret;
}

int DebugOutBuffer::write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen)
{
	int retlen = 0;
	int ret;
	char* outstr = NULL;
	int outsize = 0;
	char* bufstr = NULL;
	int bufsize = 0;
	int outlen = 0;
	int i, lasti;

	uint8_t* pcurptr;
	if (this->m_level >= level) {
		ret = format_out_string(this->m_fmtflag, 0, &outstr, &outsize, locstr, timestr, tagstr, msgstr);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}

		retlen = ret;
		if (retlen > 0) {
			ret = this->write_buffer(outstr, retlen);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
		}
		format_out_string(-1, 0, &outstr, &outsize, NULL, NULL, NULL, NULL);
		if ((this->m_fmtflag & WINLIB_OUTPUT_MSG ) != 0) {
			pcurptr = (uint8_t*)pbuffer;
			lasti = 0;
			for (i = 0; i < buflen; i++) {

				if (outlen > 2000) {
					ret = this->write_buffer(bufstr, outlen);
					if (ret < 0) {
						GETERRNO(ret);
						goto fail;
					}
					retlen += ret;
					outlen = 0;
					str_append_snprintf_safe(&bufstr, &bufsize, NULL);
				}

				if ((i % 16) == 0) {
					if (i > 0) {
						ret = str_append_snprintf_safe(&bufstr, &bufsize, "    ");
						if (ret < 0) {
							GETERRNO(ret);
							goto fail;
						}

						while (lasti != i) {
							if (pcurptr[lasti] >= ' ' && pcurptr[lasti] <= '~') {
								ret = str_append_snprintf_safe(&bufstr, &bufsize, "%c", pcurptr[lasti]);
							} else {
								ret = str_append_snprintf_safe(&bufstr, &bufsize, ".");
							}
							if (ret < 0) {
								GETERRNO(ret);
								goto fail;
							}
							lasti ++;
						}
					}
					ret = str_append_snprintf_safe(&bufstr, &bufsize, "\n0x%08x:", i);
					if (ret < 0) {
						GETERRNO(ret);
						goto fail;
					}
				}

				ret = str_append_snprintf_safe(&bufstr, &bufsize, " 0x%02x", pcurptr[i]);
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}
				outlen = (int)strlen(bufstr);
			}

			if (lasti != buflen) {
				while ((i % 16) != 0) {
					ret =  str_append_snprintf_safe(&bufstr, &bufsize, "     ");
					if (ret < 0) {
						GETERRNO(ret);
						goto fail;
					}
					i ++;
				}
				ret = str_append_snprintf_safe(&bufstr, &bufsize, "    ");
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}

				while (lasti < buflen) {
					if (pcurptr[lasti] >= ' ' && pcurptr[lasti] <= '~') {
						ret = str_append_snprintf_safe(&bufstr, &bufsize, "%c", pcurptr[lasti]);
					} else {
						ret = str_append_snprintf_safe(&bufstr, &bufsize, ".");
					}
					if (ret < 0) {
						GETERRNO(ret);
						goto fail;
					}
					lasti ++;
				}
				outlen = (int)strlen(bufstr);
				ret = this->write_buffer(bufstr, outlen);
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}
				retlen += ret;
			}

		}
		if (retlen > 0) {
			str_append_snprintf_safe(&outstr, &outsize, NULL);
			ret = str_append_snprintf_safe(&outstr, &outsize, "\n");
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
			outlen = (int) strlen(outstr);
			ret = this->write_buffer(outstr, outlen);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
			retlen += outlen;
		}
	}
	this->flush();

	str_append_snprintf_safe(&bufstr, &bufsize, NULL);
	format_out_string(-1, 0, &outstr, &outsize, NULL, NULL, NULL, NULL);
	return retlen;
fail:
	str_append_snprintf_safe(&bufstr, &bufsize, NULL);
	format_out_string(-1, 0, &outstr, &outsize, NULL, NULL, NULL, NULL);
	SETERRNO(ret);
	return ret;
}


DebugOutBuffer::DebugOutBuffer()
{
	this->m_level = BASE_LOG_ERROR;
	this->m_fmtflag = WINLIB_OUTPUT_ALL_MASK;
}

DebugOutBuffer::~DebugOutBuffer()
{
}



void DebugOutBuffer::flush()
{
	return;
}

int DebugOutBuffer::write_buffer(char* pbuffer, int buflen)
{
	if (pbuffer) {
		pbuffer = pbuffer;
	}
	return buflen;
}


int DebugOutBuffer::set_cfg(OutfileCfg* pcfg)
{
	int ret;
	this->m_level = pcfg->get_level();
	if (this->m_level < 0) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}
	this->m_fmtflag = pcfg->get_format();
	if (this->m_fmtflag < 0) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}
	return 0;
fail:
	SETERRNO(ret);
	return ret;
}

int DebugOutBuffer::set_level(int level)
{
	int retval = this->m_level;
	this->m_level = level;
	return  retval;
}


class DebugOutStderr : public DebugOutBuffer
{
public:
	DebugOutStderr();
	virtual ~DebugOutStderr();
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr);
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen);
	virtual int write_buffer(char* pbuffer , int buflen);
	virtual void flush();
	virtual int set_cfg(OutfileCfg* pcfg);
};



DebugOutStderr::DebugOutStderr()
{
	this->m_level = BASE_LOG_ERROR;
	this->m_fmtflag = WINLIB_OUTPUT_ALL_MASK;
}

DebugOutStderr::~DebugOutStderr()
{
	fflush(stderr);
}

void DebugOutStderr::flush()
{
	fflush(stderr);
	return;
}

int DebugOutStderr::set_cfg(OutfileCfg* pcfg)
{
	const char* fname = NULL;
	int maxfiles = 0;
	int type = 0;
	uint64_t size = 0;
	int ret;

	ret = pcfg->get_file_type(fname, type, size, maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	if (fname != NULL || (type & WINLIB_DEBUGOUT_FILE_MASK ) != WINLIB_DEBUGOUT_FILE_STDERR || size != 0 || maxfiles != 0) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	ret = this->DebugOutBuffer::set_cfg(pcfg);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	return 0;
fail:
	SETERRNO(ret);
	return ret;
}

int DebugOutStderr::write_buffer(char* pbuffer, int buflen)
{
	fprintf(stderr, "%s", pbuffer);
	return buflen;
}

int DebugOutStderr::write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	return this->DebugOutBuffer::write_log(level, locstr, timestr, tagstr, msgstr);
}

int DebugOutStderr::write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen)
{
	return this->DebugOutBuffer::write_buffer_log(level, locstr, timestr, tagstr, msgstr, pbuffer, buflen);
}


class DebugOutBackground : public DebugOutBuffer
{
public:
	DebugOutBackground();
	virtual ~DebugOutBackground();
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr);
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen);
	virtual int write_buffer(char* pbuffer , int buflen);
	virtual void flush();
	virtual int set_cfg(OutfileCfg* pcfg);
};

DebugOutBackground::DebugOutBackground()
{
	this->m_level = BASE_LOG_ERROR;
	this->m_fmtflag = WINLIB_OUTPUT_ALL_MASK;
}

DebugOutBackground::~DebugOutBackground()
{
}

void DebugOutBackground::flush()
{
	return;
}

int DebugOutBackground::write_buffer(char* pbuffer, int buflen)
{
#ifdef UNICODE
	LPWSTR pWide = NULL;
	int len;
	BOOL bret;
	len = (int) strlen(pbuffer) + 1;
	pWide = (wchar_t*)malloc((size_t)((len + 1) * 2));
	if (pWide == NULL) {
		return 0;
	}
	//pWide = new wchar_t[(len+1) * 2];
	bret = MultiByteToWideChar(CP_ACP, NULL, pbuffer, -1, pWide, len * 2);
	if (bret) {
		OutputDebugStringW(pWide);
	} else {
		OutputDebugString(L"can not change fmt string");
	}
	//delete [] pWide;
	free(pWide);
#else
	//fprintf(stderr,"%s",pFmtStr);
	OutputDebugStringA(pbuffer);
	//fprintf(stderr,"Out %s",pFmtStr);
#endif
	return buflen;
}

int DebugOutBackground::write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	return this->DebugOutBuffer::write_log(level, locstr, timestr, tagstr, msgstr);
}

int DebugOutBackground::write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen)
{
	return this->DebugOutBuffer::write_buffer_log(level, locstr, timestr, tagstr, msgstr, pbuffer, buflen);
}

int DebugOutBackground::set_cfg(OutfileCfg* pcfg)
{
	const char* fname = NULL;
	int maxfiles = 0;
	int type = 0;
	uint64_t size = 0;
	int ret;

	ret = pcfg->get_file_type(fname, type, size, maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	if (fname != NULL || (type & WINLIB_DEBUGOUT_FILE_MASK) != WINLIB_DEBUGOUT_FILE_BACKGROUND || size != 0 || maxfiles != 0) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	ret = this->DebugOutBuffer::set_cfg(pcfg);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}


	return 0;
fail:
	SETERRNO(ret);
	return ret;
}


class DebugOutFileTrunc : public DebugOutBuffer
{
public:
	DebugOutFileTrunc();
	virtual ~DebugOutFileTrunc();
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr);
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen);
	virtual int write_buffer(char* pbuffer , int buflen);
	virtual void flush();
	virtual int set_cfg(OutfileCfg* pcfg);
protected:
	HANDLE m_hfile;
	char* m_name;
	uint64_t m_size;
	uint64_t m_filesize;
};

DebugOutFileTrunc::DebugOutFileTrunc()
{
	this->m_hfile = NULL;
	this->m_name = NULL;
	this->m_size = 0;
	this->m_filesize = 0;
}

DebugOutFileTrunc::~DebugOutFileTrunc()
{
	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name != NULL) {
		free(this->m_name);
	}
	this->m_name = NULL;
	this->m_size = 0;
	this->m_filesize = 0;
}

int DebugOutFileTrunc::write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	return this->DebugOutBuffer::write_log(level, locstr, timestr, tagstr, msgstr);
}

int DebugOutFileTrunc::write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen)
{
	return this->DebugOutBuffer::write_buffer_log(level, locstr, timestr, tagstr, msgstr, pbuffer, buflen);
}

int DebugOutFileTrunc::write_buffer(char* pbuffer , int buflen)
{
	int ret = -1;
	BOOL bret;
	DWORD wlen;
	if (this->m_hfile == NULL) {
		ret = -ERROR_INVALID_PARAMETER;
		SETERRNO(ret);
		return ret;
	}

	bret = WriteFile(this->m_hfile, pbuffer, (DWORD)buflen, &wlen, NULL);
	if (!bret) {
		GETERRNO(ret);
		SETERRNO(ret);
		return ret;
	}

	this->m_filesize += buflen;
	return buflen;
}

void DebugOutFileTrunc::flush()
{
	if (this->m_size > 0 && this->m_filesize >= this->m_size) {
		char* nname = NULL;
		int nsize = 0;
		int ret;
		BOOL bret;
		/*this means we exceed the file limited size, so we should change */
		if (this->m_hfile != NULL)  {
			CloseHandle(this->m_hfile);
		}
		this->m_hfile = NULL;
		/*now to give the name copy*/
		if (this->m_name != NULL) {
			ret = str_append_snprintf_safe(&nname, &nsize, "%s.1", this->m_name);
			if (ret >= 0)	 {
				bret = CopyFileA(this->m_name, nname, FALSE);
				if (bret) {
					/*now to create new file*/
					this->m_hfile = CreateFileA(this->m_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (this->m_hfile == INVALID_HANDLE_VALUE) {
						this->m_hfile = NULL;
					} else {
						/*to reset filesize to zero*/
						this->m_filesize = 0;
					}
				}
			}
			str_append_snprintf_safe(&nname, &nsize, NULL);
		}
	}
	return;
}

int DebugOutFileTrunc::set_cfg(OutfileCfg* pcfg)
{
	const char* fname = NULL;
	int maxfiles = 0;
	int type = 0;
	uint64_t size = 0;
	int ret;

	ret = pcfg->get_file_type(fname, type, size, maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	if (fname == NULL || (type & WINLIB_DEBUGOUT_FILE_MASK) != WINLIB_DEBUGOUT_FILE_TRUNC ||  maxfiles != 0) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name != NULL ) {
		free(this->m_name);
	}
	this->m_name = NULL;

	ret = this->DebugOutBuffer::set_cfg(pcfg);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	this->m_name = _strdup(fname);
	this->m_size = size;
	if (this->m_name == NULL) {
		GETERRNO(ret);
		goto fail;
	}

	this->m_hfile = CreateFileA(this->m_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (this->m_hfile == INVALID_HANDLE_VALUE) {
		GETERRNO(ret);
		if (ret == -ERROR_FILE_NOT_FOUND) {
			this->m_hfile = CreateFileA(this->m_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
	}

	if (this->m_hfile == INVALID_HANDLE_VALUE) {
		GETERRNO(ret);
		this->m_hfile = NULL;
		goto fail;
	}

	/*to make write file size 0 start ok*/
	this->m_filesize = 0;
	return 0;
fail:
	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name) {
		free(this->m_name);
	}
	this->m_name = NULL;
	SETERRNO(ret);
	return ret;
}


class DebugOutFileAppend : public DebugOutFileTrunc
{
public:
	DebugOutFileAppend();
	virtual ~DebugOutFileAppend();
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr);
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen);
	virtual int write_buffer(char* pbuffer , int buflen);
	virtual void flush();
	virtual int set_cfg(OutfileCfg* pcfg);
};

DebugOutFileAppend::DebugOutFileAppend()
{
	this->m_hfile = NULL;
	this->m_name = NULL;
	this->m_size = 0;
	this->m_filesize = 0;
}

DebugOutFileAppend::~DebugOutFileAppend()
{
	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name != NULL) {
		free(this->m_name);
	}
	this->m_name = NULL;
	this->m_size = 0;
	this->m_filesize = 0;
}


int DebugOutFileAppend::write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	return this->DebugOutFileTrunc::write_log(level, locstr, timestr, tagstr, msgstr);
}

int DebugOutFileAppend::write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen)
{
	return this->DebugOutFileTrunc::write_buffer_log(level, locstr, timestr, tagstr, msgstr, pbuffer, buflen);
}

int DebugOutFileAppend::write_buffer(char* pbuffer , int buflen)
{
	return this->DebugOutFileTrunc::write_buffer(pbuffer, buflen);
}

void DebugOutFileAppend::flush()
{
	this->DebugOutFileTrunc::flush();
	return;
}

int DebugOutFileAppend::set_cfg(OutfileCfg* pcfg)
{
	const char* fname = NULL;
	int maxfiles = 0;
	int type = 0;
	uint64_t size = 0;
	int ret;
	LARGE_INTEGER fsize;
	BOOL bret;

	ret = pcfg->get_file_type(fname, type, size, maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	if (fname == NULL || (type & WINLIB_DEBUGOUT_FILE_MASK ) != WINLIB_DEBUGOUT_FILE_APPEND ||  maxfiles != 0) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name != NULL ) {
		free(this->m_name);
	}
	this->m_name = NULL;

	ret = this->DebugOutBuffer::set_cfg(pcfg);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	this->m_name = _strdup(fname);
	this->m_size = size;
	if (this->m_name == NULL) {
		GETERRNO(ret);
		goto fail;
	}


	this->m_hfile = CreateFileA(this->m_name, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (this->m_hfile == INVALID_HANDLE_VALUE) {
		GETERRNO(ret);
		this->m_hfile = NULL;
		goto fail;
	}
	bret = GetFileSizeEx(this->m_hfile, &fsize);
	if (!bret) {
		GETERRNO(ret);
		goto fail;
	}
	this->m_filesize = (uint64_t)fsize.QuadPart;
	return 0;
fail:
	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name) {
		free(this->m_name);
	}
	this->m_name = NULL;
	SETERRNO(ret);
	return ret;
}


class DebugOutFileRotate : public DebugOutFileAppend
{
public:
	DebugOutFileRotate();
	virtual ~DebugOutFileRotate();
	virtual int write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr);
	virtual int write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen);
	virtual int write_buffer(char* pbuffer, int buflen);
	virtual void flush();
	virtual int set_cfg(OutfileCfg* pcfg);
protected:
	int rotate_file(int appmode);

	int m_maxfiles;
	int m_rsv1;
};


DebugOutFileRotate::DebugOutFileRotate()
{
	this->m_hfile = NULL;
	this->m_name = NULL;
	this->m_filesize = 0;
	this->m_size = 0;
	this->m_maxfiles = 0;
	this->m_rsv1 = 0;
}

DebugOutFileRotate::~DebugOutFileRotate()
{
	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name != NULL) {
		free(this->m_name);
	}
	this->m_name = NULL;
	this->m_size = 0;
	this->m_filesize = 0;
	this->m_maxfiles = 0;
	this->m_rsv1 = 0;
}

int DebugOutFileRotate::write_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr)
{
	return this->DebugOutBuffer::write_log(level, locstr, timestr, tagstr, msgstr);
}

int DebugOutFileRotate::write_buffer_log(int level, char* locstr, char* timestr, const char* tagstr, char* msgstr, void* pbuffer, int buflen)
{
	return this->DebugOutBuffer::write_buffer_log(level, locstr, timestr, tagstr, msgstr, pbuffer, buflen);
}

int DebugOutFileRotate::write_buffer(char* pbuffer , int buflen)
{
	return this->DebugOutFileTrunc::write_buffer(pbuffer, buflen);
}

int DebugOutFileRotate::rotate_file(int appmode)
{
	HANDLE hd = NULL;
	char* nname = NULL;
	int nsize = 0;
	char* oname = NULL;
	int osize = 0;
	int i;
	int ret;
	LARGE_INTEGER fsize;
	BOOL bret;
	/*now to make sure */
	if (this->m_maxfiles > 0) {
		for (i = (this->m_maxfiles - 1); i > 0; i--) {
			/*now to make sure it is exist*/
			str_append_snprintf_safe(&nname, &nsize, NULL);
			ret = str_append_snprintf_safe(&nname, &nsize, "%s.%d", this->m_name, i + 1);
			if (ret < 0) {
				goto fail;
			}
			str_append_snprintf_safe(&oname, &osize, NULL);
			ret =  str_append_snprintf_safe(&oname, &osize, "%s.%d", this->m_name, i);
			if (ret < 0) {
				goto fail;
			}

			/*now to open existing*/
			hd = CreateFileA(oname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hd == INVALID_HANDLE_VALUE) {
				continue;
			}

			/*now to copy the file*/
			CloseHandle(hd);
			hd = NULL;

			bret = CopyFileA(oname, nname, FALSE);
			if (!bret) {
				goto fail;
			}
		}



		/*now copy file*/
		str_append_snprintf_safe(&nname, &nsize, NULL);
		str_append_snprintf_safe(&oname, &osize, NULL);
		ret = str_append_snprintf_safe(&nname, &nsize, "%s.1", this->m_name);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}

		hd = CreateFileA(this->m_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hd != INVALID_HANDLE_VALUE) {
			CloseHandle(hd);
			hd = NULL;
			/*this means we have some files*/
			bret = CopyFileA(this->m_name, nname, FALSE);
			if (!bret) {
				GETERRNO(ret);
				goto fail;
			}
		} else {
			hd = NULL;
		}


	} else {
		int maxfiles = 1;

		for (maxfiles = 1;; maxfiles++) {
			str_append_snprintf_safe(&nname, &nsize, NULL);
			ret = str_append_snprintf_safe(&nname, &nsize, "%s.%d", this->m_name, maxfiles);
			if (ret < 0) {
				GETERRNO(ret);
				goto fail;
			}
			hd = CreateFileA(oname, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hd == INVALID_HANDLE_VALUE) {
				break;
			}
			CloseHandle(hd);
			hd = NULL;
		}
		if (maxfiles > 1) {
			for (i = (maxfiles); i > 1; i--) {
				str_append_snprintf_safe(&nname, &nsize, NULL);
				str_append_snprintf_safe(&oname, &osize, NULL);
				ret = str_append_snprintf_safe(&nname, &nsize, "%s.%d", this->m_name, i);
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}

				ret = str_append_snprintf_safe(&oname, &osize, "%s.%d", this->m_name, i - 1);
				if (ret < 0) {
					GETERRNO(ret);
					goto fail;
				}

				bret = CopyFileA(oname, nname, FALSE);
				if (!bret) {
					GETERRNO(ret);
					goto fail;
				}
			}
		}

		str_append_snprintf_safe(&nname, &nsize, NULL);
		str_append_snprintf_safe(&oname, &osize, NULL);
		ret = str_append_snprintf_safe(&nname, &nsize, "%s.1", this->m_name);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}

		ret = str_append_snprintf_safe(&oname, &osize, "%s", this->m_name);
		if (ret < 0) {
			GETERRNO(ret);
			goto fail;
		}

		hd = CreateFileA(oname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hd != INVALID_HANDLE_VALUE) {
			CloseHandle(hd);
			hd = NULL;
			bret = CopyFileA(oname, nname, FALSE);
			if (!bret) {
				GETERRNO(ret);
				goto fail;
			}
		} else {
			hd = NULL;
		}

	}

	ASSERT_IF(this->m_hfile == NULL);
	/*now to open the truncation file*/
	if (appmode) {
		this->m_hfile = CreateFileA(this->m_name, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (this->m_hfile == INVALID_HANDLE_VALUE) {
			GETERRNO(ret);
			this->m_hfile = NULL;
			goto fail;
		}
		bret = GetFileSizeEx(this->m_hfile, &fsize);
		if (!bret) {
			GETERRNO(ret);
			goto fail;
		}
		this->m_filesize = (uint64_t)fsize.QuadPart;
	} else {
		this->m_hfile = CreateFileA(this->m_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (this->m_hfile == INVALID_HANDLE_VALUE) {
			GETERRNO(ret);
			this->m_hfile = NULL;
			goto fail;
		}

		/*to set for null*/
		this->m_filesize = 0;
	}

	if (hd != NULL && hd != INVALID_HANDLE_VALUE)  {
		CloseHandle(hd);
	}
	hd = NULL;
	str_append_snprintf_safe(&nname, &nsize, NULL);
	str_append_snprintf_safe(&oname, &osize, NULL);

	return 0;
fail:
	if (hd != NULL && hd != INVALID_HANDLE_VALUE)  {
		CloseHandle(hd);
	}
	hd = NULL;
	str_append_snprintf_safe(&nname, &nsize, NULL);
	str_append_snprintf_safe(&oname, &osize, NULL);
	SETERRNO(ret);
	return ret;
}

void DebugOutFileRotate::flush()
{
	if (this->m_size > 0 && this->m_filesize >= this->m_size) {
		if (this->m_hfile != NULL) {
			CloseHandle(this->m_hfile);
		}
		this->m_hfile = NULL;
		this->rotate_file(0);
	}
	return;
}

int DebugOutFileRotate::set_cfg(OutfileCfg* pcfg)
{
	const char* fname = NULL;
	int maxfiles = 0;
	int type = 0;
	uint64_t size = 0;
	int ret;

	ret = pcfg->get_file_type(fname, type, size, maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	if (fname == NULL || (type & WINLIB_DEBUGOUT_FILE_MASK ) != WINLIB_DEBUGOUT_FILE_ROTATE ) {
		ret = -ERROR_INVALID_PARAMETER;
		goto fail;
	}

	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name != NULL ) {
		free(this->m_name);
	}
	this->m_name = NULL;

	ret = this->DebugOutBuffer::set_cfg(pcfg);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	this->m_name = _strdup(fname);
	this->m_size = size;
	if (this->m_name == NULL) {
		GETERRNO(ret);
		goto fail;
	}
	this->m_maxfiles = maxfiles;

	ret = this->rotate_file(1);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	return 0;
fail:
	if (this->m_hfile != NULL) {
		CloseHandle(this->m_hfile);
	}
	this->m_hfile = NULL;
	if (this->m_name) {
		free(this->m_name);
	}
	this->m_name = NULL;
	SETERRNO(ret);
	return ret;
}


DebugOutIO* get_cfg_out(OutfileCfg* pcfg)
{
	const char* fname = NULL;
	int type = 0;
	int maxfiles = 0;
	uint64_t size;
	int ret;
	DebugOutIO* pout = NULL;

	ret = pcfg->get_file_type(fname, type, size, maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	switch (type & WINLIB_DEBUGOUT_FILE_MASK) {
	case WINLIB_DEBUGOUT_FILE_STDERR:
		pout =  new DebugOutStderr();
		break;
	case WINLIB_DEBUGOUT_FILE_APPEND:
		pout =  new DebugOutFileAppend();
		break;
	case WINLIB_DEBUGOUT_FILE_TRUNC:
		pout = new DebugOutFileTrunc();
		break;
	case WINLIB_DEBUGOUT_FILE_BACKGROUND:
		pout = new DebugOutBackground();
		break;
	case WINLIB_DEBUGOUT_FILE_ROTATE:
		pout = new DebugOutFileRotate();
		break;
	default:
		ret =  -ERROR_NOT_SUPPORTED;
		goto fail;
	}

	ret = pout->set_cfg(pcfg);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	return pout;
fail:
	if (pout) {
		delete pout;
	}
	pout = NULL;
	SETERRNO(ret);
	return NULL;
}