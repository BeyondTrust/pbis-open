
#include "netutilities.h"

typedef VIEW_ADD_INFO_PARAMS
{

}NETVIEW_ADD_INFO_PARAMS, *PVIEW_ADD_INFO_PARAMS;

typedef struct VIEW_COMMAND_INFO {
    DWORD dwControlCode;
    union {
        VIEW_ADD_INFO_PARAMS ViewAddInfo;
        VIEW_DEL_INFO_PARAMS ViewDelInfo;
        VIEW_ENUM_INFO_PARAMS ViewEnumInfo;
    }
}VIEW_COMMAND_INFO, *PVIEW_COMMAND_INFO;



DWORD
NetView(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;

    dwError = ViewParseArguments(
                    argc,
                    argv
                    &pCommandInfo
                    );
    BAIL_ON_ERROR(dwError);

    switch(pCommandInfo->dwControlCode) {

        case NETVIEW_EMPTY:
            dwError = NetViewEmpty();
    

        case NETVIEW_DEL:
            dwError = NetViewUNCName();

    }
    return dwError;
}


