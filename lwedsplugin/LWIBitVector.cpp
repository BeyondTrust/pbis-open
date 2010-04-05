#include "LWIBitVector.h"

long
LWIMakeBitVector(
    int nBits,
    PLWIBITVECTOR* ppBitVector
    )
{
    long macError = eDSNoErr;
    PLWIBITVECTOR pBitVector = NULL;

    if (nBits <= 0)
    {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWAllocateMemory(sizeof(LWIBITVECTOR), (PVOID*)&pBitVector);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWAllocateMemory((((nBits-1)/8)+1)*sizeof(uint8_t),
                                 (PVOID*)&pBitVector->data);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pBitVector->nBits = nBits;

    *ppBitVector = pBitVector;
    pBitVector = NULL;

cleanup:

    if (pBitVector)
        LWIFreeBitVector(pBitVector);

    return macError;
}

void
LWIFreeBitVector(
    PLWIBITVECTOR pBitVector
    )

{
    if (pBitVector->data)
        LWFreeMemory(pBitVector->data);

    LWFreeMemory(pBitVector);
}

