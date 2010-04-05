
#include "netutilities.h"

typedef struct _NETLOCALGROUP_ADD_INFO_PARAMS
{

}NETLOCALGROUP_ADD_INFO_PARAMS, *PNETLOCALGROUP_ADD_INFO_PARAMS;

typedef struct NETLOCALGROUP_COMMAND_INFO {
    DWORD dwControlCode;
    union {
        VIEW_ADD_INFO_PARAMS ViewAddInfo;
        VIEW_DEL_INFO_PARAMS ViewDelInfo;
        VIEW_ENUM_INFO_PARAMS ViewEnumInfo;
    }
}NETLOCALGROUP_COMMAND_INFO, *PNETLOCALGROUP_COMMAND_INFO;

DWORD
NetLocalGroup(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;

    dwError = LocalGroupParseArguments(
                    argc,
                    argv
                    &pCommandInfo
                    );
    BAIL_ON_ERROR(dwError);

    switch(pCommandInfo->dwControlCode) {

        case NETLOCALGROUP_EMPTY:
            dwError = NetLocalGroupEmpty();
    

        case NETLOCALGROUP_DEL:
            dwError = NetLocalGroupDel();

    }
    return dwError;
}


