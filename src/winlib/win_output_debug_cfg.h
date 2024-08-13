#ifndef __WIN_OUTPUT_DEBUG_CFG_H_34271879EF5F24AB1FD4CF1BDFD9DC92__
#define __WIN_OUTPUT_DEBUG_CFG_H_34271879EF5F24AB1FD4CF1BDFD9DC92__

#include <win_types.h>
#include "win_err.h"
#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#define  WINLIB_OUTPUT_LOCATION             0x1
#define  WINLIB_OUTPUT_TIMESTAMP            0x2
#define  WINLIB_OUTPUT_LEVEL                0x4
#define  WINLIB_OUTPUT_MSG                  0x8

#define  WINLIB_DEBUGOUT_FILE_STDERR                 0x1
#define  WINLIB_DEBUGOUT_FILE_APPEND                 0x2
#define  WINLIB_DEBUGOUT_FILE_TRUNC                  0x3
#define  WINLIB_DEBUGOUT_FILE_BACKGROUND             0x4
#define  WINLIB_DEBUGOUT_FILE_ROTATE                 0x5
#define  WINLIB_DEBUGOUT_FILE_MASK                   0xff

#define  WINLIB_OUTPUT_ALL_MASK             (WINLIB_OUTPUT_LOCATION | WINLIB_OUTPUT_TIMESTAMP |WINLIB_OUTPUT_LEVEL |WINLIB_OUTPUT_MSG)


#pragma warning(push)
#pragma warning(disable:4577)
#pragma warning(disable:4530)
#include <vector>
#pragma warning(pop)

class WINLIB_API OutfileCfg
{
public:
	OutfileCfg();
	~OutfileCfg();
	int set_file_type(const char* fname,int type,uint64_t size,int maxfiles);
	int set_level(int level);
	int set_format(int fmtflag);
	int get_file_type(const char*& fname, int& type, uint64_t& size, int& maxfiles);
	int get_level();
	int get_format();
	OutfileCfg* clone();
private:
	char* m_fname;
	int m_level;
	int m_fmtflag;
	int m_maxfiles;
	uint64_t m_size;
	int m_type;
};

class WINLIB_API OutputCfg
{
public:
	OutputCfg();
	~OutputCfg();
	int insert_config(OutfileCfg& cfg);
	OutfileCfg* get_config(int idx);
	void clear_configs();
private:
	std::vector<OutfileCfg*> *m_cfgs;
};

#ifdef __cplusplus
extern "C" {
#endif


WINLIB_API int InitOutputEx2(OutputCfg* pcfgs);

#ifdef __cplusplus
};
#endif


#endif /* __WIN_OUTPUT_DEBUG_CFG_H_34271879EF5F24AB1FD4CF1BDFD9DC92__ */
