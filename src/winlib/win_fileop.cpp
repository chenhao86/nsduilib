
#include <win_fileop.h>
#include <win_err.h>
#include <win_output_debug.h>
#include <win_uniansi.h>
#include <win_envop.h>
#include <win_strop.h>
#include <win_types.h>

#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4820)
#pragma warning(disable:4514)
#pragma warning(disable:4577)

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <vector>

#pragma warning(pop)

#if _MSC_VER >= 1910
#pragma warning(push)
/*disable Spectre warnings*/
#pragma warning(disable:5045)
#endif


#define TEMP_XSIZE      6

int mktempfile_safe(char* inputtemplate, char**ppoutput, int* bufsize)
{
    int ret = 0;
    char* pretout = NULL;
    int retlen = 0;
    int templen = 0;
    size_t sz = 0;
    errno_t err;
    char* plastpart = NULL;
    char* ptemppath = NULL;
    int temppathlen = 0;
    int i;

    if (inputtemplate == NULL) {
        if (ppoutput != NULL && *ppoutput != NULL) {
            free(*ppoutput);
        }
        if (ppoutput != NULL) {
            *ppoutput = NULL;
        }
        if (bufsize != NULL) {
            *bufsize = 0;
        }
        return 0;
    }

    if (ppoutput == NULL || bufsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }
    retlen = *bufsize;
    pretout = *ppoutput;
    plastpart = strrchr(inputtemplate, '\\');
    if (plastpart == NULL) {
        plastpart = inputtemplate;
    } else {
        plastpart ++;
    }
    ret = get_env_variable("TEMP", &ptemppath, &temppathlen);
    if (ret < 0) {
        GETERRNO(ret);
        ERROR_INFO("can not get TEMP env error[%d]", ret);
        goto fail;
    }
    /*to add \\ */
    templen = (int)strlen(plastpart) + 2;
    templen += (int)strlen(ptemppath);
    templen += TEMP_XSIZE;
    //DEBUG_INFO(" ");

    if (templen > retlen || pretout == NULL) {
        retlen = templen;
        pretout = (char*)malloc((size_t)retlen);
        if (pretout == NULL) {
            GETERRNO(ret);
            ERROR_INFO("can not malloc[%d] error[%d]", templen, ret);
            goto fail;
        }
    }
    memset(pretout, 0, (size_t)retlen);
    strncpy_s(pretout, (size_t)templen, ptemppath, (size_t)templen);
    strncat_s(pretout, (size_t)templen, "\\", (size_t)templen);
    strncat_s(pretout, (size_t)templen,plastpart, (size_t)templen);
    for (i = 0; i < TEMP_XSIZE; i++) {
        strncat_s(pretout, (size_t)templen,"X", (size_t)templen);
    }
    sz = strlen(pretout) + 1;
    err = _mktemp_s(pretout, sz);
    if (err != 0) {
        GETERRNO(ret);
        ERROR_INFO("can not mktemp [%s] error[%d]", pretout, ret);
        goto fail;
    }
    if (*ppoutput != NULL && pretout != *ppoutput) {
        free(*ppoutput);
    }
    *ppoutput = pretout;
    *bufsize = templen;
    get_env_variable(NULL, &ptemppath, &temppathlen);
    return templen ;
fail:
    if (pretout && pretout != *ppoutput) {
        free(pretout);
    }
    pretout = NULL;
    get_env_variable(NULL, &ptemppath, &temppathlen);
    SETERRNO(-ret);
    return ret;
}

#define MIN_BUF_SIZE   0x10000

int __read_fp_buffer(FILE* fp, char** ppoutbuf, int *pbufsize)
{
    size_t retlen = 0;
    size_t curlen;
    char* pretbuf = NULL;
    size_t retsize = 0;
    char* ptmpbuf = NULL;
    int ret;

    if (fp == NULL) {
        if (ppoutbuf != NULL) {
            if (*ppoutbuf != NULL) {
                free(*ppoutbuf);
            }
            *ppoutbuf = NULL;
        }
        if (pbufsize != NULL) {
            *pbufsize = 0;
        }
        return 0;
    }

    if (ppoutbuf == NULL || pbufsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretbuf = *ppoutbuf;
    retsize = (size_t) * pbufsize;

    if (retsize < MIN_BUF_SIZE || pretbuf == NULL) {
        if (retsize < MIN_BUF_SIZE) {
            retsize = MIN_BUF_SIZE;
        }
        pretbuf = (char*) malloc(retsize);
        if (pretbuf == NULL) {
            GETERRNO(ret);
            ERROR_INFO("alloc %d error[%d]", retsize, ret);
            goto fail;
        }
        memset(pretbuf, 0 , retsize);
    }


    while (1) {
        while (retlen >= retsize) {
            retsize += MIN_BUF_SIZE;
            if (retsize > retlen) {
                ptmpbuf = (char*) malloc(retsize);
                if (ptmpbuf == NULL) {
                    GETERRNO(ret);
                    ERROR_INFO("alloc %d error[%d]", retsize, ret);
                    goto fail;
                }
                memset(ptmpbuf, 0 , retsize);
                if (retlen > 0) {
                    memcpy(ptmpbuf, pretbuf, retlen);
                }
                if (pretbuf != NULL && pretbuf != *ppoutbuf) {
                    free(pretbuf);
                }
                pretbuf = ptmpbuf;
                ptmpbuf = NULL;
            }
        }

        curlen = retsize - retlen;
        ret = (int)fread(&(pretbuf[retlen]), (size_t)1, curlen, fp);
        if (ret < 0) {
            if (feof(fp)) {
                break;
            }
            GETERRNO(ret);
            ERROR_INFO("read [%d] error[%d]", retlen, ret);
            goto fail;
        }
        retlen += ret;
        if (ret < (int)curlen) {
            break;
        }
    }

    if (*ppoutbuf != NULL && *ppoutbuf != pretbuf) {
        free(*ppoutbuf);
    }
    *ppoutbuf = pretbuf;
    *pbufsize = (int)retsize;
    return (int)retlen;

fail:
    if (pretbuf != NULL && pretbuf != *ppoutbuf) {
        free(pretbuf);
    }
    pretbuf = NULL;
    SETERRNO(ret);
    return ret;
}


int read_file_encoded(char* infile, char** ppoutbuf, int *bufsize)
{
    int ret;
    FILE* fp = NULL;
    int filelen;
    int retlen;
    char* preadbuf = NULL;
    int readsize = 0;
    char* curbuf;
    wchar_t *pwbuf;

    if (infile == NULL) {
        return UnicodeToAnsi(NULL, ppoutbuf, bufsize);
    }

    if (ppoutbuf == NULL || bufsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    ret = fopen_s(&fp,infile,"rb");
    if (ret != 0) {
        GETERRNO(ret);
        ERROR_INFO("can not open[%s] error[%d]", infile, ret);
        goto fail;
    }


    ret = __read_fp_buffer(fp, &preadbuf, &readsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    filelen = ret;


    if (filelen > 2 && preadbuf[0] == 0xff && preadbuf[1] == 0xfe) {
        curbuf = (char*)(&(preadbuf[2]));
    } else {
        curbuf = (char*)preadbuf;
    }
    pwbuf = (wchar_t*)curbuf;

    ret = UnicodeToAnsi(pwbuf, ppoutbuf, bufsize);
    if (ret < 0) {
        goto fail;
    }
    retlen = ret;

    __read_fp_buffer(NULL, &preadbuf, &readsize);
    if (fp != NULL) {
        fclose(fp);
    }
    fp = NULL;
    return retlen;
fail:
    __read_fp_buffer(NULL, &preadbuf, &readsize);

    if (fp != NULL) {
        fclose(fp);
    }
    fp = NULL;
    SETERRNO(-ret);
    return ret;
}


int read_file_whole(char* infile, char** ppoutbuf, int *bufsize)
{
    int ret = 0;
    FILE* fp = NULL;
    if (infile == NULL) {
        return __read_fp_buffer(NULL, ppoutbuf, bufsize);
    }

    if (ppoutbuf == NULL || bufsize == NULL) {
        ret = -EINVAL;
        SETERRNO(ret);
        return ret;
    }

    ret = fopen_s(&fp,infile,"rb");
    if (ret != 0) {
        GETERRNO(ret);
        ERROR_INFO("can not open[%s] error[%d]", infile, ret);
        goto fail;
    }

    ret = __read_fp_buffer(fp, ppoutbuf, bufsize);
    if (ret < 0) {
        goto fail;
    }

    if (fp != NULL) {
        fclose(fp);
    }
    fp = NULL;
    return ret;
fail:
    if (fp != NULL) {
        fclose(fp);
    }
    fp = NULL;
    SETERRNO(-ret);
    return ret;
}

int delete_file(const char* infile)
{
    int ret;
    BOOL bret;
    TCHAR* ptfile = NULL;
    int tfilesize = 0;

    if (infile == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(-ret);
        return ret;
    }

    ret = AnsiToTchar(infile, &ptfile, &tfilesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    bret = DeleteFile(ptfile);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not delete [%s] error[%d]", infile, ret);
        goto fail;
    }

    AnsiToTchar(NULL, &ptfile, &tfilesize);
    return 0;
fail:
    AnsiToTchar(NULL, &ptfile, &tfilesize);
    SETERRNO(ret);
    return ret;
}

int read_stdin_whole(int freed, char** ppoutbuf, int *bufsize)
{
    if (freed) {
        return __read_fp_buffer(NULL, ppoutbuf, bufsize);
    }

    return __read_fp_buffer(stdin, ppoutbuf, bufsize);
}

int get_full_path(char* pinfile, char** ppfullpath, int *pfullsize)
{
    int ret;
    TCHAR* ptinfile = NULL;
    int tinsize = 0;
    TCHAR* ptfullpath = NULL;
    DWORD tfullsize = 0;
    TCHAR* ptlongpath = NULL;
    DWORD tlongsize = 0;
    int retlen = 0;
    DWORD dret;
    TCHAR* pfilepart = NULL;

    if (ppfullpath == NULL || pfullsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (pinfile == NULL) {
        TcharToAnsi(NULL, ppfullpath, pfullsize);
        return 0;
    }

    ret = AnsiToTchar(pinfile, &ptinfile, &tinsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    tfullsize = 1024;

try_again:
    if (ptfullpath) {
        free(ptfullpath);
    }
    ptfullpath = NULL;

    ptfullpath = (TCHAR*) malloc(tfullsize * sizeof(TCHAR));
    if (ptfullpath == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc [%d] error[%d]", tfullsize * sizeof(TCHAR), ret);
        goto fail;
    }

    dret = GetFullPathName(ptinfile, tfullsize, ptfullpath, &pfilepart);
    if (dret == 0) {
        GETERRNO(ret);
        ERROR_INFO("[%s] fullname error[%d]", pinfile, ret);
        goto fail;
    } else if (dret >= (tfullsize - 1)) {
        tfullsize <<= 1;
        goto try_again;
    }

    tlongsize = 1024;
get_long_again:
    if (ptlongpath) {
        free(ptlongpath);
    }
    ptlongpath = NULL;
    ptlongpath = (TCHAR*) malloc(tlongsize * sizeof(TCHAR));
    if (ptlongpath == NULL) {
        GETERRNO(ret);
        ERROR_INFO("alloc [%d] error[%d]", tlongsize * sizeof(TCHAR), ret);
        goto fail;
    }

    dret = GetLongPathName(ptfullpath, ptlongpath, tlongsize);
    if (dret == 0) {
        GETERRNO(ret);
        ERROR_INFO("[%s] longname error[%d]", pinfile, ret);
        goto fail;
    } else if (dret >= (tlongsize - 1)) {
        tlongsize <<= 1;
        goto get_long_again;
    }

    ret = TcharToAnsi(ptlongpath, ppfullpath, pfullsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    retlen = ret;

    if (ptlongpath != NULL)  {
        free(ptlongpath);
    }
    ptlongpath = NULL;

    if (ptfullpath != NULL) {
        free(ptfullpath);
    }
    ptfullpath = NULL;
    tfullsize = 0;
    AnsiToTchar(NULL, &ptinfile, &tinsize);

    return retlen;
fail:
    if (ptlongpath != NULL)  {
        free(ptlongpath);
    }
    ptlongpath = NULL;

    if (ptfullpath != NULL) {
        free(ptfullpath);
    }
    ptfullpath = NULL;
    tfullsize = 0;
    AnsiToTchar(NULL, &ptinfile, &tinsize);
    SETERRNO(ret);
    return ret;
}

int __write_fp_out(FILE* fp, char* poutbuf, int outsize)
{
    size_t outlen = 0;
    size_t curlen = 0;
    int ret;

    if (fp == NULL || poutbuf == NULL || outsize < 0) {
        ret  = -EINVAL;
        goto fail;
    }

    while ( (int)outlen < outsize) {
        curlen = (outsize - outlen);
        ret = (int) fwrite(&(poutbuf[outlen]), 1, curlen, fp);
        if (ret < 0) {
            GETERRNO(ret);
            ERROR_INFO("write [%d] error[%d]", outlen, ret);
            goto fail;
        }
        outlen += ret;
        if (ret == 0) {
            /*it means we have something wrong*/
            break;
        }
    }

    return (int) outlen;
fail:
    SETERRNO(ret);
    return ret;
}

int write_file_whole(char* outfile, char* poutbuf, int outsize)
{
    FILE* fp = NULL;
    int ret = 0;

    if (outfile == NULL) {
        ret = -EINVAL;
        goto fail;
    }

    ret = fopen_s(&fp,outfile,"wb");
    if (ret != 0) {
        GETERRNO(ret);
        ERROR_INFO("open [%s] error[%d]", outfile, ret);
        goto fail;
    }

    ret = __write_fp_out(fp, poutbuf, outsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (fp != NULL) {
        fclose(fp);
    }
    fp = NULL;
    return ret;
fail:
    if (fp != NULL) {
        fclose(fp);
    }
    fp = NULL;
    SETERRNO(ret);
    return ret;
}

int write_out_whole(int flag, char* poutbuf, int outsize)
{
    FILE* fp = NULL;
    int ret;

    if (flag == STDOUT_FILE_FLAG) {
        fp = stdout;
    } else if (flag == STDERR_FILE_FLAG) {
        fp = stderr;
    }

    if (fp == NULL) {
        ret = -EINVAL;
        goto fail;
    }

    ret = __write_fp_out(fp, poutbuf, outsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    return ret;
fail:
    SETERRNO(ret);
    return ret;
}

int __inner_create_dir(char* dir)
{
    char* longname = NULL;
    int longsize = 0;
    wchar_t* pwdir = NULL;
    int wsize = 0;
    int ret;
    int created = 1;
    BOOL bret;

    ret = snprintf_safe(&longname, &longsize, "\\\\?\\%s", dir);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToUnicode(longname, &pwdir, &wsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    bret = CreateDirectoryW(pwdir, NULL);
    if (!bret) {
        GETERRNO(ret);
        if (ret != -ERROR_ALREADY_EXISTS) {
            ERROR_INFO("create [%s] error[%d]", longname, ret);
            goto fail;
        } else {
            created = 0;
        }
    }

    AnsiToUnicode(NULL, &pwdir, &wsize);
    snprintf_safe(&longname, &longsize, NULL);
    return created;
fail:
    AnsiToUnicode(NULL, &pwdir, &wsize);
    snprintf_safe(&longname, &longsize, NULL);
    SETERRNO(ret);
    return ret;
}

int __get_volume_information(char* path)
{
    int ret;
    BOOL bret;
    char* namebuf = NULL;
    int namesize = 0;
    DWORD serialnum = 0;
    DWORD maxlength = 0;
    DWORD sysflag = 0;
    char* sysnamebuffer = NULL;
    DWORD sysnamesize = 0;

    namesize = 1;
    sysnamesize = 1;
try_again:
    if (namebuf) {
        free(namebuf);
    }
    namebuf = NULL;
    if (sysnamebuffer) {
        free(sysnamebuffer);
    }
    sysnamebuffer = NULL;

    namebuf = (char*) malloc((size_t)namesize);
    if (namebuf == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    sysnamebuffer = (char*) malloc(sysnamesize);
    if (sysnamebuffer == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    bret = GetVolumeInformationA(path,namebuf,(DWORD)namesize,&serialnum,&maxlength,&sysflag,sysnamebuffer,sysnamesize);
    if (!bret) {
        GETERRNO(ret);
        if (ret == -ERROR_MORE_DATA || ret == -ERROR_BAD_LENGTH) {
            sysnamesize <<= 1;
            namesize <<= 1;
            goto try_again;
        }
        ERROR_INFO("can not get [%s] error[%d]", path,ret);
        goto fail;
    }
    
    DEBUG_INFO("get [%s] name[%s] sysname [%s]", path, namebuf,sysnamebuffer);


    if (namebuf) {
        free(namebuf);
    }
    namebuf = NULL;
    namesize = 0;
    if (sysnamebuffer) {
        free(sysnamebuffer);
    }
    sysnamebuffer = NULL;
    sysnamesize = 0;
    serialnum = 0;
    maxlength = 0;
    sysflag = 0;

    return 0;
fail:
    if (namebuf) {
        free(namebuf);
    }
    namebuf = NULL;
    namesize = 0;
    if (sysnamebuffer) {
        free(sysnamebuffer);
    }
    sysnamebuffer = NULL;
    sysnamesize = 0;
    serialnum = 0;
    maxlength = 0;
    sysflag = 0;
    SETERRNO(ret);
    return ret;

}

int create_directory(const char* dir)
{
    char* fulldir = NULL;
    int fullsize = 0;
    //char* newfulldir = NULL;
    int ret;
    DWORD dret;
    char* partdir = NULL;
    int partsize = 0;
    char* lastptr = NULL;
    int created = 1;
    int createcnt = 0;
    char* newdir=NULL;
    int dirsize=0;
    int dirlen = 0;
    if (dir == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    dirlen = (int)strlen(dir);
    if (dirlen == 0) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    dirsize = dirlen + 3;
    newdir = (char*)malloc((size_t)dirsize);
    if (newdir == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    memset(newdir,0,(size_t)dirsize);
    memcpy(newdir, dir, (size_t)dirlen);
    if (dir[(dirlen - 1)] != '\\')  {
        /*to add last \ to the end*/
        newdir[dirlen] = '\\';
    }

    fullsize = 12;
get_full_again:
    if (fulldir) {
        free(fulldir);
    }
    fulldir = NULL;
    fulldir = (char*)malloc((size_t)fullsize);
    if (fulldir == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    dret = GetFullPathNameA(newdir, (DWORD)fullsize, fulldir, NULL);
    if (dret == 0) {
        GETERRNO(ret);
        ERROR_INFO("get fullpath for [%s] error[%d]", dir, ret);
        goto fail;
    }
    if ((int)dret > fullsize) {
        fullsize = (int)(dret << 1);
        goto get_full_again;
    }
    DEBUG_INFO("dret [%d]", dret);
    DEBUG_INFO("fulldir [%s]", fulldir);

    ret = __inner_create_dir(fulldir);
    if (ret < 0 ) {
        GETERRNO(ret);
        /*now we should give part directory get*/
        lastptr = strchr(fulldir, '\\');
        if (lastptr == NULL) {
            /*can not get full dir*/
            goto fail;
        }

        created = 0;
        /*to skip the \\*/
        lastptr ++;
        createcnt = 0;
        while (lastptr) {
            createcnt ++;
            lastptr = strchr(lastptr, '\\');
            if (lastptr == NULL) {
                if (createcnt <= 1) {
                    /*because it will like f:\ format ,so we should suppose this has*/
                    ret = __get_volume_information(fulldir);
                    if (ret < 0) {
                        GETERRNO(ret);
                        goto fail;
                    }
                    goto succ;
                }
                partsize = (int)strlen(fulldir);
            } else {
                partsize = (int)(lastptr - fulldir);
            }

            if (partdir) {
                free(partdir);
            }
            partdir = NULL;
            partdir = (char*)malloc((size_t)(partsize + 2));
            if (partdir == NULL) {
                GETERRNO(ret);
                goto fail;
            }

            memset(partdir, 0, (size_t)(partsize + 2));
            memcpy(partdir, fulldir, (size_t)partsize);
            DEBUG_INFO("partdir [%s]", partdir);
            ret = __inner_create_dir(partdir);
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
            if (ret > 0) {
                created = 1;
            }
            if (lastptr != NULL) {
                lastptr ++;
            }
        }
    }  else {
        created = ret;
    }

succ:
    if (partdir) {
        free(partdir);
    }
    partdir = NULL;
    partsize = 0;
    if (fulldir) {
        free(fulldir);
    }
    fulldir = NULL;
    if (newdir) {
        free(newdir);
    }
    newdir = NULL;
    return created;
fail:
    if (partdir) {
        free(partdir);
    }
    partdir = NULL;
    partsize = 0;
    if (fulldir) {
        free(fulldir);
    }
    fulldir = NULL;
    if (newdir) {
        free(newdir);
    }
    newdir = NULL;
    SETERRNO(ret);
    return ret;
}

int remove_directory(const char* dir)
{
    int ret = -ERROR_NOT_SUPPORTED;
    REFERENCE_ARG(dir);
    SETERRNO(ret);
    return ret;   
}

#define  FILE_OP_MAGIC 0x44319099

typedef struct _file_obj{
    uint32_t m_magic;
    int m_fnamesize;
    HANDLE m_hFile;
    char m_ansifile[FNAME_SIZE];
    TCHAR *m_pfname;
}file_obj_t,*pfile_obj_t;


int copy_file_force(const char* srcfile,const char* dstfile)
{
    TCHAR *pSF=NULL,*pDF=NULL;
    int ssize=0,dsize=0;
    int ret;
    BOOL bret;

    ret = AnsiToTchar(srcfile,&pSF,&ssize);
    if (ret < 0){
        goto fail;
    }

    ret = AnsiToTchar(dstfile,&pDF,&dsize);
    if (ret < 0){
        goto fail;
    }

    bret = CopyFile(pSF,pDF,FALSE);
    if (!bret){
        GETERRNO(ret);
        ERROR_INFO("can not copy (%s->%s) error(%d)",srcfile,dstfile,ret);
        goto fail;
    }

    AnsiToTchar(NULL,&pSF,&ssize);
    AnsiToTchar(NULL,&pDF,&dsize);
    return 0;
fail:
    AnsiToTchar(NULL,&pSF,&ssize);
    AnsiToTchar(NULL,&pDF,&dsize);
    SETERRNO(ret);
    return ret;
}


pfile_obj_t __init_file(const char* file)
{
    int ret;
    pfile_obj_t pfile=NULL;

    pfile = (pfile_obj_t)malloc(sizeof(*pfile));
    if (pfile == NULL){
        GETERRNO(ret);
        ERROR_INFO("can not alloc(%d) error(%d)",sizeof(*pfile),ret);
        goto fail;
    }
    memset(pfile,0,sizeof(*pfile));
    pfile->m_magic = FILE_OP_MAGIC;
    pfile->m_hFile = INVALID_HANDLE_VALUE;
    strncpy_s(pfile->m_ansifile,sizeof(pfile->m_ansifile),file,sizeof(pfile->m_ansifile));
    ret = AnsiToTchar(file,&(pfile->m_pfname),&pfile->m_fnamesize);
    if (ret < 0){
        goto fail;
    }

    return pfile;
fail:
    close_file((void**)&pfile);
    SETERRNO(ret);
    return NULL;
}

void close_file(void **ppobj)
{
    int ret;
    BOOL bret;
    pfile_obj_t pfile;
    if (ppobj == NULL){
        return;
    }

    pfile = (pfile_obj_t)*ppobj;

    if (pfile == NULL){
        return ;
    }

    if (pfile->m_magic != FILE_OP_MAGIC){
        ERROR_INFO("not valid 0x%p object",pfile);
    }
    if (pfile->m_hFile != INVALID_HANDLE_VALUE){
        bret = CloseHandle(pfile->m_hFile);
        if (!bret){
            GETERRNO(ret);
            ERROR_INFO("close (%s) error(%d)",pfile->m_ansifile,ret);
        }
    }
    pfile->m_hFile = INVALID_HANDLE_VALUE;
    AnsiToTchar(NULL,&(pfile->m_pfname),&(pfile->m_fnamesize));
    memset(pfile->m_ansifile,0,sizeof(pfile->m_ansifile));
    free(pfile);
    *ppobj = NULL;
    return ;
}

void*  open_file(const char* file,int mode)
{
    pfile_obj_t pfile=NULL;
    int ret;
    DWORD accmode=0,sharemode=0,createmode=0,attrmode=0;

    if (file == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return NULL;
    }

    pfile = __init_file(file);
    if (pfile == NULL){
        GETERRNO(ret);
        goto fail;
    }

    if (mode & READ_MODE){
        accmode |= GENERIC_READ;
        sharemode |= FILE_SHARE_READ;
    }

    if (mode & WRITE_MODE){
        accmode |= GENERIC_WRITE;
        sharemode |= FILE_SHARE_WRITE;
    }

    if ( (mode & WRITE_MODE)==0){
        createmode = OPEN_EXISTING;
        attrmode = FILE_ATTRIBUTE_READONLY;
    }else {
        createmode = OPEN_ALWAYS;
        attrmode = FILE_ATTRIBUTE_NORMAL;
    }

    DEBUG_INFO("will open [%s]", file);

    pfile->m_hFile = CreateFile(pfile->m_pfname,accmode,sharemode,NULL,createmode,attrmode,NULL);
    if (pfile->m_hFile == INVALID_HANDLE_VALUE){
        GETERRNO(ret);
        ERROR_INFO("can not open(%s) mode(%d) error(%d)",file,mode,ret);
        goto fail;
    }
    return pfile;
fail:
    ERROR_INFO("fail open [%s]",file);
    close_file((void**)&pfile);
    SETERRNO(ret);
    return NULL;
}

int __seek_file(pfile_obj_t pfile,uint64_t off)
{
    BOOL bret;
    int ret;
    LARGE_INTEGER movpos,retpos;
    movpos.QuadPart = (LONGLONG)off;
    ASSERT_IF(pfile && pfile->m_hFile != INVALID_HANDLE_VALUE);
    bret = SetFilePointerEx(pfile->m_hFile,movpos,&retpos,FILE_BEGIN);
    if (!bret){
        GETERRNO(ret);
        ERROR_INFO("move (%s) to (0x%llx:%lld) error(%d)",pfile->m_ansifile,off,off,ret);
        goto fail;
    }

    if (movpos.QuadPart != retpos.QuadPart){
        ret = -ERROR_INVALID_DATA;
        ERROR_INFO("move (%s) to (0x%llx:%lld) retpos (0x%llx:%lld)",pfile->m_ansifile,off,off,retpos.QuadPart,retpos.QuadPart);
        goto fail;
    }

    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int read_file(void* pobj,uint64_t off,void* pbuf,uint32_t bufsize)
{
    int ret;
    pfile_obj_t pfile = (pfile_obj_t)pobj;
    void *pcurptr=pbuf;
    addr_t curaddr;
    uint32_t leftsize=bufsize;
    BOOL bret;
    DWORD retsize;
    if (pfile == NULL ||  pfile->m_magic != FILE_OP_MAGIC || pfile->m_hFile == INVALID_HANDLE_VALUE){
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    ret = __seek_file(pfile,off);
    if (ret < 0){
        goto fail;
    }

    while(leftsize > 0){
        bret = ReadFile(pfile->m_hFile,pcurptr,leftsize,&retsize,NULL);
        if (!bret){
            GETERRNO(ret);
            if (ret == -ERROR_IO_PENDING){
                continue;
            }
            ERROR_INFO("can not read (%s) at (0x%llx:%lld) with size(%d) leftsize(%d) error(%d)",pfile->m_ansifile,off,off,bufsize,leftsize,ret);
            goto fail;
        }

        if (retsize == 0){
            ret = -ERROR_HANDLE_EOF;
            ERROR_INFO("(%s) eof",pfile->m_ansifile);
            goto fail;
        }

        curaddr = (addr_t)pcurptr;
        curaddr += retsize;
        pcurptr = (void*)curaddr;
        leftsize -= retsize;
    }


    return (int)bufsize;
fail:
    SETERRNO(ret);
    return ret;
}

int write_file(void* pobj,uint64_t off,void* pbuf,uint32_t bufsize)
{
    int ret;
    pfile_obj_t pfile = (pfile_obj_t)pobj;
    void *pcurptr=pbuf;
    addr_t curaddr;
    uint32_t leftsize=bufsize;
    BOOL bret;
    DWORD retsize;
    if (pfile == NULL ||  pfile->m_magic != FILE_OP_MAGIC || pfile->m_hFile == INVALID_HANDLE_VALUE){
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    ret = __seek_file(pfile,off);
    if (ret < 0){
        goto fail;
    }

    while(leftsize > 0){
        bret = WriteFile(pfile->m_hFile,pcurptr,leftsize,&retsize,NULL);
        if (!bret){
            GETERRNO(ret);
            if (ret == -ERROR_IO_PENDING){
                continue;
            }
            ERROR_INFO("can not write (%s) at (0x%llx:%lld) with size(%d) leftsize(%d) error(%d)",pfile->m_ansifile,off,off,bufsize,leftsize,ret);
            goto fail;
        }

        if (retsize == 0){
            ret = -ERROR_HANDLE_EOF;
            ERROR_INFO("(%s) eof",pfile->m_ansifile);
            goto fail;
        }

        curaddr = (addr_t)pcurptr;
        curaddr += retsize;
        pcurptr = (void*)curaddr;
        leftsize -= retsize;
    }


    return (int)bufsize;
fail:
    SETERRNO(ret);
    return ret;
}

uint64_t get_file_size(void* pobj)
{
    LARGE_INTEGER size;
    pfile_obj_t pfile = (pfile_obj_t) pobj;
    BOOL bret;
    int ret;
    if (pfile == NULL || pfile->m_magic != FILE_OP_MAGIC || pfile->m_hFile == INVALID_HANDLE_VALUE){
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return MAX_UINT64;
    }
    bret = GetFileSizeEx(pfile->m_hFile,&size);
    if (!bret){
        GETERRNO(ret);
        ERROR_INFO("can not get (%s) size error(%d)",pfile->m_ansifile,ret);
        goto fail;
    }
    SETERRNO(0);
    return (uint64_t)size.QuadPart;
fail:
    SETERRNO(ret);
    return MAX_UINT64;
}

int ioctl_file(void* pobj,uint32_t ctrlcode,void* pinbuf,int insize,void* poutbuf,int outsize)
{
    int ret,nret;
    BOOL bret;
    DWORD dret;
    pfile_obj_t pfile = (pfile_obj_t)pobj;
    if (pfile == NULL || pfile->m_magic != FILE_OP_MAGIC || pfile->m_hFile == INVALID_HANDLE_VALUE){
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    bret = DeviceIoControl(pfile->m_hFile,ctrlcode,pinbuf,(DWORD)insize,poutbuf,(DWORD)outsize,&dret,NULL);
    if (!bret){
        GETERRNO(ret);
        ERROR_INFO("can not ioctrl %s file error(%d)",pfile->m_ansifile,ret);
        goto fail;
    }
    nret = (int)dret;
    return nret;
fail:
    SETERRNO(ret);
    return ret;
}

HANDLE get_file_handle(void* pobj)
{
    pfile_obj_t pfile = (pfile_obj_t)pobj;

    if (pfile == NULL || pfile->m_magic != FILE_OP_MAGIC){
        return INVALID_HANDLE_VALUE;
    }
    return pfile->m_hFile;
}


typedef struct __file_sub_item {
    DWORD m_attr;
    DWORD m_res1;
    char* m_name;
} file_sub_item_t,*pfile_sub_item_t;

//        DEBUG_INFO("[%d][%s].attribute [0x%lx]", retlen,pfinddata->cFileName,                     
//            pfinddata->dwFileAttributes);                                                        

#define  ADD_SUB_ITEM(pfinddata)                                                                  \
    do{                                                                                           \
        if (pretitems == NULL || retlen >= retsize || retsize == 0) {                             \
            if (retsize == 0) {                                                                   \
                retsize = 4;                                                                      \
            } else {                                                                              \
                retsize <<= 1;                                                                    \
            }                                                                                     \
            ptmpitems = (pfile_sub_item_t)malloc(sizeof(*ptmpitems) * retsize);                   \
            if (ptmpitems == NULL) {                                                              \
                GETERRNO(ret);                                                                    \
                ERROR_INFO("alloc [%ld] error[%d]", sizeof(*ptmpitems) * retsize, ret);           \
                goto fail;                                                                        \
            }                                                                                     \
            memset(ptmpitems, 0 ,sizeof(*ptmpitems) * retsize);                                   \
            if (retlen > 0) {                                                                     \
                memcpy(ptmpitems,pretitems, sizeof(*ptmpitems) * retlen);                         \
            }                                                                                     \
            if (pretitems && pretitems != *ppitems) {                                             \
                free(pretitems);                                                                  \
            }                                                                                     \
            pretitems = ptmpitems;                                                                \
            ptmpitems = NULL;                                                                     \
        }                                                                                         \
        pretitems[retlen].m_attr = pfinddata->dwFileAttributes;                                   \
        ASSERT_IF(pretitems[retlen].m_name == NULL);                                              \
        curlen = strlen(pfinddata->cFileName) + 1;                                                \
        pretitems[retlen].m_name = (char*)malloc(curlen);                                         \
        if (pretitems[retlen].m_name == NULL) {                                                   \
            GETERRNO(ret);                                                                        \
            ERROR_INFO("alloc fname[%d] error[%d]", curlen, ret);                                 \
            goto fail;                                                                            \
        }                                                                                         \
        memcpy(pretitems[retlen].m_name, pfinddata->cFileName,curlen);                            \
        retlen ++;                                                                                \
    }while(0)


int __list_all_subitems(char* basedir,char* curdir,pfile_sub_item_t *ppitems,int *pitemsize)
{
    int i;
    int ret;
    pfile_sub_item_t pretitems=NULL;
    pfile_sub_item_t ptmpitems=NULL;
    int retsize=0;
    int retlen =0;
    WIN32_FIND_DATAA* pfinddata=NULL;
    HANDLE hfd=INVALID_HANDLE_VALUE;
    size_t curlen;
    BOOL bret;
    char* pat=NULL;
    int patsize=0;
    if (basedir == NULL || curdir == NULL) {
        if (ppitems && *ppitems) {
            pretitems = *ppitems;
            if (pitemsize) {
                for(i=0;i<*pitemsize;i++) {
                    if (pretitems[i].m_name) {
                        free(pretitems[i].m_name);
                        pretitems[i].m_name = NULL;
                    }
                    pretitems[i].m_attr = 0;
                }
            }
            free(*ppitems);
            *ppitems = NULL;
        }
        if (pitemsize) {
            *pitemsize = 0;
        }
        return 0;
    }

    if (ppitems == NULL || pitemsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretitems = *ppitems;
    retsize = *pitemsize;

    /*first to remove the items*/
    if (pretitems && retsize > 0) {
        for(i=0;i<retsize;i++) {
            if (pretitems[i].m_name) {
                free(pretitems[i].m_name);
                pretitems[i].m_name = NULL;
            }
        }
    }

    pfinddata = (WIN32_FIND_DATAA*)malloc(sizeof(*pfinddata));
    if (pfinddata == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    memset(pfinddata,0, sizeof(*pfinddata));

    ret = snprintf_safe(&pat,&patsize,"%s\\*",curdir);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    hfd = FindFirstFileA(pat,pfinddata);
    if (hfd == INVALID_HANDLE_VALUE) {
        GETERRNO(ret);
        ERROR_INFO("findfistfile [%s] error[%d]", pat,ret);
        goto fail;
    }
    
    ADD_SUB_ITEM(pfinddata);

    while(1) {
        memset(pfinddata,0, sizeof(*pfinddata));
        bret = FindNextFileA(hfd,pfinddata);
        if (!bret) {
            GETERRNO(ret);
            if (ret == -ERROR_NO_MORE_FILES) {
                break;
            }
            ERROR_INFO("[%s]find [%d] error[%d]",curdir,  retlen,ret);
            goto fail;
        }
        ADD_SUB_ITEM(pfinddata);
    }

    if (pfinddata) {
        free(pfinddata);
    }
    pfinddata = NULL;

    if (hfd != INVALID_HANDLE_VALUE && hfd != NULL) {
        FindClose(hfd);
    }
    hfd = INVALID_HANDLE_VALUE;

    snprintf_safe(&pat,&patsize,NULL);

    if (*ppitems && *ppitems != pretitems) {
        free(*ppitems);
    }
    *ppitems = pretitems;
    *pitemsize = retsize;
    return retlen;
fail:
    if (pfinddata) {
        free(pfinddata);
    }
    pfinddata = NULL;

    if (pretitems) {
        for(i=0;i<retsize;i++) {
            if (pretitems[i].m_name) {
                free(pretitems[i].m_name);
                pretitems[i].m_name = NULL;
            }
        }
    }
    if (pretitems && pretitems != *ppitems) {
        free(pretitems);
    }
    pretitems = NULL;

    if (hfd != INVALID_HANDLE_VALUE && hfd != NULL) {
        FindClose(hfd);
    }
    hfd = INVALID_HANDLE_VALUE;
    snprintf_safe(&pat,&patsize,NULL);
    SETERRNO(ret);
    return ret;
}

int __enumerate_dir_inner(char* basedir,char* curdir,int idx,enum_callback_t callback,void* arg)
{
    pfile_sub_item_t pitems=NULL;
    int itemsize=0;
    int itemlen=0;
    int i;
    char* pnextdir=NULL;
    int nextsize=0;
    int conted=1;
    int ret;

    ret = __list_all_subitems(basedir,curdir,&pitems,&itemsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }
    itemlen = ret;

    for (i=0;i< itemlen;i++) {
        if (strcmp(pitems[i].m_name,".") == 0 || 
            strcmp(pitems[i].m_name,"..") == 0) {
            continue;
        }
        if (callback != NULL) {
            ret = callback(basedir,curdir,pitems[i].m_name,arg);
            if (ret == 0) {
                conted = 0;
                break;
            } else if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
        }
        if (pitems[i].m_attr & FILE_ATTRIBUTE_DIRECTORY) {
            ret = snprintf_safe(&pnextdir,&nextsize,"%s\\%s",curdir,pitems[i].m_name);
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
            ret = __enumerate_dir_inner(basedir,pnextdir,(idx + 1),callback,arg);
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            } else if (ret == 0) {
                /*not continued*/
                conted = 0;
                break;
            }
        }
    }

    snprintf_safe(&pnextdir,&nextsize,NULL);
    __list_all_subitems(NULL,NULL,&pitems,&itemsize);
    return conted;
fail:
    snprintf_safe(&pnextdir,&nextsize,NULL);
    __list_all_subitems(NULL,NULL,&pitems,&itemsize);
    return ret;
}

int enumerate_directory(char* basedir,enum_callback_t callback,void* arg)
{    
    return __enumerate_dir_inner(basedir,basedir,0,callback,arg);
}

#define   FILE_OV_MAGIC      0x77021123

typedef struct __file_ov {
    uint32_t m_magic;
    uint32_t m_reserv1;
    HANDLE m_filehd;
    OVERLAPPED m_rdov;
    OVERLAPPED m_wrov;
    int m_rdcomplete;
    int m_wrcomplete;
    std::vector<char*>  *m_pwrbufs;
    std::vector<int> *m_wrlens;
    char* m_pwbuf;
    int m_wlen;
    int m_wsize;
    char* m_prbuf;
    int m_rlen;
    int m_rsize;
    char* m_fname;
} file_ov_t,*pfile_ov_t;

void __free_file_ov(pfile_ov_t* ppov)
{
    BOOL bret;
    int ret;
    if (ppov && *ppov) {
        pfile_ov_t pov = *ppov;
        if (pov->m_rdcomplete == 0) {
            bret = CancelIoEx(pov->m_filehd,&(pov->m_rdov));
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("cancel read [%s] error[%d]", pov->m_fname, ret);
            }
        }
        pov->m_rdcomplete = 1;

        if (pov->m_wrcomplete == 0) {
            bret = CancelIoEx(pov->m_filehd, &(pov->m_wrov));
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("cancel write [%s] error[%d]",pov->m_fname, ret);
            }
        }
        pov->m_wrcomplete = 1;

        if (pov->m_pwrbufs != NULL && pov->m_wrlens != NULL) {
            while(pov->m_pwrbufs->size() > 0) {
                ASSERT_IF(pov->m_pwrbufs->size() == pov->m_wrlens->size());
                char* pwbuf = pov->m_pwrbufs->at(0);
                int wlen = pov->m_wrlens->at(0);
                pov->m_pwrbufs->erase(pov->m_pwrbufs->begin());
                pov->m_wrlens->erase(pov->m_wrlens->begin());
                free(pwbuf);
                pwbuf = NULL;
                wlen = 0;
            }            
        }

        if (pov->m_pwrbufs != NULL) {
            delete pov->m_pwrbufs;
        }
        pov->m_pwrbufs = NULL;

        if (pov->m_wrlens != NULL) {
            delete pov->m_wrlens;
        }
        pov->m_wrlens = NULL;

        if (pov->m_pwbuf) {
            free(pov->m_pwbuf);
        }
        pov->m_pwbuf = NULL;
        pov->m_wsize = 0;
        pov->m_wlen = 0;

        /*we do not free this function*/
        pov->m_prbuf = NULL;
        pov->m_rlen = 0;
        pov->m_rsize = 0;

        if (pov->m_rdov.hEvent != NULL) {
            CloseHandle(pov->m_rdov.hEvent);
        }
        pov->m_rdov.hEvent = NULL;

        if (pov->m_wrov.hEvent != NULL) {
            CloseHandle(pov->m_wrov.hEvent);
        }
        pov->m_wrov.hEvent = NULL;

        if (pov->m_fname) {
            free(pov->m_fname);
        }
        pov->m_fname = NULL;
        pov->m_magic = 0;
        pov->m_filehd = NULL;
        free(pov);
        *ppov = NULL;
    }
}

pfile_ov_t __alloc_file_ov(HANDLE hd,const char* fname)
{
    pfile_ov_t pov= NULL;
    int ret;

    if (hd == NULL || hd == INVALID_HANDLE_VALUE || fname == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    pov = (pfile_ov_t) malloc(sizeof(*pov));
    if (pov == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    memset(pov,0,sizeof(*pov));
    pov->m_rdcomplete = 1;
    pov->m_wrcomplete = 1;
    pov->m_magic = FILE_OV_MAGIC;
    pov->m_filehd = hd;
    pov->m_fname = NULL;
    pov->m_pwrbufs = NULL;
    pov->m_wrlens = NULL;
    pov->m_pwbuf = NULL;
    pov->m_wsize = 0;
    pov->m_wlen = 0;
    pov->m_fname = _strdup(fname);
    if (pov->m_fname == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    pov->m_rdov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pov->m_rdov.hEvent == NULL) {
        GETERRNO(ret);
        ERROR_INFO("create [%s] read event error[%d]",pov->m_fname, ret);
        goto fail;
    }

    pov->m_wrov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pov->m_wrov.hEvent == NULL) {
        GETERRNO(ret);
        ERROR_INFO("create [%s] write event error[%d]", pov->m_fname, ret);
        goto fail;
    }

    pov->m_pwrbufs = new std::vector<char*>();
    pov->m_wrlens = new std::vector<int>();


    return pov;
fail:
    __free_file_ov(&pov);
    SETERRNO(ret);
    return NULL;
}


int exist_file(const char* fname)
{
    TCHAR* tfname=NULL;
    int tfsize=0;
    int ret=0;
    int isfile=0;
    DWORD fattr = INVALID_FILE_ATTRIBUTES;

    ret = AnsiToTchar(fname,&tfname,&tfsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    fattr = GetFileAttributes(tfname);
    if (fattr == INVALID_FILE_ATTRIBUTES) {
        GETERRNO(ret);
        ERROR_INFO("[%s] can not get [%d]",fname,ret);
        goto fail;
    }

    if ((fattr & FILE_ATTRIBUTE_READONLY) != 0 ||
        (fattr & FILE_ATTRIBUTE_HIDDEN) != 0 || 
        (fattr & FILE_ATTRIBUTE_SYSTEM) != 0 ||
        (fattr & FILE_ATTRIBUTE_ARCHIVE) != 0 || 
        (fattr & FILE_ATTRIBUTE_NORMAL) != 0 ||
        (fattr & FILE_ATTRIBUTE_SPARSE_FILE) != 0) {
        isfile = 1;
    }

    AnsiToTchar(NULL,&tfname,&tfsize);
    return isfile;
fail:
    AnsiToTchar(NULL,&tfname,&tfsize);
    SETERRNO(ret);
    return 0;


}

int exist_dir(const char* dname)
{
    TCHAR* tdname=NULL;
    int tdsize=0;
    int ret=0;
    int isdir=0;
    DWORD fattr = INVALID_FILE_ATTRIBUTES;

    ret = AnsiToTchar(dname,&tdname,&tdsize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    fattr = GetFileAttributes(tdname);
    if (fattr == INVALID_FILE_ATTRIBUTES) {
        GETERRNO(ret);
        ERROR_INFO("[%s] can not get [%d]",dname,ret);
        goto fail;
    }

    if ((fattr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        isdir = 1;
    }

    AnsiToTchar(NULL,&tdname,&tdsize);
    return isdir;
fail:
    AnsiToTchar(NULL,&tdname,&tdsize);
    SETERRNO(ret);
    return 0;


}


#if _MSC_VER >= 1910
#pragma warning(pop)
#endif