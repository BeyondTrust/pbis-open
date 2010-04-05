/* Include after config.h.  Put all the #ifdef HAVE_... in this file. */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
   #include <strings.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_SYS_VARARGS_H
   #include <sys/varargs.h>
#endif

#ifdef HAVE_LIBGEN_H
   #include <libgen.h>
#endif

#ifdef HAVE_TIME_H
    #include <time.h>
#endif

#ifdef HAVE_SYS_TIME_H
    #include <sys/time.h>
#endif

