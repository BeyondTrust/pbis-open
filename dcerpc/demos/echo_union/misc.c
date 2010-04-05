#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <dce/rpc.h>
#include <dce/dce_error.h>
#include <stdlib.h>

void 
chk_dce_err(ecode, where, why, fatal)
     error_status_t ecode;
     char * where;
     char * why;
     unsigned int fatal;
{

  dce_error_string_t errstr;
  int error_status;                           
  
  if (ecode != error_status_ok)
    {
       dce_error_inq_text(ecode, errstr, &error_status); 
       if (error_status == error_status_ok)
	 printf("ERROR.  where = <%s> why = <%s> error code = 0x%lx"
		"reason = <%s>\n",
	      where, why, ecode, errstr);
       else
	 printf("ERROR.  where = <%s> why = <%s> error code = 0x%lx\n",
	      where, why, ecode);
       
       if (fatal) exit(1);
    }
}

void* midl_user_allocate(idl_size_t size)
{
    void *result = malloc(size);
    fprintf(stderr, "USER_ALLOC: %u -> %p\n", size, result);
    return result;
}

void midl_user_free(void* obj)
{
    fprintf(stderr, "USER_FREE: %p\n", obj);
}
