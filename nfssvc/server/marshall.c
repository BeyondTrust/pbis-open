#include "includes.h"

static
DWORD
MarshallShareInfo1toFlatBuffer(
    PNET_SHARE_INFO1 pNetShareInfo1,
    PBYTE  * ppByte,
    DWORD * pdwBufferSize
    );

static
DWORD
MarshallShareInfo0toFlatBuffer(
    PNET_SHARE_INFO0 pNetShareInfo0,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    );

static
DWORD
MarshallShareInfo2toFlatBuffer(
    PNET_SHARE_INFO2 pNetShareInfo2,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    );

static
DWORD
MarshallShareInfo502toFlatBuffer(
    PNET_SHARE_INFO502 pNetShareInfo502,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    );

static
DWORD
MarshallShareInfo501toFlatBuffer(
    PNET_SHARE_INFO501 pNetShareInfo501,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    );

static
DWORD
MarshallShareInfo503toFlatBuffer(
    PNET_SHARE_INFO503 pNetShareInfo503,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    );

static
DWORD
ConvertOffsetstoPointers_L0(
    PBYTE pBuffer,
    PVOID pShareInfo
    );

static
DWORD
ConvertOffsetstoPointers_L1(
    PBYTE pBuffer,
    PVOID pShareInfo
    );

static
DWORD
ConvertOffsetstoPointers_L2(
    PBYTE pBuffer,
    PVOID pShareInfo
    );

static
DWORD
ConvertOffsetstoPointers_L502(
    PBYTE pBuffer,
    PVOID pShareInfo
    );

static
DWORD
ConvertOffsetstoPointers_L503(
    PBYTE pBuffer,
    PVOID pShareInfo
    );

DWORD
MarshallShareInfotoFlatBuffer(
    DWORD  dwLevel,
    PVOID  pShareInfo,
    PBYTE* ppBuffer,
    PDWORD pdwBufferSize
    )
{
    DWORD dwError = 0;

    switch (dwLevel) {

        case 0:
            dwError = MarshallShareInfo0toFlatBuffer(
                                pShareInfo,
                                ppBuffer,
                                pdwBufferSize
                                );
            break;

        case 1:
            dwError = MarshallShareInfo1toFlatBuffer(
                                pShareInfo,
                                ppBuffer,
                                pdwBufferSize
                                );
            break;


        case 2:
            dwError = MarshallShareInfo2toFlatBuffer(
                                pShareInfo,
                                ppBuffer,
                                pdwBufferSize
                                );
            break;

        case 3:
            dwError = MarshallShareInfo501toFlatBuffer(
                                pShareInfo,
                                ppBuffer,
                                pdwBufferSize
                                );
            break;

        case 502:
            dwError = MarshallShareInfo502toFlatBuffer(
                                pShareInfo,
                                ppBuffer,
                                pdwBufferSize
                                );
            break;

        case 503:
            dwError = MarshallShareInfo503toFlatBuffer(
                                pShareInfo,
                                ppBuffer,
                                pdwBufferSize
                                );
            break;
    }

    return(dwError);
}

DWORD
ConvertOffsetstoPointers(
    PBYTE  pInBuffer,
    DWORD  dwLevel,
    PVOID  pShareInfo
    )
{

    DWORD dwError = 0;

    switch (dwLevel) {

        case 0:
            dwError = ConvertOffsetstoPointers_L0(
                                pInBuffer,
                                pShareInfo
                                );
            break;

        case 1:
            dwError = ConvertOffsetstoPointers_L1(
                                pInBuffer,
                                pShareInfo
                                );
            break;


        case 2:
            dwError = ConvertOffsetstoPointers_L2(
                                pInBuffer,
                                pShareInfo
                                );
            break;

        case 502:
            dwError = ConvertOffsetstoPointers_L502(
                                pInBuffer,
                                pShareInfo
                                );
            break;

        case 503:
            dwError = ConvertOffsetstoPointers_L503(
                                pInBuffer,
                                pShareInfo
                                );
            break;
    }

    return dwError;
}

static
DWORD
MarshallShareInfo1toFlatBuffer(
    PNET_SHARE_INFO1 pNetShareInfo1,
    PBYTE  * ppByte,
    DWORD * pdwBufferSize
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
MarshallShareInfo0toFlatBuffer(
    PNET_SHARE_INFO0 pNetShareInfo0,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    )
{
    DWORD dwError = 0;

    return(dwError);
}

static
DWORD
MarshallShareInfo2toFlatBuffer(
    PNET_SHARE_INFO2 pNetShareInfo2,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    )
{
    DWORD dwError = 0;

    return(dwError);

}

static
DWORD
MarshallShareInfo502toFlatBuffer(
    PNET_SHARE_INFO502 pNetShareInfo502,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    )
{
    DWORD dwError = 0;

    return(dwError);
}

static
DWORD
MarshallShareInfo501toFlatBuffer(
    PNET_SHARE_INFO501 pNetShareInfo501,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    )
{
    DWORD dwError = 0;

    return(dwError);

}

static
DWORD
MarshallShareInfo503toFlatBuffer(
    PNET_SHARE_INFO503 pNetShareInfo503,
    PBYTE * ppByte,
    DWORD * pdwBufferSize
    )
{
    DWORD dwError = 0;


    return(dwError);
}

static
DWORD
ConvertOffsetstoPointers_L0(
    PBYTE pBuffer,
    PVOID pShareInfo
    )
{
    DWORD dwError = 0;

    return(dwError);

}

static
DWORD
ConvertOffsetstoPointers_L1(
    PBYTE pBuffer,
    PVOID pShareInfo
    )
{
    DWORD dwError = 0;

    return(dwError);

}

static
DWORD
ConvertOffsetstoPointers_L2(
    PBYTE pBuffer,
    PVOID pShareInfo
    )
{
    DWORD dwError = 0;

    return(dwError);

}

static
DWORD
ConvertOffsetstoPointers_L502(
    PBYTE pBuffer,
    PVOID pShareInfo
    )
{
    DWORD dwError = 0;

    return(dwError);

}

static
DWORD
ConvertOffsetstoPointers_L503(
    PBYTE pBuffer,
    PVOID pShareInfo
    )
{
    DWORD dwError = 0;

    return(dwError);
}

DWORD
UnmarshallAddSetResponse(
    PBYTE  pOutBuffer,    
    PDWORD pdwReturnCode,
    PDWORD pdwParmError
    )
{
    return 0;
}

