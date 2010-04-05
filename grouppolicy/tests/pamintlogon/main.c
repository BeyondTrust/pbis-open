#include "gpaclient.h"
#include "gpacommon.h"
#include "gppamsupp.h"

int
main(
    int argc,
    char* argv[]
    )
{
    PSTR val = NULL;
    if ( gp_pam_get_interactive_logon_rights(&val)) {
        printf("PAM Interactive Logon Rights: %s\n", val);
        gp_pam_free_buffer(val);
        return 1;
    }
    return 0;
}
