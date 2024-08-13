#ifndef __WIN_PROC_H_25C1849750B170DECA8546855D8BE495__
#define __WIN_PROC_H_25C1849750B170DECA8546855D8BE495__

#include <win_types.h>

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <stdarg.h>
#include <stdlib.h>

#pragma warning(push)
#pragma warning(disable:4514)
#include <stdio.h>

#pragma warning(pop)

#ifdef __cplusplus
extern "C" {
#endif

#define  PROC_PIPE_STDIN            0x1
#define  PROC_PIPE_STDOUT           0x2
#define  PROC_PIPE_STDERR           0x4
#define  PROC_STDIN_NULL            0x8
#define  PROC_STDOUT_NULL           0x10
#define  PROC_STDERR_NULL           0x20
#define  PROC_NO_WINDOW             0x100


#define  PROC_APP_MODE           1
#define  PROC_MONITOR_MODE       2


typedef struct __mod_info{
	void* m_pimgbase;
	char m_modfullname[MAX_PATH];
	uint32_t m_modsize;
} mod_info_t,*pmod_info_t;



WINLIB_API int get_pid_argv(int pid, char*** pppargv, int *pargvsize);

WINLIB_API void* start_cmd(int createflag, const char* prog, ...);
WINLIB_API void* start_cmdv(int createflag, char* prog[]);
WINLIB_API int start_cmd_detach(int createflag,const char* prog,...);
WINLIB_API int start_cmd_single_detach(int createflag,const char* prog);
WINLIB_API int start_cmdv_detach(int createflag,char* prog[]);
WINLIB_API HANDLE proc_get_proc(void* proc);
WINLIB_API int kill_proc(void* proc, int* exitcode);
WINLIB_API int get_proc_exit(void* proc, int *exitcode);
WINLIB_API int run_cmd_output(char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, const char* prog, ...);
WINLIB_API int run_cmd_outputa(char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, const char* prog, va_list ap);
WINLIB_API int run_cmd_outputv(char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, char* prog[]);
WINLIB_API int run_cmd_output_single(char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, char* prog);


WINLIB_API int run_cmd_event_output(HANDLE hevt, char* pin,  int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, const char* prog, ...);
WINLIB_API int run_cmd_event_outputa(HANDLE hevt, char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, const char* prog, va_list ap);
WINLIB_API int run_cmd_event_outputv(HANDLE hevt, char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, char* prog[]);
WINLIB_API int run_cmd_event_output_single(HANDLE hevt, char* pin, int insize, char** ppout, int *poutsize, char** pperr, int *perrsize, int *exitcode, int timeout, char* prog);

WINLIB_API int start_cmd_session_detach(DWORD session, const char* prog,...);
WINLIB_API int start_cmdv_session_detach(DWORD session, char* prog[]);
WINLIB_API int get_pids_by_name(const char* name, DWORD** ppids, int *psize);


WINLIB_API int wts_run_cmd_output(char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, const char* prog,...);
WINLIB_API int wts_run_cmd_outputa(char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, const char* prog,va_list ap);
WINLIB_API int wts_run_cmd_outputv(char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, char* prog[]);
WINLIB_API int wts_run_cmd_output_single(char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, char* prog);


WINLIB_API int wts_run_cmd_event_output(HANDLE hevt,char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, const char* prog,...);
WINLIB_API int wts_run_cmd_event_outputa(HANDLE hevt,char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, const char* prog,va_list ap);
WINLIB_API int wts_run_cmd_event_outputv(HANDLE hevt,char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, char* prog[]);
WINLIB_API int wts_run_cmd_event_output_single(HANDLE hevt,char* pin, int insize, char** ppout, int* poutsize, char** pperr, int *perrsize,int *exitcode, int timeout, char* prog);

WINLIB_API int wts_start_cmd_detach(int createflag,const char* prog,...);
WINLIB_API int wts_start_cmdv_detach(int createflag,char* prog[]);
WINLIB_API int wts_start_cmd_single_detach(int createflag,const char* prog);

WINLIB_API int process_num(char** ppnames,int numproc, int* pfinded);
WINLIB_API int get_module_info(int procid,const char* pmodname,pmod_info_t *ppinfo,int *psize);


WINLIB_API int is_wts_enabled(void);

WINLIB_API int get_temp_pipe_name(char* prefix, char** pptmp, int *psize);

WINLIB_API int kill_process(int pid);
WINLIB_API int process_exist(int pid);
WINLIB_API int list_proc(const char* procname, int** pppids,int *psize);

WINLIB_API int dump_process_security(FILE* fp,int pid);
WINLIB_API int proc_dacl_set(FILE* fp,int pid,char* maskstr,char* modestr, char* inheritstr, char* username);

WINLIB_API void stop_protect_monitor(void** ppmon);
WINLIB_API void* start_protect_monitor(char* curcmdline, char* peercmdline, int peerpid, int mode, int waitmills, int interval);
WINLIB_API int send_ctrlc(int pid);


#ifdef __cplusplus
};
#endif



#endif /* __WIN_PROC_H_25C1849750B170DECA8546855D8BE495__ */
