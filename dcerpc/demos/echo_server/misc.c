/* ex: set shiftwidth=4 softtabstop=4 expandtab: */
#ifdef _MK_HOST
#include <config.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <compat/dcerpc.h>
#include "misc.h"

void 
chk_dce_err(
    error_status_t ecode,
    char * where,
    char * why,
    unsigned int fatal
    )
{
    dce_error_string_t errstr;
    int error_status;                           
  
    if (ecode != error_status_ok)
    {
        dce_error_inq_text(ecode, errstr, &error_status); 
        if (error_status == error_status_ok)
            printf("ERROR.  where = <%s> why = <%s> error code = 0x%lx"
                   "reason = <%s>\n",
                   where, why, (long int)ecode, errstr);
        else
            printf("ERROR.  where = <%s> why = <%s> error code = 0x%lx\n",
                   where, why, (long int)ecode);
       
        if (fatal) exit(1);
    }
}
