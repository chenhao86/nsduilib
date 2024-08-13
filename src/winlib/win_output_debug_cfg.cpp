#include "win_output_debug_cfg.h"
#include "win_output_debug.h"

OutfileCfg::OutfileCfg()
{
	this->m_fname = NULL;
	this->m_level = BASE_LOG_ERROR;
	this->m_fmtflag = WINLIB_OUTPUT_ALL_MASK;
	this->m_maxfiles = 0;
	this->m_size = 0;
	this->m_type = WINLIB_DEBUGOUT_FILE_STDERR;
}

OutfileCfg::~OutfileCfg()
{
	if (this->m_fname) {
		free(this->m_fname);
	}
	this->m_fname = NULL;
	this->m_level = BASE_LOG_ERROR;
	this->m_fmtflag = WINLIB_OUTPUT_ALL_MASK;
	this->m_maxfiles = 0;
	this->m_size = 0;
	this->m_type = WINLIB_DEBUGOUT_FILE_STDERR;
}

int OutfileCfg::set_level(int level)
{
	int retlevel = -1;
	if (level >= BASE_LOG_FATAL && level <= BASE_LOG_TRACE) {
		retlevel = this->m_level;
		this->m_level = level;
	}
	return retlevel;
}

int OutfileCfg::set_format(int fmtflag)
{
	int retflag = -1;
	if ((fmtflag & WINLIB_OUTPUT_ALL_MASK) == fmtflag) {
		retflag = this->m_fmtflag;
		this->m_fmtflag = fmtflag;
	}
	return retflag;
}

int OutfileCfg::set_file_type(const char* fname,int type,uint64_t size,int maxfiles)
{
	int masktype =0;
	int ret;
	if (fname == NULL ) {
		masktype = (type & WINLIB_DEBUGOUT_FILE_MASK);
		if ((masktype == WINLIB_DEBUGOUT_FILE_BACKGROUND || masktype == WINLIB_DEBUGOUT_FILE_STDERR) &&
			(size == 0) && maxfiles == 0 ) {
			if (this->m_fname) {
				free(this->m_fname);
			}
			this->m_fname = NULL;
			this->m_type = type;
			this->m_size = size;
			this->m_maxfiles = maxfiles;
		} else {
			ret = -ERROR_INVALID_PARAMETER;
			goto fail;
		}
	} else {
		masktype = (type & WINLIB_DEBUGOUT_FILE_MASK);
		if (masktype == WINLIB_DEBUGOUT_FILE_APPEND || masktype == WINLIB_DEBUGOUT_FILE_TRUNC || 
			masktype == WINLIB_DEBUGOUT_FILE_ROTATE) {
			if (this->m_fname) {
				free(this->m_fname);
			}
			this->m_fname = NULL;
			this->m_fname = _strdup(fname);
			if (this->m_fname == NULL) {
				GETERRNO(ret);
				goto fail;
			}
			this->m_size = size;
			this->m_type = type;
			this->m_maxfiles = maxfiles;
		} else {
			ret = -ERROR_INVALID_PARAMETER;
			goto fail;
		}
	}

	return 1;
fail:
	SETERRNO(ret);
	return ret;
}

int OutfileCfg::get_file_type(const char*& fname, int& type, uint64_t& size, int& maxfiles)
{
	fname = this->m_fname;
	type = this->m_type;
	size = this->m_size;
	maxfiles = this->m_maxfiles;
	return 0;
}

int OutfileCfg::get_level()
{
	return this->m_level;
}

int OutfileCfg::get_format()
{
	return this->m_fmtflag;
}

OutfileCfg* OutfileCfg::clone()
{
	OutfileCfg* pret = new OutfileCfg();
	int ret;
	ret = pret->set_file_type(this->m_fname,this->m_type,this->m_size, this->m_maxfiles);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	ret = pret->set_level(this->m_level);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}
	ret = pret->set_format(this->m_fmtflag);
	if (ret < 0) {
		GETERRNO(ret);
		goto fail;
	}

	return pret;
fail:
	if (pret) {
		delete pret;
	}
	pret = NULL;
	SETERRNO(ret);
	return NULL;
}

OutputCfg::OutputCfg()
{
	this->m_cfgs  = new std::vector<OutfileCfg*>();
}

OutputCfg::~OutputCfg()
{
	this->clear_configs();
	delete this->m_cfgs;
}

int OutputCfg::insert_config(OutfileCfg& cfg)
{
	OutfileCfg* pret = cfg.clone();
	int ret = -ERROR_INVALID_PARAMETER;
	if (pret != NULL) {
		this->m_cfgs->push_back(pret);
		ret = 0;
	}
	return ret;	
}

OutfileCfg* OutputCfg::get_config(int idx)
{
	OutfileCfg* pret = NULL;
	if ((int)this->m_cfgs->size() > idx) {
		pret = this->m_cfgs->at((uint64_t)idx);
	}
	return pret;
}

void OutputCfg::clear_configs()
{
	while(this->m_cfgs != NULL && this->m_cfgs->size() > 0) {
		OutfileCfg* pcfg = this->m_cfgs->at(0);
		this->m_cfgs->erase(this->m_cfgs->begin());
		delete pcfg;
		pcfg = NULL;
	}
	return;
}