#include "gpagent.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = 0;
    
    GPOSetServerDefaults();

    ceError = GPAInitKrb5();
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPOLoop();

error:

    return (ceError);
}
