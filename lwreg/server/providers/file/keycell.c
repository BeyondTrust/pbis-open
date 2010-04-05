
typedef _REG_KEYLIST {
    WCHAR PathName[256];
    DWORD dwKeyCellIndex;
}

typedef struct _REG_KEYLIST {
    DWORD dwNumEntries;
    DWORD dwNumEntriesUsed;
    DWORD KeyCellIndex[50];
}

DWORD
RegStoreCreateKeyListCell(
    DWORD dwKeyCellIndex
    DWORD *pdwKeyListCellIndex
    )
{
    dwError = RegStoreAllocateRecord(
                        CELL_TYPE_KEY_LIST,
                        &dwKeyListCellIndex
                        );
    BAIL_ON_ERROR(dwError);

    dwError =RegStoreUpdateKeyCell(
                    KEY_LIST_FIELD,
                    dwKeyListCellIndex
                    );
    BAIL_ON_ERROR(dwError);

error:

    return(dwError);
}

DWORD
RegStoreFindKeyinKeyListCell(
    DWORD dwKeyListCellIndex,
    PWSTR pszKeyName,
    DWORD *pdwKeyCellIndex
    )
{
    dwError = RegStoreGetKeyList(
                    dwKeyListCellIndex,
                    &pRegKeyList
                    );
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < pRegKeyList->dwNumEntriesUsed; i++) {
            dwError = RegStoreGetKeyName(
                            pRegKeyList->KeyCellIndex[i],
                            &pszKeyName
                            );
            BAIL_ON_ERROR(dwError);
        
            if (!wcscmp(pszKeyName, pszCurrentKeyName)){

                    dwError = 0;
                    *pdwKeyCellIndex = pRegKeyList-><dwKeyCellIndex;
                    return (dwError);
            }
    }

error:

    return dwError;

}


DWORD
RegStoreDeleteKeyListCell(
    DWORD dwKeyListCellIndex
    )
{
    DWORD dwError = 0;

    dwError = RegStoreGetNextKeyListCell(
                    dwKeyListCellIndex,
                    &dwNexKeyListCellIndex
                    );
    BAIL_ON_ERROR(dwError);

}

DWORD
RegStoreAddKeyinKeyListCell(
    DWORD dwKeyListCellIndex,
    DWORD dwKeyCellIndex,
    PWSTR pszKeyName
    )
{
    DWORD dwError = 0;



}


DWORD
RegStoreRemoveKeyinKeyListCell(
    DWORD dwKeyListCellIndex,
    PWSTR pszKeyName
    )
{


}
    
