#include "ctbase.h"
#include "ctshell.h"

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    char * array[] =
    {
        "$x", "$y", "$z", NULL
    };

    char *buffer = NULL;

    BAIL_ON_CENTERIS_ERROR(
        ceError = CTShell("echo foo %one bar %two baz %three '&this %is $quoted' \"$HOME $USER\" >%capture",
                          CTSHELL_INTEGER (    one, 5            ),
                          CTSHELL_STRING  (    two, "hello world \" & $ ( )"),
                          CTSHELL_ARRAY   (  three, array        ),
                          CTSHELL_BUFFER  (capture, &buffer      )));

    printf("Buffer: %s\n", buffer);
error:
    LW_SAFE_FREE_STRING(buffer);
    return(ceError);
}
