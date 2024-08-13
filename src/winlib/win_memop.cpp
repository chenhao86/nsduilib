#include <win_memop.h>
#include <win_err.h>
#include <win_types.h>

#if _MSC_VER >= 1929
#pragma warning(disable:5045)
#endif

DWORD __filter_function()
{
    return EXCEPTION_EXECUTE_HANDLER;
}

#pragma optimize("",off)
int memory_valid(void* ptr, int memsize)
{
    int ret = 0;
    unsigned char* pgetptr = (unsigned char*)ptr;
    unsigned char con = 0;
    int i;

    __try {
        pgetptr = (unsigned char*)ptr;
        for (i = 0; i < memsize; i++, pgetptr ++) {
            con = *pgetptr;
        }
    }

    __except (__filter_function()) {
        ret = -ERROR_ACCESS_DENIED;
    }
    SETERRNO(-ret);
    return ret;
}

#pragma optimize("",on)

int memory_set_mode(void* ptr, int memsize, int mode, int *porigmode)
{
    addr_t pstart = (addr_t)ptr;
    int ret;
    DWORD newprotect = 0;
    DWORD oldprotect = 0;
    BOOL bret;

    if (pstart  & MEM_ALIGN_MASK) {
        ret =  -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (memsize & MEM_ALIGN_MASK) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (memsize != MEM_PAGE_SIZE) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (mode == MEM_EXECUTE) {
        newprotect = PAGE_EXECUTE;
    } else if (mode == (MEM_READ | MEM_EXECUTE)) {
        newprotect = PAGE_EXECUTE_READ;
    } else if (mode == (MEM_READ | MEM_WRITE)) {
        newprotect = PAGE_READWRITE;
    } else if (mode == (MEM_READ | MEM_WRITE | MEM_EXECUTE)) {
        newprotect = PAGE_EXECUTE_READWRITE;
    } else if (mode == MEM_READ) {
        newprotect = PAGE_READONLY;
    } else {
        /*we do not access the mode*/
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    __try {
        bret = VirtualProtect(ptr, (size_t)memsize, newprotect, &oldprotect);
        DEBUG_INFO("");
        if (!bret) {
            GETERRNO(ret);
            ERROR_INFO("can not protect 0x%p:0x%08x mode(0x%08x) error(%d)", ptr, memsize, newprotect, ret);
            goto fail;
        }
    }

    __except (__filter_function()) {
        ERROR_INFO("0x%p memsize 0x%08x newprotect 0x%08x error", ptr, memsize, newprotect);
        ret = -ERROR_ACCESS_DENIED;
        goto fail;
    }

    *porigmode = (int)oldprotect;
    return 0;
fail:
    SETERRNO(-ret);
    return ret;
}

int memory_reset_mode(void* ptr, int memsize, int origmode)
{
    DWORD oldprotect = 0;
    DWORD newprotect = (DWORD)origmode;
    addr_t pstart = (addr_t)ptr;
    int ret;
    BOOL bret;

    if (pstart  & MEM_ALIGN_MASK) {
        ret =  -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (memsize & MEM_ALIGN_MASK) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    if (memsize != MEM_PAGE_SIZE) {
        ret = -ERROR_INVALID_PARAMETER;
        goto fail;
    }

    bret = VirtualProtect(ptr, (size_t)memsize, newprotect, &oldprotect);
    if (!bret) {
        GETERRNO(ret);
        ERROR_INFO("can not reset 0x%p:0x%08x newprotect(0x%08x) error(%d)", ptr, memsize, newprotect, ret);
        goto fail;
    }

    return 0;
fail:
    SETERRNO(-ret);
    return ret;
}