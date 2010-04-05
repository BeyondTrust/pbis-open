
#include "netutilities.h"

typedef USER_ADD_INFO_PARAMS
{

}NET_USER_ADD_INFO_PARAMS, *PUSER_ADD_INFO_PARAMS;

typedef struct USER_COMMAND_INFO {
    DWORD dwControlCode;
    union {
        USER_ADD_INFO_PARAMS ShareAddInfo;
        USER_DEL_INFO_PARAMS ShareDelInfo;
        USER_ENUM_INFO_PARAMS ShareEnumInfo;
    }
}USER_COMMAND_INFO, *PUSER_COMMAND_INFO;

DWORD
NetUser(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PUSER_COMMAND_INFO pUserCommandInfo = NULL;

    dwError = UserParseArguments(
                    argc,
                    argv
                    &pUserCommandInfo
                    );
    BAIL_ON_ERROR(dwError);

    switch(pUserCommandInfo->dwControlCode) {

        case NET_USER_ADD:
            dwError = NetUserAdd();
            break;

        case NET_USER_DEL:
            dwError = NetUserDel(NULL, pUserCommandInfo->pUserName);
            break;

        case NET_USER_ENUM:
            dwError = NetUserEnum(NULL, 0, );
            break;
    }
    return dwError;
}


