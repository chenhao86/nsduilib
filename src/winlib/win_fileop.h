#ifndef __WIN_FILEOP_H_9B720185ABBC9B1C19F2903D4A43CC38__
#define __WIN_FILEOP_H_9B720185ABBC9B1C19F2903D4A43CC38__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__

#include <win_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  STDOUT_FILE_FLAG         1
#define  STDERR_FILE_FLAG         2


#define FNAME_SIZE         512
#define READ_MODE          1
#define WRITE_MODE         2
#define RDWR_MODE          (READ_MODE | WRITE_MODE)


WINLIB_API int mktempfile_safe(char* inputtemplate,char**ppoutput,int* bufsize);
WINLIB_API int read_file_encoded(char* infile,char** ppoutbuf,int *bufsize);
WINLIB_API int read_file_whole(char* infile,char** ppoutbuf,int *bufsize);
WINLIB_API int read_stdin_whole(int freed,char** ppoutbuf,int *bufsize);
WINLIB_API int delete_file(const char* infile);
WINLIB_API int get_full_path(char* pinfile, char** ppfullpath, int *pfullsize);
WINLIB_API int write_file_whole(char* outfile,char* poutbuf,int outsize);
WINLIB_API int write_out_whole(int flag,char* poutbuf,int outsize);
WINLIB_API int create_directory(const char* dir);
WINLIB_API int remove_directory(const char* dir);
WINLIB_API void* open_file(const char* file,int mode);
WINLIB_API int copy_file_force(const char* srcfile,const char* dstfile);
WINLIB_API int read_file(void* pfile,uint64_t off,void* pbuf,uint32_t bufsize);
WINLIB_API int write_file(void* pfile,uint64_t off,void* pbuf,uint32_t bufsize);
WINLIB_API void close_file(void** ppfile);
WINLIB_API uint64_t get_file_size(void* pfile);
WINLIB_API int ioctl_file(void* pfile,uint32_t ctrlcode,void* pinbuf,int insize,void* poutbuf,int outsize);
WINLIB_API HANDLE get_file_handle(void* pfile);


/*****************************************
*  callback function input 
*     basedir:  basedir input enumerate_directory
*     curdir:   current enumerate directory
*     curpat:   is the sub item in the curdir
*     arg:      arg input enumerate_directory
*
*  return value:
*      >0  will continue
*      =0  will break not continue
*      <0  error will stop
*
******************************************/
typedef int (*enum_callback_t)(char* basedir,char* curdir,char *curpat,void* arg);
WINLIB_API int enumerate_directory(char* basedir,enum_callback_t callback,void* arg);


WINLIB_API void* create_file_ov(HANDLE hd,char* fname);
WINLIB_API void free_file_ov(void** ppov);
WINLIB_API int read_file_ov(void* pov,char* pbuf,int buflen);
WINLIB_API int read_complete_ov(void* pov);
WINLIB_API HANDLE get_read_handle_ov(void* pov);
WINLIB_API int write_file_ov(void* pov,char* pbuf,int buflen);
WINLIB_API int write_complete_ov(VOID* pov);
WINLIB_API HANDLE get_write_handle_ov(void* pov);
WINLIB_API int exist_file(const char* fname);
WINLIB_API int exist_dir(const char* dname);


#ifdef __cplusplus
};
#endif

#endif /* __WIN_FILEOP_H_9B720185ABBC9B1C19F2903D4A43CC38__ */
