#include "fserv.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

int 
main(int argc, char** argv)
{
    int ret = 0;
    int i;
    FServFile* file = NULL;
    char buffer[2048];
    unsigned long size_read = 0;

    for (i = 1; i < argc; i++)
    {
        ret = fserv_open(argv[i], FSERV_MODE_READ, &file);
        if (ret)
        {
            goto error;
        }

        do
        {
            ret = fserv_read(file, sizeof(buffer), buffer, &size_read);
            if (ret)
            {
                goto error;
            }
            
            if (size_read)
            {
                if (fwrite(buffer, 1, size_read, stdout) < size_read)
                {
                    ret = errno;
                    goto error;
                }
            }
        } while (size_read > 0);

        ret = fserv_close(file);
        file = NULL;
        if (ret)
        {
            goto error;
        }
    }
    
error:
    
    if (file)
    {
        fserv_close(file);
    }

    if (ret)
    {
        if (ret == -1)
        {
            fprintf(stderr, "Unexpected error\n");
        }
        else
        {
            fprintf(stderr, "%s\n", strerror(ret));
        }
    }

    return ret;
}
