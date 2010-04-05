#ifndef __MARSHALL_H__
#define __MARSHALL_H__

DWORD
MarshallShareInfotoFlatBuffer(
    DWORD  dwLevel,
    PVOID  pShareInfo,
    PBYTE* ppBuffer,
    PDWORD pdwBufferSize
    );

DWORD
ConvertOffsetstoPointers(
    PBYTE  pInBuffer,
    DWORD  dwLevel,
    PVOID  pShareInfo
    );

DWORD
UnmarshallAddSetResponse(
    PBYTE  pOutBuffer,
    PDWORD pdwReturnCode,
    PDWORD pdwParmError
    );

#endif /* __MARSHALL_H__ */

