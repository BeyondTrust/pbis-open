#include "fserv.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

int 
main(int argc, char** argv)
{
    int ret = 0;
    FServFile* file = NULL;
    char buffer[2048];
    unsigned int size_read = 0;

    if (argc != 2)
    {
        ret = -1;
        goto error;
    }

    ret = fserv_open(argv[1], FSERV_MODE_WRITE, &file);
    if (ret)
    {
        goto error;
    }
    
    do
    {
        size_read = fread(buffer, 1, sizeof(buffer), stdin);

        if (size_read == 0)
        {
            if (!feof(stdin))
            {
                ret = errno;
                goto error;
            }
        }
        else
        {
            ret = fserv_write(file, size_read, buffer);
            if (ret)
            {
                goto error;
            }
        }
    } while (size_read > 0);

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
