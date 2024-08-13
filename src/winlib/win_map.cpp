#include <win_types.h>
#include <win_map.h>
#include <win_err.h>
#include <win_output_debug.h>
#include <win_uniansi.h>
#include <win_priv.h>

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

typedef struct __map_buffer_t {
    HANDLE m_hfile;
    HANDLE m_maphd;
    void* m_mapbuf;
    char* m_name;
    uint64_t m_size;
} map_buffer_t, *pmap_buffer_t;


void __free_map_buffer(pmap_buffer_t *ppmap)
{
    BOOL bret;
    int ret;
    if (ppmap && *ppmap) {
        pmap_buffer_t pmap = *ppmap;
        if (pmap->m_mapbuf) {
            bret = UnmapViewOfFile(pmap->m_mapbuf);
            if (!bret) {
                GETERRNO(ret);
                ERROR_INFO("unmap [%s] error[%d]", pmap->m_name ? pmap->m_name : "NULL", ret);
            }
            pmap->m_mapbuf = NULL;
        }

        if (pmap->m_maphd != NULL) {
            CloseHandle(pmap->m_maphd);
            pmap->m_maphd = NULL;
        }
        pmap->m_size = 0;

        if (pmap->m_hfile != NULL) {
            CloseHandle(pmap->m_hfile);
            pmap->m_hfile = NULL;
        }

        if (pmap->m_name) {
            free(pmap->m_name);
        }
        pmap->m_name = NULL;
        free(pmap);
        *ppmap = NULL;
    }
}

int __get_map_access(int flag, DWORD *prwflag)
{
    DWORD rwflag = 0;
    int ret;

    if ((flag & WINLIB_MAP_FILE_FLAGS) != flag) {
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("flag [0x%x] not valid", flag);
        goto fail;
    }

    if (flag & WINLIB_MAP_FILE_READ) {
        rwflag |= FILE_MAP_READ;
    }

    if (flag  & WINLIB_MAP_FILE_EXEC) {
        rwflag |= FILE_MAP_EXECUTE;
    }

    if (flag & WINLIB_MAP_FILE_WRITE) {
        rwflag |= FILE_MAP_WRITE;
    }

    if (prwflag) {
        *prwflag = rwflag;
    }
    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int __get_map_prot(int flag, DWORD *pprot)
{
    DWORD prot = 0;
    int ret;
    switch ((flag & (WINLIB_MAP_FILE_WRITE | WINLIB_MAP_FILE_READ | WINLIB_MAP_FILE_EXEC))) {
    case (WINLIB_MAP_FILE_READ|WINLIB_MAP_FILE_WRITE):
    case (WINLIB_MAP_FILE_WRITE):
        prot |= PAGE_READWRITE;
        prot |= SEC_COMMIT;
        break;
    case (WINLIB_MAP_FILE_WRITE|WINLIB_MAP_FILE_READ|WINLIB_MAP_FILE_EXEC):
        prot |= PAGE_EXECUTE_READWRITE;
        prot |= SEC_COMMIT;
        break;
    case (WINLIB_MAP_FILE_READ|WINLIB_MAP_FILE_EXEC):
        prot |= PAGE_EXECUTE_READ;
        break;
    case (WINLIB_MAP_FILE_READ):
        prot |= PAGE_READONLY;
        break;
    default:
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("flag [0x%x] not valid", flag);
        goto fail;
    }

    if (pprot) {
        *pprot = prot;
    }
    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int __map_buffer(pmap_buffer_t pmap, int flag, uint64_t size)
{
    DWORD rwflag = 0;
    int ret;

    ret = __get_map_access(flag, &rwflag);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    pmap->m_mapbuf = MapViewOfFile(pmap->m_maphd, rwflag, 0, (DWORD)((size >> 32) & 0xffffffff ), (DWORD)(size & 0xffffffff));
    if (pmap->m_mapbuf == NULL) {
        GETERRNO(ret);
        ERROR_INFO("map view [%s] with [%lld:0x%llx] error[%d]", pmap->m_name ? pmap->m_name : "NULL", size, size, ret);
        goto fail;
    }

    //DEBUG_INFO("map [%s]:[%p] to buffer [%p] rwflag [0x%lx]", pmap->m_name, pmap->m_maphd, pmap->m_mapbuf, rwflag);

    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int __map_file(pmap_buffer_t pmap, int flag, uint64_t size)
{
    DWORD rwflag;
    int ret;
    ret = __get_map_access(flag, &rwflag);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    pmap->m_mapbuf = MapViewOfFile(pmap->m_hfile, rwflag, 0 , (DWORD)((size >> 32) & 0xffffffff), (DWORD)(size & 0xffffffff));
    if (pmap->m_mapbuf == NULL) {
        GETERRNO(ret);
        ERROR_INFO("map file[%s] with [%lld:0x%llx] error [%d]", pmap->m_name, size, size, ret);
        goto fail;
    }
    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int __map_memory_name(pmap_buffer_t pmap, int flag, uint64_t size)
{
    TCHAR* ptname = NULL;
    int tnamesize = 0;
    int ret;
    DWORD prot = 0;
    DWORD rwflag = 0;
    int enabled = 0;
    if (pmap->m_name != NULL) {
        ret = AnsiToTchar(pmap->m_name, &ptname, &tnamesize);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
    }

    ret = __get_map_access(flag, &rwflag);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __get_map_prot(flag, &prot);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (ptname != NULL && (flag & WINLIB_MAP_CREATE) == 0) {
        pmap->m_maphd = OpenFileMapping(rwflag, FALSE, ptname);
    } else if (ptname != NULL && (flag & WINLIB_MAP_CREATE)) {
        if (flag & WINLIB_MAP_GLOBAL) {
            ret = enable_create_global_priv();
            if (ret < 0) {
                GETERRNO(ret);
                goto fail;
            }
            enabled = 1;
        }
        if (flag & WINLIB_MAP_FORALL) {
            SECURITY_ATTRIBUTES sa;
            SECURITY_DESCRIPTOR sd;
            InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
            SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE);
            sa.nLength = (DWORD) sizeof(SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = (LPVOID) &sd;
            sa.bInheritHandle = TRUE;
            DEBUG_INFO("creat for everyone [%s]", pmap->m_name);
            pmap->m_maphd = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, prot, 0, (DWORD)size, ptname);
        } else {
            DEBUG_INFO("creat not for everyone [%s]", pmap->m_name);
            pmap->m_maphd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, prot, 0, (DWORD)size, ptname);
        }

    }

    if (pmap->m_maphd == NULL) {
        GETERRNO(ret);
        ERROR_INFO("can not open or create map [%s] error[%d]", pmap->m_name ? pmap->m_name : "NULL", ret);
        goto fail;
    }

    if (enabled) {
        disable_create_global_priv();
        enabled = 0;
    }


    AnsiToTchar(NULL, &ptname, &tnamesize);
    return 0;
fail:
    if (enabled) {
        disable_create_global_priv();
        enabled = 0;
    }
    AnsiToTchar(NULL, &ptname, &tnamesize);
    SETERRNO(ret);
    return ret;
}

pmap_buffer_t __alloc_map_buffer(char* name)
{
    pmap_buffer_t pmap = NULL;
    int ret;

    pmap = (pmap_buffer_t)malloc(sizeof(*pmap));
    if (pmap == NULL) {
        GETERRNO(ret);
        goto fail;
    }
    memset(pmap, 0, sizeof(*pmap));
    if (name != NULL) {
        pmap->m_name = _strdup(name);
        if (pmap->m_name == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }


    return pmap;
fail:
    __free_map_buffer(&pmap);
    SETERRNO(ret);
    return NULL;
}

int map_buffer(char* name, int flag, uint64_t size, void** ppmap)
{
    pmap_buffer_t pmap = NULL;
    int ret;
    if (ppmap == NULL || *ppmap != NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pmap = __alloc_map_buffer(name);
    if (pmap == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __map_memory_name(pmap, flag, size);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __map_buffer(pmap, flag, size);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    pmap->m_size = size;


    *ppmap = pmap;
    return 0;
fail:
    __free_map_buffer(&pmap);
    SETERRNO(ret);
    return ret;
}

int write_buffer(void* pmap1, uint64_t offset, void* pbuf, int size)
{
    int ret;
    pmap_buffer_t pmap = (pmap_buffer_t) pmap1;
    int wsize = size;
    char* pinbuf = NULL;
    if (pmap == NULL || pmap->m_mapbuf == NULL || pbuf == NULL ||
            offset > pmap->m_size) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pinbuf = (char*) pmap->m_mapbuf;
    if ((offset + size) > pmap->m_size) {
        wsize = (int)(pmap->m_size - offset);
    }
    memcpy(&(pinbuf[offset]), pbuf, (size_t)wsize);
    return wsize;
}

int __create_file_flag(int flag, DWORD* pfileflag)
{
    DWORD fileflag = 0;
    if (flag & WINLIB_MAP_FILE_READ) {
        fileflag |= GENERIC_READ;
    }

    if (flag & WINLIB_MAP_FILE_WRITE) {
        fileflag |= GENERIC_WRITE;
    }

    if (pfileflag) {
        *pfileflag = fileflag;
    }
    return 0;
}

int __share_file_flag(int flag, DWORD* psharemode)
{
    DWORD sharemode = 0;
    if (flag & WINLIB_MAP_FILE_WRITE ||
            flag & WINLIB_MAP_FILE_READ) {
        sharemode |= FILE_SHARE_READ;
    }

    if (psharemode) {
        *psharemode = sharemode;
    }
    return 0;
}

int __file_open_mode(int flag , DWORD *pomode)
{
    DWORD omode = 0;
    if (flag & WINLIB_MAP_FILE_READ ||
            flag & WINLIB_MAP_FILE_EXEC) {
        omode |= OPEN_EXISTING;
    }
    if (flag & WINLIB_MAP_FILE_WRITE) {
        omode |= CREATE_ALWAYS;
    }

    if (pomode) {
        *pomode = omode;
    }
    return 0;
}

int __file_attr_flag(int flag, DWORD* pattr)
{
    DWORD attr = FILE_FLAG_NO_BUFFERING;
    int ret;
    if ((flag & WINLIB_MAP_FILE_FLAGS) != flag) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }
    switch ( (flag & WINLIB_MAP_FILE_FLAGS)) {
    case (WINLIB_MAP_FILE_READ|WINLIB_MAP_FILE_WRITE):
    case (WINLIB_MAP_FILE_WRITE|WINLIB_MAP_FILE_READ|WINLIB_MAP_FILE_EXEC):
    case (WINLIB_MAP_FILE_EXEC|WINLIB_MAP_FILE_WRITE):
    case (WINLIB_MAP_FILE_WRITE):
        attr |= FILE_FLAG_WRITE_THROUGH;
        break;
    case (WINLIB_MAP_FILE_READ):
    case (WINLIB_MAP_FILE_READ|WINLIB_MAP_FILE_EXEC):
    case (WINLIB_MAP_FILE_EXEC):
        attr |= FILE_ATTRIBUTE_READONLY;
        break;
    default:
        ret = -ERROR_INVALID_PARAMETER;
        ERROR_INFO("not valid flag [0x%x]", flag);
        goto fail;
    }
    if (pattr) {
        *pattr = attr;
    }
    return 0;
fail:
    SETERRNO(ret);
    return ret;
}

int __open_file_map(pmap_buffer_t pmap, int flag)
{
    DWORD sharemode = 0;
    DWORD fileflag = 0;
    DWORD attr = 0;
    DWORD omode = 0;
    int ret;
    TCHAR* ptname = NULL;
    int tnamesize = 0;

    if (pmap == NULL ||
            pmap->m_name == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    ret = __create_file_flag(flag, &fileflag);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __share_file_flag(flag, &sharemode);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __file_attr_flag(flag, &attr);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __file_open_mode(flag, &omode);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    ret = AnsiToTchar(pmap->m_name, &ptname, &tnamesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    DEBUG_INFO("open [%s] fileflag [0x%lx] sharemode [0x%lx] omode[0x%lx] attr[0x%lx] ", pmap->m_name, fileflag, sharemode, omode, attr);
    pmap->m_hfile = CreateFile(ptname, fileflag, sharemode, NULL, omode, attr, NULL);
    if (pmap->m_hfile == INVALID_HANDLE_VALUE) {
        GETERRNO(ret);
        pmap->m_hfile = NULL;
        ERROR_INFO("open [%s] file for [0x%x] error[%d]", pmap->m_name, flag, ret);
        goto fail;
    }

    AnsiToTchar(NULL, &ptname, &tnamesize);
    return 0;
fail:
    AnsiToTchar(NULL, &ptname, &tnamesize);
    SETERRNO(ret);
    return ret;
}

int __get_file_size(pmap_buffer_t pmap, uint64_t* psize)
{
    BOOL bret;
    LARGE_INTEGER retoff;
    LARGE_INTEGER setoff;
    int ret;
    if (pmap == NULL || pmap->m_hfile == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }
    setoff.QuadPart = 0;

    bret = SetFilePointerEx(pmap->m_hfile, setoff, &retoff, FILE_END);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("file [%s] seek end error[%d]", pmap->m_name, ret);
        goto fail;
    }

    if (psize) {
        *psize = (uint64_t) retoff.QuadPart;
    }

    return 0;
fail:
    SETERRNO(ret);
    return ret;
}


int map_file(char* name, int flag, uint64_t* psize, void** ppmap)
{
    int ret;
    pmap_buffer_t pmap = NULL;
    uint64_t filesize = 0;
    if (ppmap == NULL || *ppmap != NULL || psize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    pmap = __alloc_map_buffer(name);
    if (pmap == NULL) {
        GETERRNO(ret);
        goto fail;
    }

    ret = __open_file_map(pmap, flag);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    if (flag & WINLIB_MAP_FILE_WRITE) {
        if (psize) {
            filesize = *psize;
        }
    } else {
        ret = __get_file_size(pmap, &filesize);
        if (ret < 0) {
            GETERRNO(ret);
            goto fail;
        }
    }

    /*now map */
    ret = __map_file(pmap, flag, filesize);
    if (ret < 0) {
        GETERRNO(ret);
        goto fail;
    }

    pmap->m_size = filesize;

    *psize = filesize;
    *ppmap = pmap;
    return 0;
fail:
    __free_map_buffer(&pmap);
    SETERRNO(ret);
    return ret;
}


int read_buffer(void* pmap1, uint64_t offset, void* pbuf, int size)
{
    int ret;
    pmap_buffer_t pmap = (pmap_buffer_t) pmap1;
    int rsize = size;
    char* poutbuf = NULL;
    if (pmap == NULL || pmap->m_mapbuf == NULL || pbuf == NULL ||
            offset > pmap->m_size) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    poutbuf = (char*) pmap->m_mapbuf;
    if ((offset + size) > pmap->m_size) {
        rsize = (int)(pmap->m_size - offset);
    }
    memcpy( pbuf, &(poutbuf[offset]), (size_t)rsize);
    return rsize;
}


void unmap_buffer(void** ppmap)
{
    pmap_buffer_t pmap;
    if (ppmap && *ppmap) {
        pmap = (pmap_buffer_t) * ppmap;
        __free_map_buffer(&pmap);
        *ppmap = NULL;
    }
    return ;
}
