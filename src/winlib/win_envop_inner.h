#ifndef __WIN_ENVOP_INNER_H_DD85A855129275D024E76D36BFE7A510__
#define __WIN_ENVOP_INNER_H_DD85A855129275D024E76D36BFE7A510__

#pragma warning(push)
#pragma warning(disable:4820)
#pragma warning(disable:4514)
#pragma warning(disable:4668)


#include <Windows.h>

#pragma warning(pop)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


#define  NTSTATUS_SUCCESS            0
#define  NTSTATUS_BUFFER_TOO_SMALL   0xC0000023

#define  NTSTATUS_FLT_NOT_INITIALIZED 0xC01C0007


NTSTATUS
NTAPI
NtSetSecurityObjectFake(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
NTAPI
NtQuerySecurityObjectFake(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    );


#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_ENVOP_INNER_H_DD85A855129275D024E76D36BFE7A510__ */
