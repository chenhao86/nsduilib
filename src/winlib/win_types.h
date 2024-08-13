#ifndef __WIN_TYPES_H__
#define __WIN_TYPES_H__

#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4820)
#pragma warning(disable:4530)
#pragma warning(disable:4577)

#include <Windows.h>

#pragma warning(pop)

typedef UINT8         uint8_t;
typedef UINT16        uint16_t;
typedef UINT32        uint32_t;
typedef UINT64        uint64_t;

typedef INT8          int8_t;
typedef INT16         int16_t;
typedef INT32         int32_t;
typedef INT64         int64_t;


#ifdef _M_X64
typedef UINT64 addr_t;
typedef UINT64 flags_t;
#elif defined(_M_IX86)
typedef UINT32 addr_t;
typedef UINT32 flags_t;
#else
#error "not support architecture for windows compiler"
#endif

#define MAX_UINT64       (0xffffffffffffffffULL)


#define REFERENCE_ARG(arg)                                                                        \
do{                                                                                               \
    if ((arg)) {                                                                                  \
        arg = arg;                                                                                \
    }                                                                                             \
}while(0)



#ifdef __cplusplus
#define EXPR_C  "C"
#else
#define EXPR_C
#endif



#endif /*__WIN_TYPES_H__*/