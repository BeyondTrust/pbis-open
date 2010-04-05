#include "includes.h"

GroupPolicyMessageType handle_domain_join_signal()
{
    return JOIN_SUCCESSFUL;
}

GroupPolicyMessageType handle_domain_leave_signal()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(ceError == CENTERROR_SUCCESS)
        return LEAVE_SUCCESSFUL;
    else
        return LEAVE_FAILED;
}
